# Component Inventory — Feature-Relevant Modules

Modules in scope for 3rd-party SLA printer support, with their role and whether the feature is expected to touch them.

| Component | File(s) | Role | Feature touches? |
|---|---|---|---|
| SLAPrinterConfig | `src/libslic3r/PrintConfig.hpp` (~1352), `PrintConfig.cpp::init_sla_params` (~4340) | SLA printer config schema (build volume, display, output format) | Maybe — only if new config keys needed (e.g. SLA post-process option) |
| SLA printer profiles | `resources/profiles/*.ini` (+ `.idx`), e.g. `AnycubicSLA.ini` | Bundled vendor/printer presets | Yes — add 3rd-party printers / new vendor INI |
| SLAArchiveWriter (abstract) | `src/libslic3r/Format/SLAArchiveWriter.hpp/.cpp` | Base class + factory for output writers | Maybe — if Strategy A (new writer subclass) |
| SLAArchiveFormatRegistry | `src/libslic3r/Format/SLAArchiveFormatRegistry.hpp/.cpp` | id → writer/reader factory table | Yes if Strategy A — register new format id |
| SL1Archive | `src/libslic3r/Format/SL1.cpp` | Canonical PrusaSlicer SLA archive (.sl1/.sl1s) | Reference / reused as intermediate |
| AnycubicSLAArchive | `src/libslic3r/Format/AnycubicSLA.cpp` | In-process raster→manufacturer binary writer | Reference model for a native writer |
| SLAPrint | `src/libslic3r/SLAPrint.cpp/.hpp` | Orchestrates SLA process + export | Read-only understanding; export seam at ~360/~746 |
| SLAPrintSteps | `src/libslic3r/SLAPrintSteps.cpp` | Core rasterization/supports/hollowing | NO — out of scope (must not modify) |
| RasterBase / AGGRaster | `src/libslic3r/SLA/RasterBase.hpp`, `AGGRaster.hpp` | Per-layer raster image | NO — reuse only |
| PostProcessor (FFF) | `src/libslic3r/GCode/PostProcessor.cpp` + `.hpp` | External script spawner (`run_script`) | Reference model; maybe reused/generalized for SLA |
| Export call sites (GUI) | `src/slic3r/GUI/BackgroundSlicingProcess.cpp` (~196/214/785) | GUI export flow | Maybe — if Strategy B (hook after export) |
| Export call sites (CLI) | `src/CLI/ProcessActions.cpp` (~324, ~393-406) | CLI export flow + tech selection | Maybe — if Strategy B |
| Printer Settings GUI | `src/slic3r/GUI/Tab.cpp` (SLA printer tab) | Surfaces printer config keys to user | Maybe — if new config keys need UI |
| Preset system | `src/libslic3r/Preset.cpp/.hpp`, `PresetBundle` | Loads/validates profiles | Read-only unless new config keys added |

## External components

| Component | Nature | Integration |
|---|---|---|
| UVtools | .NET/C# app, GUI + `UVtools.Cmd` CLI, AGPL-3.0 | External process invocation (`convert`). Not linkable into C++. User-installed or bundled binary. |

Content was rephrased for compliance with licensing restrictions. UVtools details: [sn4k3/UVtools](https://github.com/sn4k3/UVtools).
