///|/
///|/ PrusAngleSlicer SLA 3rd-party printer support
///|/ Released under the terms of the AGPLv3 or higher
///|/
// T6 — UI verification (data-level).
//
// PrusaSlicer's wxWidgets GUI has no automated rendering harness in-repo, and
// spinning up wx headlessly here is fragile. These tests instead verify the
// *data and wiring behind* the new UI surfaces, which is what actually breaks:
//   1. The bundled 3rd-party SLA printer profiles exist, declare SLA technology,
//      and set a well-formed uvtools:* archive format that the format map accepts
//      and maps to a real encoder + extension (this is what appears/selects in
//      the printer picker and drives export).
//   2. The global UVtools executable-path setting round-trips through AppConfig
//      (this is what the Preferences text field reads/writes).
// A complementary manual UI checklist lives in
//   aidlc-docs/construction/build-and-test/ui-verification-checklist.md

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>

#include "libslic3r/AppConfig.hpp"
#include "libslic3r/Format/UVTools/SLAFormatMap.hpp"

using namespace Slic3r;

namespace {

// Tiny INI section/key reader (sufficient for the vendor-profile assertions;
// avoids pulling in the full PresetBundle). Returns section -> key -> value.
std::map<std::string, std::map<std::string, std::string>> read_ini(const std::string &path)
{
    std::map<std::string, std::map<std::string, std::string>> out;
    std::ifstream in(path);
    std::string line, section;
    while (std::getline(in, line)) {
        // trim
        auto l = line.find_first_not_of(" \t\r\n");
        if (l == std::string::npos) continue;
        auto r = line.find_last_not_of(" \t\r\n");
        std::string s = line.substr(l, r - l + 1);
        if (s.empty() || s[0] == '#' || s[0] == ';') continue;
        if (s.front() == '[' && s.back() == ']') { section = s.substr(1, s.size() - 2); continue; }
        auto eq = s.find('=');
        if (eq == std::string::npos) continue;
        std::string key = s.substr(0, eq);
        std::string val = s.substr(eq + 1);
        auto tk = key.find_last_not_of(" \t"); if (tk != std::string::npos) key = key.substr(0, tk + 1);
        auto vk = val.find_first_not_of(" \t"); if (vk != std::string::npos) val = val.substr(vk);
        out[section][key] = val;
    }
    return out;
}

std::string profiles_dir()
{
#ifdef PROFILES_DIR
    return std::string(PROFILES_DIR);
#else
    return std::string();
#endif
}

} // namespace

TEST_CASE("Bundled 3rd-party SLA profiles expose valid uvtools targets", "[AngledSlicing][UVtools][ui]")
{
    REQUIRE_FALSE(profiles_dir().empty());

    struct Expect { const char *file; const char *vendor_substr; };
    const std::vector<Expect> vendors = {
        { "ElegooSLA.ini",  "Elegoo" },
        { "PhrozenSLA.ini", "Phrozen" },
    };

    for (const auto &e : vendors) {
        const std::string path = profiles_dir() + "/" + e.file;
        INFO("profile file: " << path);
        auto ini = read_ini(path);
        REQUIRE_FALSE(ini.empty());

        // At least one [printer:*] section with SLA technology + a uvtools:* format.
        int sla_printers = 0, uvtools_targets = 0;
        for (const auto &sec : ini) {
            if (sec.first.rfind("printer:", 0) != 0) continue;
            const auto &kv = sec.second;
            auto tech = kv.find("printer_technology");
            if (tech == kv.end() || tech->second != "SLA") continue;
            ++sla_printers;

            auto fmt = kv.find("sla_archive_format");
            REQUIRE(fmt != kv.end());
            const std::string &archive_format = fmt->second;
            // Every bundled 3rd-party SLA printer must target a uvtools:* format...
            CHECK(uvtools::is_convert_target(archive_format));
            // ...and that format must resolve to a real encoder + extension.
            auto t = uvtools::lookup(archive_format);
            REQUIRE(t.has_value());
            CHECK_FALSE(t->encoder_name.empty());
            CHECK_FALSE(t->extension.empty());
            ++uvtools_targets;

            // The printer must declare a build volume + display (what the UI shows).
            CHECK(kv.count("bed_shape") == 1);
            CHECK(kv.count("display_pixels_x") == 1);
            CHECK(kv.count("display_pixels_y") == 1);
            CHECK(kv.count("max_print_height") == 1);
        }
        CHECK(sla_printers > 0);
        CHECK(uvtools_targets == sla_printers);
    }
}

TEST_CASE("UVtools executable path round-trips through AppConfig", "[AngledSlicing][UVtools][ui]")
{
    // Mirrors what the Preferences 'Other' tab field reads/writes.
    AppConfig cfg(AppConfig::EAppMode::Editor);

    const std::string key = "uvtools_executable_path";
    CHECK_FALSE(cfg.has(key)); // not set by default

    const std::string p = "/opt/uvtools/UVtoolsCmd";
    cfg.set(key, p);
    CHECK(cfg.has(key));
    CHECK(cfg.get(key) == p);

    // Also the cached-path key used by auto-download persistence.
    const std::string cached = "/tmp/uvtools-7.1.2/UVtools.app/Contents/MacOS/UVtoolsCmd";
    cfg.set("uvtools_cached_path", cached);
    CHECK(cfg.get("uvtools_cached_path") == cached);
}
