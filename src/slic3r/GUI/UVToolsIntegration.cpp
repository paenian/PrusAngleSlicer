///|/
///|/ PrusAngleSlicer SLA 3rd-party printer support
///|/ Released under the terms of the AGPLv3 or higher
///|/
#include "UVToolsIntegration.hpp"

#include <fstream>
#include <string>

#include <boost/log/trivial.hpp>

#include "GUI_App.hpp"
#include "libslic3r/AppConfig.hpp"
#include "libslic3r/Format/UVTools/SLAConvertArchiveWriter.hpp"
#include "libslic3r/Format/UVTools/UVToolsService.hpp"
#include "slic3r/Utils/Http.hpp"

namespace Slic3r { namespace GUI {

// --- Pinned UVtools release (U1 data). Update version+sha256 together. ---
// NOTE: placeholder checksums; the exact pinned version + per-arch SHA-256 are
// finalized during Build & Test against the chosen UVtools release. Until a
// real checksum is provided, auto-download verification will (correctly) fail
// closed and the writer falls back to .sl1 with a warning.
namespace {
constexpr const char *UVTOOLS_PINNED_VERSION = "5.1.6";

uvtools::UVToolsRelease pinned_release_for_current_arch()
{
    uvtools::UVToolsRelease rel;
    rel.version = UVTOOLS_PINNED_VERSION;
#if defined(__APPLE__)
    #if defined(__aarch64__)
        const char *arch = "arm64";
        rel.sha256 = ""; // TODO: pin arm64 checksum
    #else
        const char *arch = "x64";
        rel.sha256 = ""; // TODO: pin x64 checksum
    #endif
    rel.url = std::string("https://github.com/sn4k3/UVtools/releases/download/v")
            + UVTOOLS_PINNED_VERSION + "/UVtools_osx-" + arch + "_v"
            + UVTOOLS_PINNED_VERSION + ".zip";
#else
    // Non-macOS: left unset this pass (macOS-first per CON-3). Service will
    // report ToolMissing -> fallback to .sl1.
    rel.url = "";
    rel.sha256 = "";
#endif
    return rel;
}

uvtools::UVToolsPaths paths_from_appconfig()
{
    uvtools::UVToolsPaths p;
    AppConfig *cfg = wxGetApp().app_config;
    if (cfg) {
        if (cfg->has("", "uvtools_executable_path"))
            p.override_path = cfg->get("", "uvtools_executable_path");
        if (cfg->has("", "uvtools_cached_path"))
            p.cached_path = cfg->get("", "uvtools_cached_path");
    }
    return p;
}

// Http-backed download: fetch url -> dest file. Reports coarse progress.
bool download_file(const std::string &url, const std::string &dest,
                   const uvtools::ProgressFn &progress, std::string &error_out)
{
    bool ok = false;
    std::string body;
    Http::get(url)
        .size_limit(0) // default large limit; UVtools zips are tens of MB
        .on_progress([&progress](Http::Progress p, bool &cancel) {
            if (progress && p.dltotal > 0) {
                double frac = double(p.dlnow) / double(p.dltotal);
                if (! progress(frac * 0.85, "Downloading UVtools..."))
                    cancel = true;
            }
        })
        .on_complete([&](std::string data, unsigned /*status*/) {
            std::ofstream out(dest, std::ios::binary | std::ios::trunc);
            out.write(data.data(), std::streamsize(data.size()));
            ok = out.good();
        })
        .on_error([&](std::string /*body*/, std::string err, unsigned status) {
            error_out = err.empty() ? ("HTTP status " + std::to_string(status)) : err;
            ok = false;
        })
        .perform_sync();
    return ok;
}

void persist_cached_path(const std::string &resolved)
{
    if (AppConfig *cfg = wxGetApp().app_config) {
        cfg->set("", "uvtools_cached_path", resolved);
        cfg->save();
    }
}

void show_warning(const std::string &msg)
{
    // Non-fatal: log and surface through the standard notification/log path.
    BOOST_LOG_TRIVIAL(warning) << "[UVtools] " << msg;
    // A richer GUI notification can be added; logging keeps it non-blocking.
}

} // namespace

void init_uvtools_integration()
{
    SLAConvertArchiveWriter::set_paths(paths_from_appconfig());
    SLAConvertArchiveWriter::set_release(pinned_release_for_current_arch());
    SLAConvertArchiveWriter::set_download_fn(&download_file);
    SLAConvertArchiveWriter::set_persist_fn(&persist_cached_path);
    SLAConvertArchiveWriter::set_warning_fn(&show_warning);
}

}} // namespace Slic3r::GUI
