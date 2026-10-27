# Application Design (Consolidated) — 3rd-Party SLA Printer Support

Consolidates `components.md`, `component-methods.md`, `services.md`, and `component-dependency.md`.

## Overview
Strategy-A (registry writer) design: a new SLA output format (`uvtools:*`) produces an intermediate `.sl1`, then converts it to a manufacturer format via an auto-acquired UVtools, with a safe `.sl1` fallback. Additive and isolated; core SLA slicing untouched.

## Design decisions (from plan answers)
- **Q1=A** — UVtools logic is a self-contained libslic3r module (`Format/UVTools/`), reusable by GUI + CLI.
- **Q2=A** — Download is lazy (first export that needs it), progress via existing status callback.
- **Q3=B** — UVtools path is a **global app setting** (one install per machine), injected into libslic3r via a `UVToolsPaths` seam (libslic3r stays GUI-agnostic).
- **Q4=A** — Target format carried in `sla_archive_format` as `uvtools:<id>` (e.g. `uvtools:ctb`, `uvtools:goo`); non-prefixed ids keep existing behavior; supports multiple formats per manufacturer.
- **Q5=A** — Fallback warning via existing SLA export status/error channel (non-fatal).

## Components
- **C1 SLAConvertArchiveWriter** (new, libslic3r/Format) — orchestrates write-.sl1 → map → convert → finalize/fallback. Registered for `uvtools:*`.
- **C2 UVToolsService** (new, libslic3r/Format/UVTools) — locate / acquire(download+verify+cache) / invoke(injection-safe, timeout). Returns typed `ConvertResult`.
- **C3 SLAFormatMap** (new, pure) — `sla_archive_format` → `{uvtools_id, extension}`; total, side-effect-free (PBT target).
- **C4 Config/App-config** (additive) — global UVtools override + cached path; `sla_archive_format` carries `uvtools:*`.
- **C5 Bundled profiles** (data) — Anycubic/Phrozen/Elegoo INIs.
- **C6 GUI settings/status** (additive, minimal) — global path field, progress, fallback warning.

## Services
- **S1 Export/Convert orchestration** (C1) — synchronous on the background export thread; progress/warnings via existing callbacks.
- **S2 UVtools acquisition** (C2) — lazy, verified, idempotent (cached after first success).

## Key seam
Global UVtools paths (Q3=B) are populated at the GUI/CLI boundary into a `UVToolsPaths` struct and passed into the libslic3r `UVToolsService`; cached-path persistence goes back via an injected writer. Keeps libslic3r free of GUI dependencies.

## Traceability
- US-1/FR-2 → C5 profiles; US-2/FR-1,FR-5 → C3 + C4; US-3/FR-3 → C1 + C2 + C3; US-4/NFR-1,CON-2 → isolation (registry entry only); US-5/FR-4,NFR-2 → C2 acquire; US-6/FR-4 → C4 global override + C6; US-7/FR-6,NFR-2/3 → C1 fallback + status.

## NFR hooks (detailed in CONSTRUCTION NFR stages)
- **Security**: HTTPS + integrity verification before executing downloaded binary (SECURITY-10/13); argument-array subprocess invocation (SECURITY-05); fail-closed to `.sl1` fallback + resource cleanup (SECURITY-15); safe logging (SECURITY-03).
- **Resiliency**: download retry/timeout, graceful degradation (fallback), idempotent cached acquisition, visible progress.
- **PBT**: `SLAFormatMap` mapping invariants + convert-command argument construction safety.

## Minimal-footprint summary
New files: C1, C2, C3. Additive edits: one registry entry, config keys, minimal GUI, `CMakeLists.txt`. Untouched: core slicing + existing writers (byte-compatible).

## Open items carried to Functional/NFR design
1. Exact `UVtools.Cmd convert` arg syntax + canonical UVtools format ids/extensions (spike).
2. UVtools download URL(s) per platform, version pin, verification mechanism.
3. Final bundled model list + specs for Anycubic/Phrozen/Elegoo.
4. App-config key names + app-data cache location.
