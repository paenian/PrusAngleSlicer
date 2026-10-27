# Business Overview — 3rd-Party SLA Printer Support

**Scope note**: This reverse-engineering pass is intentionally scoped to the subsystems relevant to the target feature (3rd-party SLA printer support), not the entire PrusaSlicer codebase. Core SLA slicing algorithms (rasterization, supports, hollowing) are mapped only enough to understand integration boundaries; they are out of scope for modification.

## What the system does today (SLA path)

PrusaSlicer is a dual-technology slicer: FFF (filament) and SLA (resin/MSLA). For SLA it:

1. Loads a **printer profile** that declares the machine's build volume (`bed_shape`, `max_print_height`), LCD display geometry (`display_width/height`, `display_pixels_x/y`, orientation, mirroring), and — critically — an **output archive format** id (`sla_archive_format`).
2. Runs the **SLA slicing pipeline** (`SLAPrint::process`) which hollows, generates supports, slices objects + supports into per-layer raster images.
3. **Exports** the sliced result by handing the rasterized layers to an **archive writer** selected by `sla_archive_format`, producing a printable file (e.g. `.sl1`, `.sl1s`, or an Anycubic `.pw*` binary).

## The business problem this feature solves

Resin printer file formats are fragmented: nearly every manufacturer uses its own proprietary container (CTB, GOO, PWS/PW*, etc.). Today PrusaSlicer can natively write only a handful (PrusaResearch SL1/SL1S/SL1SVG and a few Anycubic formats). Owners of other 3rd-party SLA printers cannot slice directly to a file their printer accepts.

**Goal**: Let users select or define a profile for a 3rd-party SLA printer (with arbitrary build volume), slice with the existing SLA core, and produce a file in that printer's native format — by integrating **UVtools** (an established MSLA/DLP tool supporting 35+ formats) as a separate but linked **post processor** that converts PrusaSlicer's SLA output into the manufacturer format.

## Core business transactions (today, SLA)

| Transaction | Entry point | Outcome |
|---|---|---|
| Select SLA printer | Preset system (profile INI) | SLAPrinterConfig populated incl. `sla_archive_format` |
| Slice SLA job | `SLAPrint::process()` | Per-layer encoded rasters held in archive writer |
| Export SLA file | `SLAPrint::export_print()` → writer | Printable archive on disk |
| (FFF only) Post-process G-code | `run_post_process_scripts()` | External script transforms output file |

## What changes (feature intent)

- **Printer loading**: add 3rd-party SLA printer profiles with arbitrary build volumes; optionally bundle presets for popular local-manufacturer printers.
- **Output translation**: add a conversion step that turns the sliced SLA output into the target manufacturer format via UVtools, without touching core slicing.

## Key external dependency: UVtools

- MSLA/DLP file analysis, repair, calibration, **conversion** tool. Cross-platform (Windows/Linux/macOS). Has a GUI and a CLI (`UVtools.Cmd`) with a `convert` command. Supports 35+ formats (CTB, GOO, PWS, SL1, etc.). Can read a `.sl1` and convert to a target format.
- Built in **.NET/C#** — it is NOT a C++ library and cannot be linked into libslic3r directly. It runs as a **separate process** (the user's phrase "separate but linked post processor" is architecturally accurate: linked by invocation, not by static/dynamic linking).
- **License: AGPL-3.0** — this is a significant constraint that must be evaluated (see Requirements). Shelling out to an independently-installed UVtools binary is materially different, license-wise, from bundling or linking its code.

Content was rephrased for compliance with licensing restrictions. Source: [UVtools (sn4k3/UVtools)](https://github.com/sn4k3/UVtools).
