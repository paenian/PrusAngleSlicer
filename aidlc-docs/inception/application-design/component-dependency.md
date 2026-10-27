# Component Dependencies & Data Flow — 3rd-Party SLA Printer Support

## Dependency matrix
| Component | Depends on | Depended on by | Communication |
|---|---|---|---|
| C1 SLAConvertArchiveWriter | C2 UVToolsService, C3 SLAFormatMap, existing SL1Archive, SLAArchiveFormatRegistry | SLAPrint (via registry/factory) | Direct in-process calls |
| C2 UVToolsService | app-config seam (paths), HTTPS download, OS process spawn | C1 | Direct calls; subprocess; network |
| C3 SLAFormatMap | — (pure) | C1 | Direct calls |
| C4 Config/App-config | existing config system | C1 (reads archive_format), C2 (paths), C6 | Config reads |
| C5 Bundled profiles | — (data) | Preset system | INI data |
| C6 GUI settings/status | C4 (global path), existing status channel | user | GUI bindings |
| Core slicing (SLAPrintSteps, RasterBase) | — | SL1 path used by C1 | UNCHANGED |

## Key seam — libslic3r ↔ app config (from Q3=B)
The UVtools override + cached path are **global app settings**, but `UVToolsService` lives in libslic3r, which must not depend on the GUI's `AppConfig`. Resolution:
- A small `UVToolsPaths { override_path, cached_path }` struct is populated at the boundary (GUI or CLI entry) and **passed into** `UVToolsService`.
- Persisting the cached path after download goes back through a callback/config-writer injected at the same boundary.
- This keeps libslic3r GUI-agnostic while honoring the global-setting decision.

## Data flow (export → convert)
```
sla_archive_format = "uvtools:ctb"            (from selected printer profile, C5/C4)
        |
        v
SLAPrint.export_print  --(registry: create "uvtools:*")-->  C1 SLAConvertArchiveWriter
        |
        | 1. rasterize + write intermediate .sl1   (reuses existing SL1 path; core slicing unchanged)
        |
        | 2. C3 SLAFormatMap.lookup("uvtools:ctb") -> { uvtools_id="ctb", ext="ctb" }
        |
        | 3. C2 UVToolsService.ensure_available(paths, progress)
        |        -> override? cached? else download(HTTPS)+verify -> cache path
        |
        | 4. C2 UVToolsService.convert(intermediate.sl1, output.ctb, "ctb", opts)
        |        -> UVtools.Cmd convert <args...>  (argument array, injection-safe, timeout)
        |
        v
   success: output.ctb at user path, intermediate removed
   failure: intermediate .sl1 kept + non-fatal warning (status channel)
```

## Communication patterns
- All in-process synchronous calls on the existing background slicing/export thread.
- External interactions: one HTTPS download (acquisition) and one subprocess invocation (convert), both owned by C2 and both guarded (verify, timeout, typed errors).
- No new shared mutable state beyond the cached-path persistence (idempotent).

## Isolation / minimal-footprint check (NFR-1, CON-2)
- New code: C1, C2, C3 (new files under `src/libslic3r/Format/` + `Format/UVTools/`).
- Modified (additive only): registry entry (one line in `SLAArchiveFormatRegistry.cpp`), config entries (C4), GUI field + status (C6), `CMakeLists.txt`.
- Untouched: `SLAPrintSteps.cpp`, `RasterBase`/`AGGRaster`, existing writers (SL1/SVG/Anycubic) remain byte-compatible.
