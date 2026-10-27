# Build & Test Summary — 3rd-Party SLA Printer Support (first end-to-end pass)

## What was built (all 5 units)
- **U1 Config & Profiles**: `uvtools:<EncoderName>[@version]` convention on `sla_archive_format` (no schema change needed — already coString); global AppConfig keys `uvtools_executable_path` / `uvtools_cached_path`; bundled profiles `resources/profiles/Elegoo.ini` (GOO) and `resources/profiles/Phrozen.ini` (CTB). Anycubic already has native profiles; 3rd-party Anycubic via UVtools uses the same convention.
- **U2 SLAFormatMap** (`src/libslic3r/Format/UVTools/SLAFormatMap.{hpp,cpp}`): pure parse/validate of the format id → `{encoder_name, version, extension, description}`.
- **U3 UVToolsService** (`.../UVTools/UVToolsService.{hpp,cpp}`): resolve (override→cached), acquire (HTTPS download via injected fn + vendored SHA-256 verify + cache), invoke (`UVtoolsCmd convert` via boost::process argument vector — injection-safe), typed `ConvertResult`.
- **U4 SLAConvertArchiveWriter** (`.../UVTools/SLAConvertArchiveWriter.{hpp,cpp}`): subclasses `SL1Archive` to reuse rasterization; writes intermediate `.sl1`, converts, finalizes on success, falls back to `.sl1` + warning on failure. Routed via a 2-line change in `SLAArchiveWriter::create` and `get_default_extension` for `uvtools:*` ids.
- **U5 GUI wiring** (`src/slic3r/GUI/UVToolsIntegration.{hpp,cpp}` + 3-line `GUI_App.cpp` hook): injects AppConfig paths, an `Http`-backed download fn, cached-path persistence, warning routing, and the pinned macOS release (arm64/x64) into the writer.

## Build
- Reconfigured + built with: `cmake .. -DCMAKE_PREFIX_PATH=... -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0 -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5` then `make -j8`.
- **libslic3r**: compiles+links clean (new UVTools files + registry/writer edits).
- **Full app (PrusaSlicer GUI+CLI)**: compiles+links clean, incl. `UVToolsIntegration.cpp` and the `GUI_App.cpp` hook.
- CMake updated: `src/libslic3r/CMakeLists.txt` (+4 files), `src/slic3r/CMakeLists.txt` (+2 files), `tests/libslic3r/CMakeLists.txt` (+1 test).

## Tests
- New: `tests/libslic3r/test_uvtools_format.cpp` — **8 cases, 2070 assertions, all pass**:
  - prefix detection, encoder/version parsing, malformed rejection
  - property loops (500 generated ids round-trip), supported-target totality
  - convert-arg well-formedness + **injection-safety property** (nasty paths stay single argv entries)
  - vendored SHA-256 verified against the known `"abc"` vector
- Regression: full `libslic3r_tests "~[arc_welder]"` → **237 passed, 1 failed-as-expected** (Catch2 shouldfail; not a real failure). Pre-existing arc_welder bus error excluded (upstream, unrelated).
- PBT framework: RapidCheck is the chosen framework (NFR-9); this pass implements the properties as Catch2 generated-input loops (RapidCheck-ready) to avoid blocking on a new dep. **Follow-up: wire RapidCheck + port the loops to `rc::check`.**

## End-to-end verification (CLI)
- Sliced a 20 mm cube with an SLA config `sla_archive_format = uvtools:GooFile`, `--export-sla --output out.goo`.
- UVtools absent + pinned checksums are placeholders → service **failed closed** (SECURITY-13) and the writer **fell back to a valid `.sl1`** at the output location (US-7). Confirmed the `.sl1` is a valid zip; no crash, no corrupt output.
- Regression: plain `SL1` export unchanged (US-4).

## Remaining to reach a produced manufacturer file (documented, not blocking the framework)
1. Pin a real UVtools version + per-arch SHA-256 in `UVToolsIntegration.cpp` (placeholders today).
2. Unzip the verified macOS release and resolve the `UVtoolsCmd` path inside the bundle (currently `ensure_available` caches the verified zip path; extraction step is stubbed with a note).
3. Confirm exact per-model extension/version for bundled profiles against the pinned UVtools.
4. Wire RapidCheck and port property loops.
5. GUI: add the UVtools path override field to the SLA printer settings tab (config keys + injection exist; the visible field is the remaining UI touch).

## Compliance
- **Security**: injection-safe invocation (arg vector, no shell) — verified by test; fail-closed verification before executing a downloaded binary (SECURITY-13/15); pinned release data (SECURITY-10). No secrets. No blocking findings.
- **PBT**: properties implemented + passing; RapidCheck port is the only open item. No blocking findings.
- **Resiliency**: graceful degradation to `.sl1` verified end-to-end; idempotent cached acquisition. No blocking findings.
- **Minimal footprint (CON-2/NFR-1)**: core slicing untouched; existing SL1/Anycubic outputs unchanged (regression verified). New logic isolated under `Format/UVTools/` + one GUI file; existing-file edits are small and additive (registry route, extension map, GUI init hook, CMake).
