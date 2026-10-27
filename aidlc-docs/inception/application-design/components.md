# Components — 3rd-Party SLA Printer Support

High-level component definitions. Detailed business rules are deferred to Functional Design (per unit).

Design decisions carried from the plan answers:
- Q1=A: UVtools logic is a self-contained module in libslic3r (`src/libslic3r/Format/UVTools/`), reusable by GUI and CLI.
- Q2=A: UVtools download is lazy — triggered on first export that needs it, progress surfaced via the existing slicing-status callback.
- Q3=B: The UVtools executable path is a **global application setting** (AppConfig), not per-printer. The resolved/remembered path is passed into the libslic3r service (libslic3r does not read GUI AppConfig directly — see seam note in component-dependency.md).
- Q4=A: Target format encoded in `sla_archive_format` with a `uvtools:` prefix (e.g. `uvtools:ctb`, `uvtools:goo`). Non-prefixed ids keep existing behavior.
- Q5=A: Fallback warning uses the existing SLA export status/error channel (non-fatal).

---

## C1 — SLAConvertArchiveWriter
- **Layer**: `src/libslic3r/Format/`
- **Type**: Concrete `SLAArchiveWriter` subclass, registered in `SLAArchiveFormatRegistry` for `uvtools:*` format ids.
- **Purpose**: Implement the Strategy-A conversion output path.
- **Responsibilities**:
  - Reuse the existing SL1 rasterization/encoding (compose or subclass `SL1Archive`) to produce an intermediate `.sl1`.
  - Resolve the target UVtools format + extension via `SLAFormatMap`.
  - Invoke `UVToolsService` to convert the intermediate into the manufacturer file at the final output path.
  - On success: ensure only the final file remains (remove intermediate).
  - On failure: leave the `.sl1` and raise a non-fatal warning (US-7) — never a hard crash.
- **Interface**: honors `SLAArchiveWriter::export_print(fname, print, thumbnails, projectname)` and `create_raster()/get_encoder()`.
- **Does NOT**: touch core slicing/rasterization; own download logic (delegates to C2).

## C2 — UVToolsService
- **Layer**: `src/libslic3r/Format/UVTools/`
- **Type**: Self-contained service module.
- **Purpose**: Own the entire relationship with the external UVtools tool.
- **Responsibilities**:
  - **Locate**: determine the UVtools executable to use — explicit override path (from global config, passed in) first, else a previously cached/remembered path, else trigger acquisition.
  - **Acquire**: download UVtools from its official source over HTTPS, verify integrity (checksum/signature) before use, cache the resolved path (persisted via the app-config seam), and report progress via a callback.
  - **Invoke**: run `UVtools.Cmd convert <intermediate.sl1> <output.ext> [format args]` using an argument array (never a shell string) to prevent injection; capture exit status + stderr; enforce a timeout.
  - Surface typed results (success / tool-missing / verify-failed / convert-failed) to the caller for fallback decisions.
- **Interface** (high-level): `resolve_executable(...)`, `ensure_available(progress_cb)`, `convert(src_sl1, dst_path, uvtools_format_id, opts) -> ConvertResult`.
- **Does NOT**: decide fallback UX (returns status; C1 decides); depend on GUI types.

## C3 — SLAFormatMap
- **Layer**: `src/libslic3r/Format/UVTools/`
- **Type**: Pure lookup/mapping utility.
- **Purpose**: Translate a profile's `sla_archive_format` value into (a) whether it is a UVtools-convert target, (b) the UVtools format identifier, and (c) the output file extension.
- **Responsibilities**:
  - Parse the `uvtools:<id>` convention; recognize multiple formats per manufacturer.
  - Provide the canonical UVtools format id + extension for each supported target.
  - Be total over the supported set and side-effect free (PBT target, NFR-4).
- **Interface**: `is_convert_target(archive_format) -> bool`, `lookup(archive_format) -> optional<{uvtools_id, extension, description}>`, `supported_targets() -> list`.

## C4 — SLA Config / App-config additions
- **Layer**: `src/libslic3r/PrintConfig.*` and the app-config seam.
- **Purpose**: Hold the global UVtools path override + remembered path (Q3=B), and allow `sla_archive_format` to carry `uvtools:*` ids.
- **Responsibilities**:
  - Define the global config entries for UVtools executable path (user override) and cached/remembered path.
  - No new per-printer key for the UVtools path (global per Q3=B). Target-format selection reuses `sla_archive_format`.
- **Note**: exact storage location (AppConfig key names, app-data dir) finalized in Functional/NFR design.

## C5 — Bundled Printer Profiles (data)
- **Layer**: `resources/profiles/`
- **Purpose**: Ship ready-made SLA profiles for Anycubic, Phrozen, Elegoo (US-1, FR-2).
- **Responsibilities**:
  - Per model: `printer_technology = SLA`, correct `bed_shape` / `max_print_height` / `display_*`, and `sla_archive_format = uvtools:<id>` for the model's native format.
  - No C++ changes required for the data itself.
- **Note**: final model list + specs finalized in Functional Design (spike item).

## C6 — GUI Settings/Status Surface (minimal)
- **Layer**: `src/slic3r/GUI/` (SLA printer settings tab + export status path)
- **Purpose**: Expose the global UVtools path override, show download/convert progress, and present the non-fatal fallback warning (US-6, US-7, FR-7).
- **Responsibilities**:
  - A settings field for the UVtools override path (global).
  - Progress indication during download/convert via existing status mechanisms.
  - Display the fallback warning through the existing SLA export status/error channel.
- **Does NOT**: contain conversion logic (delegates to libslic3r service).

## Component summary
| ID | Component | Layer | New/Modified |
|---|---|---|---|
| C1 | SLAConvertArchiveWriter | libslic3r/Format | New |
| C2 | UVToolsService | libslic3r/Format/UVTools | New |
| C3 | SLAFormatMap | libslic3r/Format/UVTools | New |
| C4 | SLA/App config additions | libslic3r + app-config seam | Modified (additive) |
| C5 | Bundled profiles | resources/profiles | New data |
| C6 | GUI settings/status | slic3r/GUI | Modified (additive, minimal) |
