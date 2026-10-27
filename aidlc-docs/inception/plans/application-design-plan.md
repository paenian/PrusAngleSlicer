# Application Design Plan — 3rd-Party SLA Printer Support

**Role**: Application designer
**Sources**: requirements.md, stories.md (US-1..US-7), reverse-engineering/*

## Methodology / Execution Checklist (Part 2 will execute after approval)
- [x] Generate `components.md` (component purposes + responsibilities + interfaces)
- [x] Generate `component-methods.md` (method signatures + I/O; business rules deferred to Functional Design)
- [x] Generate `services.md` (service/orchestration definitions)
- [x] Generate `component-dependency.md` (dependency matrix + data flow)
- [x] Generate consolidated `application-design.md`
- [x] Validate completeness/consistency against requirements + stories

## Proposed Components (for your review — questions below confirm/adjust)

1. **SLAConvertArchiveWriter** — a new `SLAArchiveWriter` subclass registered in the SLA format registry (Strategy A). Produces the intermediate `.sl1` (reusing `SL1Archive`), then delegates conversion to the UVtools service, and implements the fallback-on-failure behavior. *(US-3, US-7; FR-3/FR-6)*
2. **UVToolsService** — encapsulates everything about the external tool: locate (remembered path / override), acquire (download + integrity verify + cache), and invoke (`UVtools.Cmd convert`, injection-safe argument array). *(US-5, US-6; FR-4, NFR-2)*
3. **SLAFormatMap** — maps a printer profile's target identifier (`sla_archive_format` convention) to the UVtools format name + output file extension; supports multiple formats per manufacturer. Pure, property-testable. *(US-1/US-2/US-3; FR-5)*
4. **SLA printer config additions** — any new keys on `SLAPrinterConfig` (e.g. UVtools path override). Target-format selection reuses `sla_archive_format` per Q7. *(US-2, US-6)*
5. **Bundled profiles (data)** — Anycubic/Phrozen/Elegoo vendor INIs under `resources/profiles/`. *(US-1; FR-2)*
6. **GUI settings surface (minimal)** — SLA printer-settings additions for UVtools override + download/convert status/warnings. *(US-6, US-7; FR-7)*

Core slicing (`SLAPrintSteps`, rasterization) is untouched.

---

# Clarifying Questions

Answer each `[Answer]:` tag. My recommendation is option A each time.

## Question 1 — Component boundary for the UVtools service
Where should the download/verify/invoke logic live?

A) A single self-contained `UVToolsService` module under `src/libslic3r/` (e.g. `Format/UVTools/`), callable from the writer — keeps it in the core lib, reusable by GUI and CLI. (Recommended)

B) Split: download/acquire in the GUI layer (`src/slic3r`), invoke in libslic3r.

C) Other (please describe after [Answer]: tag below)

[Answer]: A

## Question 2 — Who triggers the UVtools download?
Auto-download is a potentially slow, network + GUI-progress operation. Where should it be initiated?

A) Lazily inside the writer/service on first export that needs it, surfacing progress through the existing slicing-status/ progress callback. (Recommended — one path, works for the registry writer)

B) Eagerly from the GUI when a 3rd-party profile is selected (pre-fetch before export).

C) Other (please describe after [Answer]: tag below)

[Answer]: A

## Question 3 — Config key for the UVtools override path
How to store the manual override (US-6)?

A) New `SLAPrinterConfig` string key (e.g. `uvtools_path`), empty = auto-download/auto-detect. (Recommended — travels with the printer profile)

B) A global app setting (AppConfig), not per-printer.

C) Other (please describe after [Answer]: tag below)

[Answer]: B

## Question 4 — Format identifier convention (reuses sla_archive_format, Q7)
How should the target format be encoded in `sla_archive_format` so SLAFormatMap can route it?

A) Use a namespaced id aligned to UVtools naming, e.g. `uvtools:ctb`, `uvtools:ctb_encrypted`, `uvtools:goo`, `uvtools:pm3n` — the `uvtools:` prefix signals the convert path; the suffix is the UVtools format id. Non-prefixed values (e.g. `SL1`) keep today's behavior. (Recommended — unambiguous, multi-format-per-vendor, no new key)

B) Plain format id without a prefix; SLAFormatMap decides by lookup table membership.

C) Other (please describe after [Answer]: tag below)

[Answer]: A

## Question 5 — Fallback warning surfacing (US-7)
How should the "conversion didn't run, .sl1 left" warning reach the user?

A) Via the existing SLA export status/error channel (non-fatal warning message), consistent with how other export messages appear. (Recommended)

B) A dedicated modal dialog.

C) Other (please describe after [Answer]: tag below)

[Answer]: A

## Question 6 — Anything else for the design
Any component boundary, naming, or interaction preferences I should honor?

[Answer]: nope
