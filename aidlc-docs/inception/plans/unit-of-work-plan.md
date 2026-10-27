# Unit of Work Plan — 3rd-Party SLA Printer Support

**Context**: Brownfield, single-service (the PrusaSlicer desktop app). Units here are **logical modules** for development sequencing within the existing app — not deployable services.

## Methodology / Execution Checklist (Part 2 after approval)
- [x] Generate `unit-of-work.md` (unit definitions + responsibilities + components per unit)
- [x] Generate `unit-of-work-dependency.md` (dependency matrix + build/implementation order)
- [x] Generate `unit-of-work-story-map.md` (stories → units)
- [x] Validate boundaries, dependencies, and that all stories (US-1..US-7) are assigned

## Proposed Units (for your review — questions below)

| Unit | Name | Components | Stories |
|---|---|---|---|
| U1 | Config & Bundled Profiles | C4 (config keys incl. `uvtools:*` + global UVtools path seam), C5 (Anycubic/Phrozen/Elegoo profiles) | US-1, US-2 (partial), US-6 (config part) |
| U2 | SLA Format Mapping | C3 SLAFormatMap (pure, PBT target) | US-2, US-3 (routing), US-1 |
| U3 | UVtools Service | C2 (locate / download+verify+cache / invoke) | US-5, US-6 |
| U4 | Convert Writer & Fallback | C1 SLAConvertArchiveWriter + registry entry; fallback behavior | US-3, US-4, US-7 |
| U5 | GUI Settings & Status | C6 (global path field, progress, fallback warning surfacing) | US-6, US-7 (surfacing) |

**Implementation order (by dependency)**: U1 → U2 → U3 → U4 → (U5 last; depends on U1/U3/U4). U2 and U3 are largely independent and could proceed in parallel after U1.

---

# Clarifying Questions

Answer each `[Answer]:` tag. Recommendation is option A.

## Question 1 — Unit granularity
Is the 5-unit decomposition right, or would you prefer coarser/finer?

A) 5 units as proposed (U1 config+profiles, U2 mapping, U3 UVtools service, U4 writer+fallback, U5 GUI). (Recommended — clean seams, each testable)

B) Coarser — merge U2 mapping into U4 writer (4 units total)

C) Finer — split U3 into download/verify vs invoke, or split profiles out of U1 (6+ units)

D) Other (please describe after [Answer]: tag below)

[Answer]: A

## Question 2 — Bundled profiles grouping
The Anycubic/Phrozen/Elegoo profiles (C5) — keep them in U1 (config), or treat as their own unit?

A) Keep in U1 — they're data that depends on the config/format conventions landing first. (Recommended)

B) Separate unit U6 (profiles only), done after U2 so format ids are settled.

C) Other (please describe after [Answer]: tag below)

[Answer]: A

## Question 3 — U2 and U3 parallelization
U2 (mapping) and U3 (UVtools service) are independent after U1. Preference?

A) Sequential U1→U2→U3→U4→U5 for simplicity (single-developer flow). (Recommended)

B) Allow U2 and U3 in parallel after U1.

C) Other (please describe after [Answer]: tag below)

[Answer]: A

## Question 4 — Anything else on decomposition
Any grouping/boundary/ownership preference I should honor?

[Answer]: nope
