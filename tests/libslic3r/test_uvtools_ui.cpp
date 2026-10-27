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

        // Config Wizard discovery gate: vendor must declare repo_id = non-prusa-sla
        // (otherwise it is silently dropped from the Add-printer wizard).
        REQUIRE(ini.count("vendor") == 1);
        CHECK(ini["vendor"].count("repo_id") == 1);
        CHECK(ini["vendor"]["repo_id"] == "non-prusa-sla");
        CHECK(ini["vendor"].count("config_version") == 1);

        // Each printer_model must declare SLA technology + a default material.
        int models = 0;
        for (const auto &sec : ini) {
            if (sec.first.rfind("printer_model:", 0) != 0) continue;
            ++models;
            CHECK(sec.second.count("technology") == 1);
            CHECK(sec.second.at("technology") == "SLA");
            CHECK(sec.second.count("default_materials") == 1);
            CHECK_FALSE(sec.second.at("default_materials").empty());
        }
        CHECK(models > 0);

        // The default material each model names must exist as an [sla_material:*] section.
        for (const auto &sec : ini) {
            if (sec.first.rfind("printer_model:", 0) != 0) continue;
            const std::string mat = sec.second.at("default_materials");
            CHECK(ini.count("sla_material:" + mat) == 1);
        }

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

TEST_CASE("Bundled 3rd-party SLA profiles ship model thumbnails", "[AngledSlicing][UVtools][ui]")
{
    // The Config Wizard shows <VendorId>/<model id>_thumbnail.png; a real PNG is
    // required or the wizard falls back to a generic placeholder.
    REQUIRE_FALSE(profiles_dir().empty());

    struct T { const char *vendor_dir; const char *model_id; };
    const std::vector<T> thumbs = {
        { "PhrozenSLA", "SONIC MEGA 8K S" },
        { "PhrozenSLA", "SONIC MINI 8K" },
        { "ElegooSLA",  "SATURN 4 ULTRA" },
        { "ElegooSLA",  "MARS 4" },
    };
    for (const auto &t : thumbs) {
        const std::string png = profiles_dir() + "/" + t.vendor_dir + "/" + t.model_id + "_thumbnail.png";
        INFO("thumbnail: " << png);
        std::ifstream in(png, std::ios::binary);
        REQUIRE(in.good());
        // PNG magic.
        unsigned char sig[8] = {0};
        in.read(reinterpret_cast<char*>(sig), 8);
        const unsigned char png_magic[8] = {0x89,'P','N','G','\r','\n',0x1a,'\n'};
        for (int i = 0; i < 8; ++i) CHECK(sig[i] == png_magic[i]);
    }
}

TEST_CASE("Phrozen Sonic Mega 8K S profile has correct 8K specs", "[AngledSlicing][UVtools][ui]")
{
    // Guards the first end-to-end target against spec drift.
    REQUIRE_FALSE(profiles_dir().empty());
    auto ini = read_ini(profiles_dir() + "/PhrozenSLA.ini");
    REQUIRE(ini.count("printer:Phrozen Sonic Mega 8K S") == 1);
    const auto &p = ini.at("printer:Phrozen Sonic Mega 8K S");
    CHECK(p.at("printer_technology") == "SLA");
    CHECK(p.at("display_pixels_x") == "7680");
    CHECK(p.at("display_pixels_y") == "4320");
    CHECK(p.at("display_width") == "330");
    CHECK(p.at("display_height") == "185");
    CHECK(p.at("max_print_height") == "300");
    CHECK(p.at("sla_archive_format") == "uvtools:ChituboxFile@4");
    CHECK(p.at("default_sla_material_profile") == "Anycubic ABS-Like Grey Resin @PHROZEN 0.05");
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
