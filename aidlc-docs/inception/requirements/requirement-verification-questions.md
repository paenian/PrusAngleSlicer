# Requirements Verification Questions — 3rd-Party SLA Printer Support

Please answer each question by filling in the letter choice after the `[Answer]:` tag.
If none of the options fit, choose the "Other" option and describe your preference after the `[Answer]:` tag.
Let me know when you're done and I'll analyze the answers.

---

## Section 1 — Feature Scope

## Question 1
The feature has two parts: (a) 3rd-party SLA printer profiles with arbitrary build volumes, and (b) output-format translation via UVtools. For this first effort, what should we build?

A) Both parts together — printer profiles AND the UVtools conversion pipeline

B) Output-format translation (UVtools pipeline) only — assume profiles are added as data later

C) Printer profiles only (arbitrary sizes + a few bundled printers) — defer UVtools conversion

D) Other (please describe after [Answer]: tag below)

[Answer]: A

## Question 2
For bundled 3rd-party printer profiles, what's the target for this first pass?

A) Don't bundle any yet — just make arbitrary custom sizes work; users define their own

B) Bundle a small set of popular printers (you name them, e.g. Elegoo Saturn/Mars, Anycubic, etc.)

C) Bundle a broad set covering many manufacturers

D) Other (please describe after [Answer]: tag below)

[Answer]: B - cover Anycubic, Phrozen and Elegoo

## Question 3
If bundling profiles (Q2 = B or C), which specific printers/manufacturers matter most to you? (List them after the tag. If Q2 = A, answer N/A.)

[Answer]: Anycubic, Phrozen and Elegoo

---

## Section 2 — Integration Strategy (the registry writer vs. post-processor choice)

## Question 4
How should the UVtools conversion be wired into the slicer? (See reverse-engineering/architecture.md for detail.)

A) Strategy A — Registry writer: a new built-in SLA output format. User picks their printer profile, clicks Export, and gets the manufacturer file directly (UVtools runs invisibly in the background). Feels native; most C++ work.

B) Strategy B — Post-process hook: add an SLA "post-processing command" setting (like FFF has). Slicer exports .sl1, then runs a user-configured UVtools command. Most decoupled/generic; user wires up the command.

C) Hybrid — Build the generic SLA post-process mechanism (B's plumbing), but ship printer profiles that pre-fill the UVtools convert command so users still get a near one-click experience. Keeps UVtools as external config, not compiled in (helps with AGPL).

D) Other (please describe after [Answer]: tag below)

[Answer]: A

---

## Section 3 — UVtools Dependency (license, acquisition, discovery)

## Question 5
UVtools is licensed AGPL-3.0. How do you want to handle acquisition/distribution? (This affects legal obligations — bundling/distributing AGPL code with the slicer likely triggers AGPL on the combined work; invoking a separately-installed binary at arm's length generally does not.)

A) Require user-installed UVtools — the slicer only invokes it; we never bundle or distribute it (lowest legal risk)

B) Bundle/ship UVtools with the slicer (accept and comply with AGPL obligations on the distribution)

C) Auto-download UVtools on first use from its official source (user's machine fetches it, not us distributing it)

D) Not sure — flag this for a dedicated license review before committing

E) Other (please describe after [Answer]: tag below)

[Answer]: C

## Question 6
How should the slicer locate the UVtools executable?

A) A setting in the slicer where the user points to the UVtools.Cmd path

B) Search the system PATH and common default install locations automatically

C) Both — auto-detect, with a manual override setting as fallback

D) Other (please describe after [Answer]: tag below)

[Answer]: D - download itself and remember where it is

## Question 7
How should a printer profile declare its target output format (the format UVtools converts to, e.g. CTB, GOO, PWS)?

A) A new config key on the SLA printer profile (e.g. a "target format" / UVtools format id + file extension)

B) Reuse the existing sla_archive_format key with a naming convention the pipeline recognizes

C) Encode it in the post-process command itself (only applies if Strategy B/Hybrid)

D) Other (please describe after [Answer]: tag below)

[Answer]: B - but each manufacturer might have more than one format.  Can even explore how UVTools labels them

---

## Section 4 — Behavior & Quality

## Question 8
If UVtools is missing, the wrong version, or conversion fails, what should happen?

A) Hard error — fail the export with a clear message; produce no output file

B) Fall back to leaving the intermediate .sl1 and warn the user that conversion didn't run

C) Make it configurable per the above (hard error by default, with an opt-in fallback)

D) Other (please describe after [Answer]: tag below)

[Answer]: B

## Question 9
Must the feature work in both the GUI and the CLI, or is one enough for this pass?

A) Both GUI and CLI must route through the conversion

B) CLI only is sufficient for this first pass

C) GUI only is sufficient for this first pass

D) Other (please describe after [Answer]: tag below)

[Answer]: C

## Question 10
What is the success/validation criterion for this feature (how will we know it's done)?

A) Slice a test model for a 3rd-party printer and confirm a valid manufacturer-format file is produced that the printer (or UVtools re-opening it) accepts

B) Compiles + produces a converted file of the right type; deeper print validation is manual/later

C) Full end-to-end: an actual print on a physical 3rd-party printer

D) Other (please describe after [Answer]: tag below)

[Answer]: A

## Question 11
Same guiding constraint as before — modify existing PrusaSlicer code as little as possible, and do NOT change the core SLA slicing algorithms. Confirm?

A) Yes — minimize changes; core SLA slicing is off-limits; prefer additive/new files

B) Some core changes are acceptable if clearly justified

C) Other (please describe after [Answer]: tag below)

[Answer]: A

---

## Section 5 — Extension Opt-Ins (AI-DLC)

## Question 12: Security Extensions
Should security extension rules be enforced for this project?

A) Yes — enforce all SECURITY rules as blocking constraints (recommended for production-grade applications)

B) No — skip all SECURITY rules (suitable for PoCs, prototypes, and experimental projects)

X) Other (please describe after [Answer]: tag below)

[Answer]: A

## Question 13: Resiliency Extensions
Should the resiliency baseline be applied to this project?

The resiliency baseline applies directional, design-time best practices for fault tolerance, availability, observability, and recoverability (from the AWS Well-Architected Reliability Pillar). It is a starting point, not a production-readiness guarantee. Note: this is a desktop C++ application, so cloud-oriented resiliency practices may have limited applicability.

A) Yes — apply the resiliency baseline as directional best practices and design-time guidance

B) No — skip the resiliency baseline (suitable for PoCs, prototypes, desktop tools, and experimental projects)

X) Other (please describe after [Answer]: tag below)

[Answer]: A

## Question 14: Property-Based Testing Extension
Should property-based testing (PBT) rules be enforced for this project?

A) Yes — enforce all PBT rules as blocking constraints (recommended for projects with business logic, data transformations, serialization, or stateful components)

B) Partial — enforce PBT rules only for pure functions and serialization round-trips (suitable for projects with limited algorithmic complexity)

C) No — skip all PBT rules (suitable for simple CRUD applications, UI-only projects, or thin integration layers with no significant business logic)

X) Other (please describe after [Answer]: tag below)

[Answer]: A

---

## Question 15 — Anything else
Any other constraints, preferences, or context I should factor in (e.g. specific UVtools version, target OS priority, how profiles should appear in the printer picker)?

[Answer]: no, go for it
