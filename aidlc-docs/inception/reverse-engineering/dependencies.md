# Dependencies & Integration Analysis

## Internal dependency chain (SLA output)

```
Preset INI (sla_archive_format)
   -> SLAPrinterConfig.sla_archive_format
      -> SLAArchiveWriter::create(id, cfg)        [SLAArchiveWriter.cpp ~14]
         -> get_writer_factory(id)                [SLAArchiveFormatRegistry.cpp]
            -> ArchiveEntry.wrfactoryfn(cfg)       [Registry::entries]
               -> concrete SLAArchiveWriter
   -> SLAPrint::export_print(fname)               [SLAPrint.cpp ~746]
      -> writer->export_print(...)                -> file on disk
```

Core slicing (`SLAPrintSteps.cpp`, `RasterBase`/`AGGRaster`) feeds `m_layers` into the writer but is independent of format selection. A new output format therefore requires **no core-slicing change**.

## External dependency: UVtools

| Attribute | Value | Implication |
|---|---|---|
| Language/runtime | .NET / C# | Cannot be statically/dynamically linked into libslic3r (C++). Must run as a separate process. |
| Interfaces | GUI + `UVtools.Cmd` CLI (`convert` verb) | CLI is the integration channel. |
| Formats | 35+ MSLA/DLP (CTB, GOO, PWS, SL1, ...) | Covers most 3rd-party printers; PrusaSlicer writes `.sl1` as the conversion source. |
| Platforms | Windows, Linux, macOS | Matches PrusaSlicer targets, but binary must be present/resolvable per-platform. |
| License | **AGPL-3.0** | **Blocking legal question.** Invoking a separately-installed binary via CLI (arm's-length) is generally distinct from bundling/linking AGPL code into the slicer. Bundling UVtools, or distributing it with the slicer, likely triggers AGPL obligations on the combined/distributed work. This MUST be resolved in Requirements before design. |

### Invocation model (expected)
UVtools can open a `.sl1` and convert to a target format. A conversion step would:
1. Produce an intermediate `.sl1` from the existing SL1Archive writer (reusing current rasters).
2. Invoke `UVtools.Cmd convert <intermediate.sl1> <target.ext>` (exact flags to be confirmed against the installed UVtools version during design/spike).
3. Place the manufacturer file at the user's chosen output path; clean up the intermediate.

### Open integration questions (for Requirements)
1. **UVtools acquisition**: bundle it, require user-installed, or auto-download? (License + packaging impact.)
2. **Discovery**: how does the slicer locate `UVtools.Cmd` (PATH, config path setting, platform default install dirs)?
3. **Format mapping**: how does a printer profile declare its target UVtools format and extension? (New config key vs. reuse of `sla_archive_format` with a convention.)
4. **Strategy A vs B** (see architecture.md): registry writer vs. generic SLA post-process hook.
5. **Failure UX**: what happens if UVtools is missing, wrong version, or conversion fails? (SLA export has no post-process error path today.)
6. **CLI + GUI parity**: both export paths must route through the conversion.

## Build-system dependencies (verified this session)
- Two-stage CMake build (deps prebuilt at `deps/build/destdir/usr/local`, app via `make -j8`).
- Toolchain: Apple clang 21, CMake 4.3.4 (needs `-DCMAKE_POLICY_VERSION_MINIMUM=3.5`).
- Adding a new C++ format writer → update `src/libslic3r/CMakeLists.txt` and `src/libslic3r/Format/`. Adding only profiles → no build change (resources are data).

Content was rephrased for compliance with licensing restrictions. UVtools: [sn4k3/UVtools](https://github.com/sn4k3/UVtools) (AGPL-3.0).
