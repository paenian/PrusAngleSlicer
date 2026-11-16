///|/
///|/ PrusAngleSlicer SLA 3rd-party printer support
///|/ Released under the terms of the AGPLv3 or higher
///|/
// Loads the bundled 3rd-party SLA vendor files with PrusaSlicer's OWN profile
// loader (PresetBundle::load_configbundle, the same call the app uses for system
// vendor bundles), so inheritance, compatibility conditions and the vendor model
// table are evaluated for real -- not just text-matched.
//
// Also checks the settings UVtools actually consumes from the notes: UVtools reads
// "<Keyword>_<value>" lines (LiftHeight_6, FILEFORMAT_prz, ...) from the material
// and printer notes of the .sl1. Notes in any other spelling are silently ignored
// and UVtools falls back to its own defaults.

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <fstream>
#include <string>

#include <boost/filesystem.hpp>

#include "libslic3r/Preset.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/Format/UVTools/SLAFormatMap.hpp"

using namespace Slic3r;

namespace {

std::string profiles_dir() { return std::string(PROFILES_DIR); }

// Load one vendor .ini through the real loader.
struct Loaded {
    PresetBundle bundle;
    size_t       count = 0;
    explicit Loaded(const std::string &file)
    {
        auto res = bundle.load_configbundle(profiles_dir() + "/" + file, PresetBundle::LoadSystem,
                                            ForwardCompatibilitySubstitutionRule::Disable);
        count = res.second;
    }
};

bool has_text(const std::string &hay, const std::string &needle) { return hay.find(needle) != std::string::npos; }

} // namespace

TEST_CASE("Phrozen vendor bundle loads and the Mega 8K S matches the spec brief", "[UVtools][profiles]")
{
    Loaded l("PhrozenSLA.ini");
    REQUIRE(l.count > 0);

    const Preset *printer = l.bundle.printers.find_preset("Phrozen Sonic Mega 8K S", false, false);
    REQUIRE(printer != nullptr);
    const DynamicPrintConfig &c = printer->config;

    CHECK(printer->printer_technology() == ptSLA);

    // Brief: build volume 33 x 18.5 x 30 cm.
    CHECK(c.opt_float("max_print_height") == 300.);
    CHECK(c.opt_float("display_width")  == 330.);
    CHECK(c.opt_float("display_height") == 185.);

    // Brief: 15" 8K mono LCD, 43 um XY resolution. 8K = 7680 x 4320 and the pixel
    // pitch must come out at ~43 um on both axes.
    const int px = c.opt_int("display_pixels_x"), py = c.opt_int("display_pixels_y");
    CHECK(px == 7680);
    CHECK(py == 4320);
    CHECK(c.opt_float("display_width")  / px * 1000. == Catch::Approx(43.).margin(0.5));
    CHECK(c.opt_float("display_height") / py * 1000. == Catch::Approx(43.).margin(0.5));

    // Output: UVtools' GooFile writing Phrozen's .prz.
    const std::string fmt = c.opt_string("sla_archive_format");
    CHECK(fmt == "uvtools:GooFile.prz");
    auto target = uvtools::lookup(fmt);
    REQUIRE(target.has_value());
    CHECK(target->encoder_name == "GooFile");
    CHECK(target->extension == "prz");

    // Self-describing fallback .sl1: UVtools "convert ... auto" reads FILEFORMAT_<ext>.
    CHECK(has_text(c.opt_string("printer_notes"), "FILEFORMAT_prz"));
}

TEST_CASE("Phrozen default material and layer-height presets resolve and are compatible", "[UVtools][profiles]")
{
    Loaded l("PhrozenSLA.ini");
    REQUIRE(l.count > 0);

    const Preset *printer = l.bundle.printers.find_preset("Phrozen Sonic Mega 8K S", false, false);
    REQUIRE(printer != nullptr);
    const PresetWithVendorProfile printer_v = l.bundle.printers.get_preset_with_vendor_profile(*printer);

    // The printer's default material exists, is Anycubic's grey ABS-like resin, and is
    // compatible with this printer.
    const std::string mat_name = printer->config.opt_string("default_sla_material_profile");
    CHECK(mat_name == "Anycubic ABS-Like Grey Resin @PHROZEN 0.05");
    const Preset *material = l.bundle.sla_materials.find_preset(mat_name, false, false);
    REQUIRE(material != nullptr);
    CHECK(material->config.opt_string("material_vendor") == "Anycubic");
    CHECK(material->config.opt_string("material_colour") == "#808080");
    const PresetWithVendorProfile material_v = l.bundle.sla_materials.get_preset_with_vendor_profile(*material);
    CHECK(is_compatible_with_printer(material_v, printer_v));

    // Brief: layer thickness 0.01 - 0.30 mm. The default print preset exists, and the
    // material is usable across the offered layer heights (not just 0.05).
    CHECK(l.bundle.sla_prints.find_preset(printer->config.opt_string("default_sla_print_profile"), false, false) != nullptr);
    for (const char *name : { "0.025 Fine @PHROZEN", "0.05 Normal @PHROZEN", "0.1 Draft @PHROZEN" }) {
        INFO("print preset: " << name);
        const Preset *print = l.bundle.sla_prints.find_preset(name, false, false);
        REQUIRE(print != nullptr);
        const double lh = print->config.opt_float("layer_height");
        CHECK(lh >= 0.01);
        CHECK(lh <= 0.30);
        CHECK(is_compatible_with_printer(l.bundle.sla_prints.get_preset_with_vendor_profile(*print), printer_v));
        CHECK(is_compatible_with_print(material_v, l.bundle.sla_prints.get_preset_with_vendor_profile(*print), printer_v));
    }
}

