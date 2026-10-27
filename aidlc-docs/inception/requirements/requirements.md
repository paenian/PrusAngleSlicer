# Requirements — 3rd-Party SLA Printer Support

## Intent Analysis
- **User request**: Add support for 3rd-party SLA (resin/MSLA) 3D printers to the PrusaSlicer fork: (1) printer loading with arbitrary build-volume sizes plus bundled profiles for popular manufacturers, and (2) translation of PrusaSlicer's SLA output into manufacturer-specific file formats by integrating UVtools as a separate but linked post processor. Core SLA slicing is assumed correct and must not change.
- **Request type**: New feature (brownfield enhancement).
- **Scope estimate**: Multiple components — SLA printer config/profiles, a new SLA output format + conversion path, external-tool acquisition/invocation, GUI surfacing.
- **Complexity estimate**: Complex — new external (.NET, AGPL) dependency invoked as a subprocess, auto-download/caching, format mapping across manufacturers, failure handling, all while minimizing changes to existing code.

## Decision Summary (from clarifying answers)
| # | Decision |
|---|---|
| Q1 | Build BOTH: 3rd-party printer profiles AND the UVtools conversion pipeline. |
| Q2/Q3 | Bundle a small set of popular printers covering **Anycubic, Phrozen, Elegoo**. |
| Q4 | **Strategy A — Registry writer**: a new built-in SLA output format; export produces the manufacturer file directly, UVtools invoked under the hood. |
| Q5 | **Auto-download** UVtools on first use from its official source (user's machine fetches it; we do not distribute it). |
| Q6 | Slicer **downloads UVtools itself and remembers where it installed it** (cached path); no manual path entry required in the normal path. |
| Q7 | Map target format via the existing `sla_archive_format` key using a naming convention; account for **multiple formats per manufacturer**, aligned to **how UVtools labels/identifies formats**. |
| Q8 | On missing/failed conversion: **fall back** to leaving the intermediate `.sl1` and **warn** the user that conversion did not run. |
| Q9 | **GUI only** is sufficient for this first pass. |
| Q10 | Success = slice a test model for a 3rd-party printer and confirm a **valid manufacturer-format file** is produced that UVtools (re-opening) / the printer accepts. |
| Q11 | **Minimize changes**; core SLA slicing is off-limits; prefer additive/new files. |
| Q12–Q14 | Extensions ENABLED: Security (full), Resiliency (directional), Property-Based Testing (full). |

## Functional Requirements

### FR-1 — Arbitrary-size SLA printer profiles
The system shall allow SLA printer profiles with arbitrary build volumes (bed shape, max print height) and display geometry (physical size, pixel resolution, orientation, mirroring), using the existing `SLAPrinterConfig` schema (which already imposes only soft minimums). Custom user-defined sizes shall be supported without code changes.

### FR-2 — Bundled 3rd-party printer profiles
The system shall ship profiles for selected popular SLA printers from **Anycubic, Phrozen, and Elegoo**. Each profile shall set `printer_technology = SLA`, correct build volume and display parameters, and a target output format identifier (per FR-5). The specific printer models per manufacturer shall be finalized during design (candidate list to be confirmed), chosen to cover common/current models.

### FR-3 — SLA output-format translation via UVtools (Registry writer)
The system shall add a new SLA output path (Strategy A) that, on export, (a) produces the sliced layers as an intermediate PrusaSlicer `.sl1` archive using the existing writer, and (b) invokes UVtools to convert that intermediate into the manufacturer format selected by the printer profile, placing the result at the user's chosen output path and removing the intermediate on success. This shall NOT modify core SLA slicing (`SLAPrintSteps.cpp`, rasterization).

### FR-4 — UVtools acquisition and discovery (auto-download + remember)
- The system shall automatically download UVtools from its official source on first use when it is not already present.
- The system shall remember (cache/persist) the resolved UVtools executable location for subsequent runs, avoiding repeat downloads.
- The system shall NOT bundle or redistribute UVtools (the user's machine fetches it) to respect the AGPL-3.0 licensing posture (see CON-1).
- A manual override of the UVtools path is desirable as a fallback but is secondary to the auto-download path.

### FR-5 — Target-format selection and mapping
- A printer profile shall declare its target manufacturer format via the `sla_archive_format` key using a naming convention recognized by the new conversion path.
- The mapping shall accommodate **multiple formats per manufacturer** (e.g. Chitubox CTB variants, Anycubic PW*, Phrozen/Elegoo GOO, etc.).
- The format identifiers and extensions shall align with **how UVtools names/identifies formats** so the convert invocation can target them unambiguously. Investigating UVtools' exact format identifiers and the `UVtools.Cmd convert` argument syntax is a required design/spike task.

### FR-6 — Failure handling (fallback + warn)
If UVtools is missing and cannot be obtained, is the wrong/incompatible version, or the conversion fails, the system shall NOT fail the export outright. It shall leave the intermediate `.sl1` file in place and present a clear warning to the user that the manufacturer-format conversion did not run (including the reason where available).

### FR-7 — GUI integration
The feature shall be usable end-to-end from the GUI: selecting a bundled/custom 3rd-party SLA printer profile and exporting shall produce the manufacturer-format file (or the `.sl1` fallback with a warning per FR-6). Any new settings (e.g. UVtools path override, download status) shall be surfaced in the SLA printer settings UI.
- **Assumption A-1 (confirm at approval)**: Because Strategy A hooks at the `SLAArchiveWriter` level (below both front-ends), the conversion will also work from the CLI for free. "GUI only" is interpreted as *GUI is the required/validated target for this pass*, not as a requirement to actively block the CLI path. Flagging for confirmation.

## Non-Functional Requirements

### NFR-1 — Minimal footprint on existing code (CON / Q11)
Changes shall be additive and isolated where possible (new files/modules under `src/libslic3r/Format/` and a focused download/invocation module). No changes to core SLA slicing algorithms. Existing output formats (SL1/SL1S/SVG/Anycubic) shall remain byte-compatible and unaffected.

### NFR-2 — Security (Security Baseline ENABLED)
Applicable rules for a desktop tool that downloads and executes an external binary:
- **SECURITY-13 (Integrity)**: The auto-downloaded UVtools artifact MUST be verified via checksum and/or digital signature before execution; download MUST use TLS from the official source. No execution of an unverified binary.
- **SECURITY-10 (Supply chain)**: The UVtools source/version MUST be pinned/known; record the expected version and verification method.
- **SECURITY-05 (Input validation)**: All values passed to the UVtools subprocess (paths, format ids) MUST be validated/escaped to prevent command/argument injection; use argument-array invocation, never shell string interpolation.
- **SECURITY-15 (Exceptional conditions / fail-safe)**: Download and conversion errors MUST be handled explicitly and fail safe to the FR-6 fallback; resources (temp files, processes) cleaned up on all paths.
- **SECURITY-03 (Logging)**: Log download/convert actions and outcomes without leaking sensitive data.
- Rules tied to web servers, data stores, auth, network intermediaries (SECURITY-01/02/04/06/07/08/12/14) are **N/A** for this desktop feature (no server, DB, auth, or network service introduced).

### NFR-3 — Resiliency (Resiliency Baseline ENABLED, directional)
Desktop-appropriate subset: graceful degradation (FR-6 fallback), retry/timeout on the download, clear observability of the download/convert steps (progress + errors in the GUI), and idempotent re-runs (cached UVtools, re-export safe). Cloud/HA/DR practices are not applicable to a local desktop tool.

### NFR-4 — Property-Based Testing (PBT ENABLED, full)
PBT applies to the pure/serialization-like parts of this feature:
- **Format-id ↔ UVtools-format/extension mapping** (FR-5): round-trip / invariant properties (a valid profile id always maps to a known format + extension; mapping is total over the supported set).
- **Convert-command construction** (FR-3/NFR-2): invariant/property that generated argument arrays are well-formed and injection-safe for arbitrary (including adversarial) path/id inputs.
- The subprocess execution and network download themselves are integration-tested, not PBT (external side effects). Framework: a C++ PBT approach compatible with the existing Catch2-based tests (e.g. RapidCheck) — to be finalized in NFR/design.

### NFR-5 — Maintainability
New code shall be self-contained modules with clear seams, so UVtools version bumps or format additions are data/config changes, not core rewrites.

## Constraints

- **CON-1 (License, AGPL-3.0)**: UVtools is AGPL-3.0. The chosen posture (auto-download, never bundle/redistribute — Q5=C/Q6=D) keeps UVtools at arm's length as a user-obtained, separately-executed binary. The feature MUST NOT link UVtools code into the slicer or redistribute UVtools binaries. (UVtools is .NET/C# and is not linkable into C++ regardless.) A brief license note should accompany the download step.
- **CON-2 (No core slicing changes)**: `SLAPrintSteps.cpp` and the rasterization pipeline are off-limits.
- **CON-3 (Platform)**: macOS is the verified build/dev platform this pass (per environment). Design should not preclude Windows/Linux, but validation targets macOS first.
- **CON-4 (Toolchain)**: Apple clang 21, CMake 4.3.4 (`-DCMAKE_POLICY_VERSION_MINIMUM=3.5`), two-stage build; new C++ files require `CMakeLists.txt` updates.

## Success Criteria (Q10)
1. A 3rd-party SLA printer profile (bundled Anycubic/Phrozen/Elegoo or a custom arbitrary-size one) can be selected in the GUI.
2. Slicing a test model and exporting produces a valid manufacturer-format file that UVtools can re-open (and that the target printer would accept).
3. On UVtools unavailability/failure, the export falls back to `.sl1` with a clear warning (no crash, no corrupt output).
4. Existing SL1/SL1S/Anycubic outputs and all core slicing behavior are unchanged.
5. Build compiles on macOS; added mapping/command-construction logic covered by example-based + property-based tests.

## Open Items for Design / Spike
1. Exact `UVtools.Cmd convert` argument syntax and the canonical list of UVtools format identifiers + extensions (per manufacturer, multiple per vendor).
2. UVtools official download URL(s) per platform, versioning, and the checksum/signature verification mechanism (NFR-2 / SECURITY-13).
3. Final list of bundled printer models for Anycubic, Phrozen, Elegoo (build volumes, display specs, target formats).
4. Where the cached UVtools path and download live (app data dir), and GUI surface for status/override.
5. Confirm Assumption A-1 (CLI works for free; GUI is the validated target).

Content was rephrased for compliance with licensing restrictions. UVtools: [sn4k3/UVtools](https://github.com/sn4k3/UVtools) (AGPL-3.0).
