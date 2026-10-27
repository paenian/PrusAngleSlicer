///|/
///|/ PrusAngleSlicer SLA 3rd-party printer support
///|/ Released under the terms of the AGPLv3 or higher
///|/
#include "SLAConvertArchiveWriter.hpp"

#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>

#include "libslic3r/PrintConfig.hpp"
#include "libslic3r/SLAPrint.hpp"

namespace Slic3r {

// Boundary-injected statics (set by the GUI/CLI entry; see U5/U1 seam).
namespace {
    uvtools::UVToolsPaths                      g_paths;
    uvtools::UVToolsRelease                    g_release;
    uvtools::DownloadFn                        g_download;
    uvtools::PersistPathFn                     g_persist;
    uvtools::ProgressFn                        g_progress;
    std::function<void(const std::string &)>   g_warn;
}

void SLAConvertArchiveWriter::set_paths(uvtools::UVToolsPaths paths)          { g_paths = std::move(paths); }
void SLAConvertArchiveWriter::set_download_fn(uvtools::DownloadFn fn)          { g_download = std::move(fn); }
void SLAConvertArchiveWriter::set_persist_fn(uvtools::PersistPathFn fn)        { g_persist = std::move(fn); }
void SLAConvertArchiveWriter::set_release(uvtools::UVToolsRelease release)     { g_release = std::move(release); }
void SLAConvertArchiveWriter::set_warning_fn(std::function<void(const std::string &)> fn) { g_warn = std::move(fn); }
void SLAConvertArchiveWriter::set_progress_fn(uvtools::ProgressFn fn)          { g_progress = std::move(fn); }

namespace {
void emit_warning(const std::string &msg)
{
    BOOST_LOG_TRIVIAL(warning) << "[UVtools convert] " << msg;
    if (g_warn) g_warn(msg);
}
} // namespace

void SLAConvertArchiveWriter::export_print(const std::string     fname,
                                           const SLAPrint       &print,
                                           const ThumbnailsList &thumbnails,
                                           const std::string    &projectname)
{
    namespace fs = boost::filesystem;

    // Determine the UVtools target from the printer's sla_archive_format.
    const std::string archive_format = print.printer_config().sla_archive_format.value;
    auto target = uvtools::lookup(archive_format);
    if (! target) {
        // Should not happen (registry only routes well-formed uvtools:* here),
        // but fail safe: write a plain .sl1 so the user still gets output.
        emit_warning("Invalid UVtools target format '" + archive_format +
                     "'. Wrote an .sl1 archive instead.");
        SL1Archive::export_print(fname, print, thumbnails, projectname);
        return;
    }

    BOOST_LOG_TRIVIAL(info) << "[UVtools convert] Export requested: format='" << archive_format
                            << "' encoder='" << target->encoder_name << "' -> '" << fname << "'";

    // 1. Write the intermediate .sl1 next to the requested output.
    fs::path out_path(fname);
    fs::path sl1_path = out_path;
    sl1_path.replace_extension("sl1");
    // Avoid clobbering the final output if it happened to be named .sl1.
    if (sl1_path == out_path)
        sl1_path = out_path.string() + ".intermediate.sl1";

    try {
        SL1Archive::export_print(sl1_path.string(), print, thumbnails, projectname);
        BOOST_LOG_TRIVIAL(info) << "[UVtools convert] Intermediate .sl1 written: " << sl1_path.string();
    } catch (const std::exception &e) {
        // If even the .sl1 can't be written, this is a genuine export error.
        BOOST_LOG_TRIVIAL(error) << "[UVtools convert] Failed to write intermediate .sl1: " << e.what();
        throw;
    }

    // 2. Convert via UVtools.
    uvtools::UVToolsService service(g_paths, g_release, g_download, g_persist);
    uvtools::ProgressFn progress = g_progress;

    uvtools::ConvertResult res =
        service.ensure_and_convert(sl1_path.string(), fname, *target, progress);

    if (res.ok()) {
        // Success: remove the intermediate, leave only the manufacturer file.
        boost::system::error_code ec;
        fs::remove(sl1_path, ec);
        return;
    }

    // 3. Fallback (US-7): keep the .sl1, warn, never hard-fail.
    //    Rename the intermediate to the user's chosen location (with .sl1 ext)
    //    so they get a usable file where they expected output.
    fs::path fallback = out_path;
    fallback.replace_extension("sl1");
    boost::system::error_code ec;
    if (fallback != sl1_path) {
        fs::rename(sl1_path, fallback, ec);
        if (ec) fallback = sl1_path; // keep wherever it landed
    }

    std::string why = res.message.empty() ? "UVtools conversion did not run." : res.message;
    emit_warning("Could not produce '" + target->encoder_name + "' file for '" +
                 archive_format + "'. " + why +
                 "\nAn .sl1 archive was kept at: " + fallback.string());
}

} // namespace Slic3r