TEST_CASE("Phrozen/Elegoo material notes use the keywords UVtools actually reads", "[UVtools][profiles]")
{
    for (const char *file : { "PhrozenSLA.ini", "ElegooSLA.ini" }) {
        INFO("vendor file: " << file);
        Loaded l(file);
        REQUIRE(l.count > 0);

        bool checked = false;
        for (const Preset &m : l.bundle.sla_materials) {
            if (! m.is_system || m.name.find("Anycubic ABS-Like Grey Resin") == std::string::npos)
                continue;
            checked = true;
            const std::string notes = m.config.opt_string("material_notes");
            INFO("material: " << m.name);
            // Lift / retract that UVtools writes into the printer file (mm, mm/min).
            for (const char *kw : { "LiftHeight_", "LiftSpeed_", "RetractSpeed_",
                                    "BottomLiftHeight_", "BottomLiftSpeed_", "BottomRetractSpeed_" })
                CHECK(has_text(notes, kw));
            // The Anycubic-writer spellings are ignored by UVtools (it then silently
            // uses its own defaults), so they must not be what we rely on.
            CHECK_FALSE(has_text(notes, "LIFT_DISTANCE"));
            CHECK_FALSE(has_text(notes, "LIFT_SPEED"));
        }
        CHECK(checked);
    }
}

TEST_CASE("Vendor config_version matches the newest idx entry (so installed copies get updated)", "[UVtools][profiles]")
{
    // PrusaSlicer only refreshes an installed vendor file when the idx shipped in
    // resources recommends a version that equals the .ini's config_version and is
    // newer than the installed one. A mismatch means edits never reach existing
    // users ("Corrupted installation?" in the log), so keep them in lockstep.
    for (const char *stem : { "PhrozenSLA", "ElegooSLA" }) {
        INFO("vendor: " << stem);
        Loaded l(std::string(stem) + ".ini");
        REQUIRE(l.count > 0);

        const Preset *any = nullptr;
        for (const Preset &p : l.bundle.printers)
            if (p.is_system && p.vendor != nullptr && p.vendor->id == stem) { any = &p; break; }
        REQUIRE(any != nullptr);

        // Newest entry = first "<version> <description>" line of the idx.
        std::ifstream idx(profiles_dir() + "/" + stem + ".idx");
        REQUIRE(idx.good());
        std::string line, newest;
        while (std::getline(idx, line)) {
            if (line.empty() || line.rfind("min_slic3r_version", 0) == 0 || line.rfind("max_slic3r_version", 0) == 0)
                continue;
            newest = line.substr(0, line.find(' '));
            break;
        }
        CHECK(newest == any->vendor->config_version.to_string());
    }
}

TEST_CASE("Vendor model tables: wizard picture and default material exist", "[UVtools][profiles]")
{
    struct M { const char *file; const char *vendor_dir; const char *model_id; };
    for (const M &m : { M{ "PhrozenSLA.ini", "PhrozenSLA", "SONIC MEGA 8K S" },
                        M{ "ElegooSLA.ini",  "ElegooSLA",  "SATURN 4 ULTRA" } }) {
        INFO("model: " << m.model_id);
        Loaded l(m.file);
        REQUIRE(l.count > 0);

        const Preset *p = nullptr;
        for (const Preset &pr : l.bundle.printers)
            if (pr.is_system && pr.config.opt_string("printer_model") == m.model_id) { p = &pr; break; }
        REQUIRE(p != nullptr);
        REQUIRE(p->vendor != nullptr);
        CHECK(p->vendor->repo_id == "non-prusa-sla");   // Config Wizard discovery gate

        const VendorProfile::PrinterModel *model = nullptr;
        for (const auto &pm : p->vendor->models)
            if (pm.id == m.model_id) { model = &pm; break; }
        REQUIRE(model != nullptr);
        CHECK(model->technology == ptSLA);
        REQUIRE_FALSE(model->default_materials.empty());
        CHECK(l.bundle.sla_materials.find_preset(model->default_materials.front(), false, false) != nullptr);
        // The picture the wizard will show.
        CHECK(boost::filesystem::exists(boost::filesystem::path(profiles_dir()) / m.vendor_dir / model->thumbnail));
    }
}
