# Technology Stack (Feature-Relevant)

## PrusaSlicer (host application)
- **Language**: C++ (C++17), with C and some Objective-C++ (macOS). CMake build.
- **Core library**: `libslic3r` (slicing engine, config, formats). GUI in `src/slic3r` (wxWidgets + ImGui). CLI in `src/CLI`.
- **SLA output**: raster-image archives written via a registry-selected `SLAArchiveWriter`. Zip packaging via internal `Zipper` (miniz). Raster via AGG.
- **Config/profile system**: `DynamicPrintConfig` / typed `*Config` classes; presets loaded from INI under `resources/profiles/`.
- **Build**: two-stage CMake (deps then app). Verified: Apple clang 21, CMake 4.3.4 (`-DCMAKE_POLICY_VERSION_MINIMUM=3.5`), `make -j8`, binary `build/src/PrusaSlicer`.

## UVtools (external post processor)
- **Runtime**: .NET / C# (cross-platform). Separate process; not linkable into C++.
- **Interface for integration**: `UVtools.Cmd` CLI, `convert` verb; reads `.sl1`, writes manufacturer formats.
- **License**: AGPL-3.0 (constraint — see dependencies.md / requirements).

## Integration technology options
- **Process spawning**: reuse the pattern in `src/libslic3r/GCode/PostProcessor.cpp` (`run_script`), which already handles cross-platform spawning, env-var passing, and output renaming.
- **No new runtime added to the C++ side** — UVtools stays external. The feature adds C++ glue (config keys, invocation, error handling) and data (profiles), not a new library dependency in libslic3r.

Content was rephrased for compliance with licensing restrictions. UVtools: [sn4k3/UVtools](https://github.com/sn4k3/UVtools).
