///|/
///|/ PrusAngleSlicer SLA 3rd-party printer support
///|/ Released under the terms of the AGPLv3 or higher
///|/
#ifndef UV_TOOLS_SERVICE_HPP
#define UV_TOOLS_SERVICE_HPP

#include <string>
#include <functional>
#include <optional>

#include "SLAFormatMap.hpp"

namespace Slic3r { namespace uvtools {

// Carries the global UVtools executable location into libslic3r without a GUI
// dependency (the GUI/CLI boundary populates this). See U1 design (Q3=B seam).
struct UVToolsPaths {
    std::string override_path; // user-specified executable (may be empty)
    std::string cached_path;   // remembered after a successful acquire (may be empty)
};

// Outcome of a conversion attempt. The convert writer uses this to decide
// success vs. fallback (US-7).
struct ConvertResult {
    enum Status { Success, ToolMissing, VerifyFailed, ConvertFailed, Timeout } status = ConvertFailed;
    std::string message;     // safe-to-show, human-readable
    std::string output_path; // populated on Success
    bool ok() const { return status == Success; }
};

// Progress reporting callback: (fraction 0..1, message). Return false to cancel.
using ProgressFn = std::function<bool(double, const std::string &)>;

// Injectable download function so libslic3r stays GUI-agnostic. The boundary
// (GUI/CLI) supplies an implementation (e.g. backed by slic3r::Http).
// Downloads `url` to local file `dest`; reports progress; returns true on success.
using DownloadFn = std::function<bool(const std::string &url,
                                      const std::string &dest,
                                      const ProgressFn &progress,
                                      std::string &error_out)>;

// Callback invoked with a newly-resolved executable path so the boundary can
// persist it to the global app config (cached_path).
using PersistPathFn = std::function<void(const std::string &resolved_path)>;

// Pinned UVtools release descriptor for the current platform/arch (from U1 data).
struct UVToolsRelease {
    std::string version;      // e.g. "5.0.0"
    std::string url;          // full download URL for this arch
    std::string sha256;       // expected SHA-256 (lowercase hex) of the downloaded archive
    bool valid() const { return !version.empty() && !url.empty() && !sha256.empty(); }
};

class UVToolsService {
public:
    UVToolsService(UVToolsPaths paths,
                   UVToolsRelease release,
                   DownloadFn download = nullptr,
                   PersistPathFn persist = nullptr)
        : m_paths(std::move(paths))
        , m_release(std::move(release))
        , m_download(std::move(download))
        , m_persist(std::move(persist))
    {}

    // Resolve the executable to use: override -> cached -> (none).
    // Does NOT download. Returns the path if an existing executable is found.
    std::optional<std::string> resolve_executable() const;

    // Ensure a verified UVtools executable is available, downloading+verifying
    // if necessary. On success, m_resolved is set and persisted via PersistPathFn.
    // Returns a ConvertResult whose status is Success (available) or an error.
    ConvertResult ensure_available(const ProgressFn &progress);

    // Convert src_sl1 -> dst_path using the given UVtools target (strict encoder
    // name + optional version). Uses argument-vector invocation (no shell).
    ConvertResult convert(const std::string &src_sl1,
                          const std::string &dst_path,
                          const UVToolsTarget &target,
                          unsigned int timeout_seconds = 300);

    // Convenience: ensure_available + convert.
    ConvertResult ensure_and_convert(const std::string &src_sl1,
                                     const std::string &dst_path,
                                     const UVToolsTarget &target,
                                     const ProgressFn &progress,
                                     unsigned int timeout_seconds = 300);

    const std::string &resolved_executable() const { return m_resolved; }

    // Exposed for testing: lowercase-hex SHA-256 of a file's contents.
    static std::optional<std::string> sha256_file(const std::string &path);

private:
    UVToolsPaths   m_paths;
    UVToolsRelease m_release;
    DownloadFn     m_download;
    PersistPathFn  m_persist;
    std::string    m_resolved; // resolved executable after ensure_available
};

// Build the UVtools convert argument vector (exposed for testing / PBT).
// Returns: { "convert", src, encoder_name, dst, ["-v", version], "--no-overwrite" }
std::vector<std::string> build_convert_args(const std::string &src_sl1,
                                            const std::string &dst_path,
                                            const UVToolsTarget &target,
                                            bool no_overwrite = true);

}} // namespace Slic3r::uvtools

#endif // UV_TOOLS_SERVICE_HPP
