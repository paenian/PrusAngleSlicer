///|/
///|/ PrusAngleSlicer SLA 3rd-party printer support
///|/ Released under the terms of the AGPLv3 or higher
///|/
// Hermetic test of the UVtools acquire -> verify -> extract -> link -> convert path.
// A tiny zip containing a fake "UVtoolsCmd" shell script stands in for the real
// release, and a fake DownloadFn copies it instead of using the network. This
// exercises the real extraction (miniz), permission fix-up, checksum verification,
// path persistence ("linking") and subprocess invocation code.
//
// POSIX only (the fake tool is a shell script).
#ifndef _WIN32

#include <catch2/catch_test_macros.hpp>

#include <fstream>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>
#include <miniz.h>

#include "libslic3r/Format/UVTools/UVToolsService.hpp"

using namespace Slic3r::uvtools;
namespace fs = boost::filesystem;

namespace {

const char *ENTRY = "UVtools.app/Contents/MacOS/UVtoolsCmd";

// Fake tool: `UVtoolsCmd convert <in> <encoder> <out> [...]` copies in -> out.
const char *FAKE_TOOL = "#!/bin/sh\n[ \"$1\" = convert ] || exit 2\ncp \"$2\" \"$4\"\n";

struct Fixture {
    fs::path root, zip, install;
    std::string sha;

    Fixture()
    {
        root    = fs::temp_directory_path() / fs::unique_path("uvtools-svc-%%%%-%%%%");
        install = root / "install" / "v0.0.0";
        zip     = root / "release.zip";
        fs::create_directories(root);

        mz_zip_archive z;
        std::memset(&z, 0, sizeof(z));
        REQUIRE(mz_zip_writer_init_file(&z, zip.string().c_str(), 0));
        REQUIRE(mz_zip_writer_add_mem(&z, ENTRY, FAKE_TOOL, std::strlen(FAKE_TOOL), MZ_DEFAULT_COMPRESSION));
        REQUIRE(mz_zip_writer_finalize_archive(&z));
        REQUIRE(mz_zip_writer_end(&z));
        sha = *UVToolsService::sha256_file(zip.string());
    }
    ~Fixture() { boost::system::error_code ec; fs::remove_all(root, ec); }

    UVToolsRelease release(const std::string &sha_override = {}) const
    {
        UVToolsRelease r;
        r.version       = "0.0.0";
        r.url           = "https://example.invalid/fake.zip";
        r.sha256        = sha_override.empty() ? sha : sha_override;
        r.entry_relpath = ENTRY;
        r.install_dir   = install.string();
        return r;
    }

    DownloadFn fake_download() const
    {
        const fs::path src = zip;
        return [src](const std::string &, const std::string &dest, const ProgressFn &progress, std::string &) {
            if (progress) progress(0.5, "fake download");
            boost::system::error_code ec;
            fs::copy_file(src, dest, fs::copy_option::overwrite_if_exists, ec);
            return ! ec;
        };
    }
};

} // namespace

TEST_CASE("UVtools acquire: downloads, verifies, installs persistently and links", "[UVtools][service]")
{
    Fixture f;
    std::string persisted;
    UVToolsService svc({}, f.release(), f.fake_download(), [&](const std::string &p) { persisted = p; });

    REQUIRE_FALSE(svc.resolve_executable().has_value());   // nothing installed yet

    auto r = svc.ensure_available(nullptr);
    REQUIRE(r.ok());

    // Installed under the persistent install_dir, NOT a temp directory.
    const fs::path exe = fs::path(f.install) / ENTRY;
    CHECK(fs::exists(exe));
    CHECK(svc.resolved_executable() == exe.string());
    // Extraction dropped the unix exec bit; the service must have restored it.
    CHECK((fs::status(exe).permissions() & fs::perms::owner_exe) != fs::perms::no_perms);
    // "Linking": the resolved path was handed to the persistence callback.
    CHECK(persisted == exe.string());

    // A fresh service given that remembered path finds it with no download.
    UVToolsPaths remembered; remembered.cached_path = persisted;
    UVToolsService again(remembered, f.release(), /*no download*/ nullptr);
    CHECK(again.resolve_executable().has_value());
    CHECK(again.ensure_available(nullptr).ok());
}

TEST_CASE("UVtools acquire then convert runs the installed tool", "[UVtools][service]")
{
    Fixture f;
    UVToolsService svc({}, f.release(), f.fake_download(), nullptr);

    const fs::path in = f.root / "in.sl1", out = f.root / "out.ctb";
    { std::ofstream(in.string()) << "sliced-data"; }

    UVToolsTarget t; t.encoder_name = "ChituboxFile"; t.version = 4;
    auto r = svc.ensure_and_convert(in.string(), out.string(), t, nullptr);
    REQUIRE(r.ok());
    REQUIRE(fs::exists(out));
    std::ifstream o(out.string()); std::string body; std::getline(o, body);
    CHECK(body == "sliced-data");
}

TEST_CASE("UVtools acquire fails closed on checksum mismatch", "[UVtools][service]")
{
    Fixture f;
    UVToolsService svc({}, f.release(std::string(64, '0')), f.fake_download(), nullptr);

    auto r = svc.ensure_available(nullptr);
    CHECK(r.status == ConvertResult::VerifyFailed);
    // The unverified archive must never have been extracted or executed.
    CHECK_FALSE(fs::exists(f.install));
}

TEST_CASE("UVtools acquire reports a failed download and a missing tool", "[UVtools][service]")
{
    Fixture f;
    DownloadFn failing = [](const std::string &, const std::string &, const ProgressFn &, std::string &err) {
        err = "network down"; return false;
    };
    UVToolsService svc({}, f.release(), failing, nullptr);
    auto r = svc.ensure_available(nullptr);
    CHECK(r.status == ConvertResult::ToolMissing);
    CHECK(r.message.find("network down") != std::string::npos);

    // No download function at all (e.g. CLI): clear, actionable message, no crash.
    UVToolsService none({}, f.release(), nullptr, nullptr);
    auto r2 = none.ensure_available(nullptr);
    CHECK(r2.status == ConvertResult::ToolMissing);
    CHECK_FALSE(r2.message.empty());
}

#endif // _WIN32
