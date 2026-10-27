# Units of Work — 3rd-Party SLA Printer Support

**Context**: Brownfield, single-service desktop app (PrusaSlicer). These units are **logical development modules** within the existing application, not independently deployable services. Decomposition confirmed by plan answers: 5 units (Q1=A), profiles in U1 (Q2=A), sequential implementation (Q3=A).

---

## U1 — Config & Bundled Profiles
- **Purpose**: Establish the configuration foundation and ship the printer data.
- **Components**: C4 (config), C5 (profiles).
- **Responsibilities**:
  - Allow `sla_archive_format` to carry `uvtools:<id>` values (no new per-printer key).
  - Define the global UVtools path settings (user override + remembered/cached path) and the `UVToolsPaths` seam that injects them into libslic3r.
  - Add bundled SLA printer profiles for Anycubic, Phrozen, Elegoo (`resources/profiles/`), each with correct build volume/display and a `uvtools:<id>` target format.
- **Deliverables**: config entries + seam struct; vendor INIs (+ `.idx`).
- **Off-limits**: core slicing.
- **Notes**: Final bundled model list + specs is a Functional-Design spike item.

## U2 — SLA Format Mapping
- **Purpose**: Translate profile format ids to UVtools format + extension.
- **Components**: C3 SLAFormatMap (pure).
- **Responsibilities**:
  - `is_convert_target`, `lookup`, `supported_targets` over the `uvtools:*` set.
  - Support multiple formats per manufacturer; total + side-effect-free.
- **Deliverables**: new header/source under `Format/UVTools/`; unit + property tests.
- **PBT**: primary property target (mapping invariants).

## U3 — UVtools Service
- **Purpose**: Own acquisition and invocation of the external tool.
- **Components**: C2 UVToolsService.
- **Responsibilities**:
  - Locate (override → cached → acquire), acquire (HTTPS download + integrity verify + cache), invoke (`UVtools.Cmd convert`, argument-array, timeout).
  - Return typed `ConvertResult`.
- **Deliverables**: new module under `Format/UVTools/`; unit tests (mapping of exit codes/results), integration test (real/mocked convert); property test for argument construction.
- **Security**: SECURITY-10/13 (verify download), SECURITY-05 (injection-safe args), SECURITY-15 (fail-safe), SECURITY-03 (safe logging).
- **Spike**: exact convert syntax, download URLs, version pin, verification method.

## U4 — Convert Writer & Fallback
- **Purpose**: The Strategy-A registry writer tying it together.
- **Components**: C1 SLAConvertArchiveWriter + registry entry.
- **Responsibilities**:
  - Register for `uvtools:*`; write intermediate `.sl1` (reuse SL1); map (U2); convert (U3); finalize on success; fallback (keep `.sl1` + non-fatal warning) on failure.
  - Guarantee existing writers/outputs unchanged (US-4).
- **Deliverables**: new writer files; one-line registry addition; `CMakeLists.txt` update; tests (success + each failure mode → fallback).

## U5 — GUI Settings & Status
- **Purpose**: Minimal user-facing surface.
- **Components**: C6.
- **Responsibilities**:
  - Global UVtools path override field in SLA printer settings; populate `UVToolsPaths` at the boundary; persist cached path.
  - Progress during download/convert; present fallback warning via existing SLA export status channel.
- **Deliverables**: GUI field + status wiring (`src/slic3r/GUI/`).
- **Scope**: minimal; no conversion logic in GUI.

## Implementation order
U1 → U2 → U3 → U4 → U5 (sequential, Q3=A). Each unit is independently buildable/testable; U4 integrates U2+U3; U5 integrates U1/U3/U4 at the UI.
