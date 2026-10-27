///|/
///|/ PrusAngleSlicer SLA 3rd-party printer support
///|/ Released under the terms of the AGPLv3 or higher
///|/
#ifndef SLA_CONVERT_ARCHIVE_WRITER_HPP
#define SLA_CONVERT_ARCHIVE_WRITER_HPP

#include "libslic3r/Format/SL1.hpp"
#include "UVToolsService.hpp"
#include "SLAFormatMap.hpp"

namespace Slic3r {

// A Strategy-A registry writer: produce the intermediate .sl1 (reusing the
// SL1Archive rasterization/encoding), then convert it to a manufacturer format
// via UVtools. On any failure it leaves the .sl1 in place and reports a
// non-fatal warning (US-7) rather than hard-failing.
//
// Registered in SLAArchiveFormatRegistry for "uvtools:*" ids.
class SLAConvertArchiveWriter : public SL1Archive {
public:
    SLAConvertArchiveWriter() = default;
    explicit SLAConvertArchiveWriter(const SLAPrinterConfig &cfg) : SL1Archive(cfg) {}

    void export_print(const std::string     fname,
                      const SLAPrint       &print,
                      const ThumbnailsList &thumbnails,
                      const std::string    &projectname = "") override;

    // Boundary-injected dependencies (GUI/CLI populate these so libslic3r stays
    // GUI-agnostic). If unset, acquisition/warnings degrade gracefully.
    static void set_paths(uvtools::UVToolsPaths paths);
    static void set_download_fn(uvtools::DownloadFn fn);
    static void set_persist_fn(uvtools::PersistPathFn fn);
    static void set_release(uvtools::UVToolsRelease release);
    static void set_warning_fn(std::function<void(const std::string &)> fn);
    static void set_progress_fn(uvtools::ProgressFn fn);
};

} // namespace Slic3r

#endif // SLA_CONVERT_ARCHIVE_WRITER_HPP
