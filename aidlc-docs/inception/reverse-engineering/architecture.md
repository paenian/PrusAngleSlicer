# Architecture — SLA Printer Loading, Output, and Post-Processing

**Scope**: SLA printer profiles, the SLA slicing→output seam, output format registry, and external-process integration points. Core rasterization/supports/hollowing are treated as a black box that must not be modified.

## Layered view (SLA path)

```
+-------------------------------------------------------------+
|  GUI (src/slic3r) / CLI (src/CLI)                           |
|   - Preset selection, Printer Settings tab                  |
|   - Export action -> BackgroundSlicingProcess / ProcessActions |
+-------------------------------------------------------------+
                 |  apply(model, config) / export
                 v
+-------------------------------------------------------------+
|  libslic3r core (src/libslic3r)                             |
|   SLAPrint  ---process()--->  per-layer encoded rasters     |
|      |                                                      |
|      | export_print(fname)                                 |
|      v                                                      |
|   SLAArchiveWriter (abstract)  <--- created by factory      |
|      ^   selected by sla_archive_format                     |
|      |                                                      |
|   SLAArchiveFormatRegistry (id -> writer/reader factory)    |
|      |                                                      |
|   Concrete writers: SL1Archive, SL1_SVGArchive,            |
|                     AnycubicSLAArchive                      |
+-------------------------------------------------------------+
                 |  (NEW) convert output to manufacturer format
                 v
+-------------------------------------------------------------+
|  External process: UVtools.Cmd convert  (AGPL, .NET)        |
+-------------------------------------------------------------+
```

## Key components and files

### Configuration schema
- `src/libslic3r/PrintConfig.hpp` — `SLAPrinterConfig` class (lines ~1352-1382). Holds `printer_technology`, `bed_shape`, `max_print_height`, `display_width/height`, `display_pixels_x/y`, `display_orientation`, `display_mirror_x/y`, `sla_archive_format`, `printer_model`.
- `src/libslic3r/PrintConfig.cpp` — `PrintConfigDef::init_sla_params()` (from line ~4340) defines the options, defaults, and (soft) minimums. `sla_archive_format` defined at ~4923 (default "SL1"). Build-volume sizes are effectively arbitrary (only per-field min guards; no upper bound).
- `get_bed_shape(const SLAPrinterConfig&)` declared in PrintConfig.hpp ~line 1515.

### Printer profiles (data)
- `resources/profiles/<Vendor>.ini` (+ `.idx`). SLA example: `resources/profiles/AnycubicSLA.ini`.
- Each vendor INI has `[vendor]`, `[printer_model:...]` (with `technology = SLA`), and `[printer:...]` presets setting `printer_technology = SLA`, `bed_shape`, `max_print_height`, `display_*`, and `sla_archive_format = <registered id>`.
- Adding a new 3rd-party printer = new/extended vendor INI. **No C++ change** unless a brand-new output-format id is needed.

### SLA slicing → output seam
- `src/libslic3r/SLAPrint.cpp`:
  - `m_archiver = SLAArchiveWriter::create(sla_archive_format, m_printer_config)` (~line 360) — archiver chosen when printer config changes.
  - `SLAPrint::export_print(fname, thumbnails, projectname)` (~line 746) — delegates to `m_archiver->export_print(...)`; throws "Unknown archive format" if archiver is null.
- `src/libslic3r/SLAPrintSteps.cpp` — core rasterization (`lvlfn` lambda on `sla::RasterBase&`, ~line 1319). **DO NOT MODIFY.**

### Output format abstraction (primary integration surface)
- `src/libslic3r/Format/SLAArchiveWriter.hpp` — abstract `SLAArchiveWriter` with `create_raster()`, `get_encoder()`, `draw_layers(...)`, `export_print(...)`, and static `create(archtype, cfg)`.
- `src/libslic3r/Format/SLAArchiveFormatRegistry.hpp/.cpp` — the `Registry` singleton with an `entries` set (`.cpp` lines ~24-76). Each `ArchiveEntry` maps an id → description, extension(s), writer factory, reader factory. **This is the single place a new output format is registered.** Lookup via `get_writer_factory(id)` / `registered_sla_archives()`.
- Concrete writers: `src/libslic3r/Format/SL1.cpp` (SL1/SL1S zip archive — the canonical PrusaSlicer SLA output), `SL1_SVG.cpp`, `AnycubicSLA.cpp` (in-process raster→binary converter; closest existing analog to a manufacturer-format writer).

### Export call sites (where a post-process could hook)
- GUI: `src/slic3r/GUI/BackgroundSlicingProcess.cpp` — SLA export at ~line 214 (local) and ~785 (upload) via `m_sla_print->export_print(...)`. Output path from `finalize_output_path(m_export_path)` (~line 196).
- CLI: `src/CLI/ProcessActions.cpp` — SLA branch ~lines 393-397: `sla_print.export_print(outfile_final)`.

### External-process pattern (reference)
- `src/libslic3r/GCode/PostProcessor.cpp` — `run_post_process_scripts(...)` and `run_script(...)`. FFF-only today (explicit "likely running in SLA mode" early-return when `post_process` is empty). Passes output path + config via env vars (`SLIC3R_PP_HOST`, `SLIC3R_PP_OUTPUT_NAME`); supports output rename via a `.output_name` sidecar. This is the model for spawning UVtools.
- `post_process` config option defined in `PrintConfig.cpp` ~line 2640, attached to the FFF `PrintConfig` only (PrintConfig.hpp ~line 1020).

## Two candidate integration strategies (detailed in dependencies.md and carried to design)

- **Strategy A — Registry writer**: register a new `ArchiveEntry` whose `export_print` writes an intermediate `.sl1` then invokes `UVtools.Cmd convert` to produce the manufacturer file. Pros: participates in normal format selection via `sla_archive_format`; one seam; no core-slicing change. 
- **Strategy B — External post-process hook (SLA)**: add an SLA-side `sla_post_process` config + invoke an external converter at the export call sites, mirroring FFF `run_post_process_scripts`. Pros: fully decoupled, generic; closest to the literal "separate but linked post processor" framing.

Both avoid modifying `SLAPrintSteps.cpp` (core slicing). Selection between A and B is a design decision for the Requirements/Design stages.
