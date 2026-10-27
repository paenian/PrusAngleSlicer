# Component Methods — 3rd-Party SLA Printer Support

High-level method signatures and I/O. **Business rules are deferred to Functional Design** (per unit). Signatures are indicative C++-style; final names/types settle during code generation.

---

## C1 — SLAConvertArchiveWriter
```
// Registered for "uvtools:*" ids in SLAArchiveFormatRegistry.
class SLAConvertArchiveWriter : public SLAArchiveWriter {
  // Build intermediate .sl1 rasters (reuse SL1 encoding).
  std::unique_ptr<sla::RasterBase> create_raster() const override;
  sla::RasterEncoder get_encoder() const override;

  // Orchestrates: write .sl1 -> map format -> convert -> finalize/fallback.
  void export_print(const std::string fname,
                    const SLAPrint& print,
                    const ThumbnailsList& thumbnails,
                    const std::string& projectname) override;
};
```
- `export_print`: writes intermediate `.sl1`; resolves target via `SLAFormatMap::lookup`; calls `UVToolsService::convert`; on success removes intermediate and leaves final file; on failure leaves `.sl1` and reports a non-fatal warning. I/O: input = sliced print + target path; output = manufacturer file or `.sl1` + warning.

## C2 — UVToolsService
```
struct ConvertResult {
  enum Status { Success, ToolMissing, VerifyFailed, ConvertFailed, Timeout } status;
  std::string message;        // human-readable, safe to show
  std::string output_path;    // populated on Success
};

class UVToolsService {
  // Resolve which executable to use: explicit override -> cached -> none.
  std::optional<std::string> resolve_executable(const UVToolsPaths& paths) const;

  // Ensure UVtools is present+verified; may download. Reports progress.
  // Returns resolved executable path or an error status.
  ConvertResult ensure_available(const UVToolsPaths& paths,
                                 const ProgressFn& progress);

  // Convert an intermediate .sl1 into dst_path in the given UVtools format.
  // Uses argument-array invocation (injection-safe) + timeout.
  ConvertResult convert(const std::string& src_sl1,
                        const std::string& dst_path,
                        const std::string& uvtools_format_id,
                        const ConvertOptions& opts);
};
```
- `UVToolsPaths`: carries the override path and cached path (populated from the app-config seam; libslic3r does not read GUI AppConfig directly).
- `ensure_available`: download (HTTPS) + integrity verify + cache; progress via callback.
- `convert`: builds `UVtools.Cmd convert` argument vector; captures exit code/stderr; maps to `ConvertResult`.

## C3 — SLAFormatMap
```
struct UVToolsTarget { std::string uvtools_id; std::string extension; std::string description; };

namespace SLAFormatMap {
  bool is_convert_target(const std::string& archive_format);           // true for "uvtools:*"
  std::optional<UVToolsTarget> lookup(const std::string& archive_format);
  std::vector<std::pair<std::string, UVToolsTarget>> supported_targets();
}
```
- Pure, total over the supported set, side-effect free. PBT targets (NFR-4): `is_convert_target(k) == lookup(k).has_value()`; every `supported_targets()` entry round-trips through `lookup`.

## C4 — Config / App-config additions
```
// Global app-config (Q3=B): one UVtools install for the whole app.
//   uvtools_executable_path   (user override; empty = auto)
//   uvtools_cached_path       (remembered after download/detect)
// sla_archive_format (existing SLAPrinterConfig key) may hold "uvtools:<id>".
```
- No new per-printer key for the UVtools path. Accessor/seam converts these into a `UVToolsPaths` passed to C2.

## C6 — GUI Settings/Status
```
// SLA printer settings tab: a field bound to the global uvtools_executable_path.
// Export flow: surface progress + the non-fatal fallback warning via the
// existing SLA export status/error channel (no new modal).
```
- No conversion logic in GUI; it reads/writes the global config and shows status.

---

## Method → Story/Requirement trace
| Method | Stories | Requirements |
|---|---|---|
| SLAConvertArchiveWriter::export_print | US-3, US-7 | FR-3, FR-6, NFR-1 |
| UVToolsService::ensure_available | US-5 | FR-4, NFR-2 (SECURITY-10/13), NFR-3 |
| UVToolsService::convert | US-3, US-7 | FR-3, NFR-2 (SECURITY-05/15) |
| UVToolsService::resolve_executable | US-6 | FR-4 |
| SLAFormatMap::* | US-1, US-2, US-3 | FR-5, NFR-4 (PBT) |
| Config additions | US-2, US-6 | FR-4, FR-5 |
| GUI settings/status | US-6, US-7 | FR-7 |
