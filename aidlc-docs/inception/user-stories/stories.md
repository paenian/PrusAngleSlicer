# User Stories — 3rd-Party SLA Printer Support

**Breakdown**: Feature-based with persona mapping. Each story follows INVEST, has Given/When/Then acceptance criteria, lists the personas it serves, and traces to requirement IDs in `requirements.md`.

Personas: P1 Nadia (novice), P2 Ian (intermediate), P3 Priya (advanced), P4 Marco (maintainer).

---

## Epic A — Printer selection & definition

### US-1 — Select a bundled 3rd-party SLA printer
**As** a resin printer owner, **I want** my printer available as a ready-made profile, **so that** I can slice for it without configuring build volume or output format by hand.
- **Personas**: P1, P2, P3
- **Traces**: FR-2, FR-1
- **Acceptance criteria**:
  - **Given** the slicer is in SLA mode, **when** I open the printer picker, **then** I see bundled profiles for Anycubic, Phrozen, and Elegoo models.
  - **Given** I select a bundled profile, **when** it loads, **then** the build volume, display resolution, and target output format are pre-set correctly for that model.
  - **Given** a bundled profile is selected, **when** I inspect it, **then** it is marked as an SLA (resin) printer and shows which manufacturer format it targets.

### US-2 — Define a custom SLA printer with an arbitrary build volume
**As** an advanced resin user, **I want** to define a custom SLA printer with an arbitrary build volume and display geometry, **so that** I can use a printer that isn't in the bundled list.
- **Personas**: P3, P2
- **Traces**: FR-1, FR-5
- **Acceptance criteria**:
  - **Given** I create/edit an SLA printer profile, **when** I set a non-standard bed size, max height, and display resolution, **then** the slicer accepts the values (subject only to existing soft minimums) and slices against them.
  - **Given** a custom profile, **when** I set its target output format identifier, **then** export uses that format.
  - **Given** a custom profile with no target format set, **when** I export, **then** I get the default SL1 output (no silent wrong-format conversion).

---

## Epic B — Export & format conversion (core)

### US-3 — Export directly to a manufacturer format
**As** a resin printer owner, **I want** exporting to produce my printer's native file format directly, **so that** I don't have to run a separate conversion tool myself.
- **Personas**: P1, P2, P3
- **Traces**: FR-3, FR-5, FR-7, NFR-1
- **Acceptance criteria**:
  - **Given** a 3rd-party SLA printer profile with a target format, **when** I slice a model and export, **then** the output file is in that manufacturer format and opens successfully in UVtools.
  - **Given** the export runs, **when** conversion succeeds, **then** only the final manufacturer file remains at my chosen path (the intermediate `.sl1` is removed).
  - **Given** a profile whose target is a format with multiple variants for that manufacturer, **when** I export, **then** the correct variant (per the profile's identifier) is produced.
  - **Given** the core SLA slicing pipeline, **when** this feature is used, **then** slicing output (rasters) is identical to today — no change to core slicing.

### US-4 — Existing SLA outputs are unaffected
**As** a maintainer, **I want** the existing SL1/SL1S/SVG/Anycubic outputs and core slicing to behave exactly as before, **so that** adding this feature introduces no regressions.
- **Personas**: P4
- **Traces**: NFR-1, CON-2
- **Acceptance criteria**:
  - **Given** a profile using an existing format (e.g. SL1), **when** I export, **then** the result is byte-compatible with the current behavior.
  - **Given** the new code, **when** the project builds, **then** no core SLA slicing files are modified and new logic lives in isolated modules.

---

## Epic C — UVtools acquisition (essential supporting flow)

### US-5 — UVtools is acquired automatically on first use
**As** a novice user, **I want** the slicer to obtain the conversion tool automatically the first time it's needed, **so that** I don't have to find, download, or install anything myself.
- **Personas**: P1, P2, P4
- **Traces**: FR-4, NFR-2 (SECURITY-10/13), CON-1
- **Acceptance criteria**:
  - **Given** UVtools is not present, **when** I export to a manufacturer format for the first time, **then** the slicer downloads UVtools from its official source over a secure connection and shows progress.
  - **Given** a download completes, **when** the binary is verified (checksum/signature), **then** it is used; **if** verification fails, **then** it is not executed and I'm warned (fallback per US-7).
  - **Given** UVtools was obtained once, **when** I export again later, **then** the slicer reuses the remembered location without re-downloading.
  - **Given** UVtools is downloaded, **when** it is stored, **then** it is kept in the app's data area and is never redistributed as part of the slicer (AGPL-safe, arm's-length).

---

## Epic D — Power-user control (supporting flow)

### US-6 — Override UVtools with my own install
**As** an advanced user, **I want** to point the slicer at my own UVtools executable, **so that** it uses my preferred install/version instead of auto-downloading.
- **Personas**: P3, P4
- **Traces**: FR-4
- **Acceptance criteria**:
  - **Given** a UVtools path setting, **when** I set it to a valid UVtools executable, **then** the slicer uses that executable for conversion and does not auto-download.
  - **Given** an override path that is invalid/missing, **when** I export, **then** I'm warned and the slicer falls back (auto-download if allowed, otherwise the `.sl1` fallback per US-7).
  - **Given** an override is set, **when** I inspect the setting, **then** the resolved UVtools location in use is visible to me.

---

## Epic E — Failure handling (essential supporting flow)

### US-7 — Graceful fallback when conversion can't run
**As** a resin printer owner, **I want** a clear warning and a still-usable file when the conversion can't be done, **so that** a failure never leaves me with nothing.
- **Personas**: P1, P2, P3, P4
- **Traces**: FR-6, NFR-2 (SECURITY-15), NFR-3
- **Acceptance criteria**:
  - **Given** UVtools is missing and cannot be obtained, or is the wrong version, or conversion fails, **when** I export, **then** the export does not hard-fail/crash.
  - **Given** such a failure, **when** it occurs, **then** the intermediate `.sl1` file is left at my output location and I see a clear warning explaining that the manufacturer-format conversion did not run (with the reason where available).
  - **Given** a failure path, **when** it completes, **then** no partial/corrupt manufacturer file is left behind and temporary resources are cleaned up.

---

## Traceability summary
| Story | Requirements | Personas |
|---|---|---|
| US-1 | FR-2, FR-1 | P1, P2, P3 |
| US-2 | FR-1, FR-5 | P3, P2 |
| US-3 | FR-3, FR-5, FR-7, NFR-1 | P1, P2, P3 |
| US-4 | NFR-1, CON-2 | P4 |
| US-5 | FR-4, NFR-2, CON-1 | P1, P2, P4 |
| US-6 | FR-4 | P3, P4 |
| US-7 | FR-6, NFR-2, NFR-3 | P1, P2, P3, P4 |

**Note on validation (per success criteria Q10)**: US-3 + US-7 together are the acceptance spine — a valid manufacturer file on success, a safe `.sl1` fallback with warning otherwise.


---

## Epic F — Novice onboarding (added after first end-to-end pass)

### US-13 — Add a non-Prusa SLA printer from the wizard, ready to print
**As** Nadia (novice), **I want** to pick a non-Prusa SLA printer (e.g. Phrozen Sonic Mega 8K S) from the "Add printer" wizard and have everything set up automatically, **so that** I can slice and export to my printer's format without manual configuration.
- **Personas**: P1, P2
- **Traces**: FR-1, FR-2, FR-3, FR-5 (+ US-1, US-3)
- **Acceptance criteria**:
  - **Given** the Config Wizard / Add-printer flow, **when** I browse SLA printers, **then** Phrozen (and Elegoo) 3rd-party SLA printers appear with a picture and are selectable (vendor `repo_id = non-prusa-sla`, bundled `.ini` + `.idx` + model `*_thumbnail.png`).
  - **Given** I select the Phrozen Sonic Mega 8K S, **when** it is added, **then** the app activates the SLA workflow automatically (printer_technology = SLA) and the UVtools target format is pre-configured (`sla_archive_format = uvtools:ChituboxFile@4`).
  - **Given** the printer is selected, **when** I look at materials, **then** an "Anycubic ABS-Like Grey Resin" SLA material is installed and set as the printer's default.
  - **Given** I slice a model and export, **when** UVtools is available, **then** a valid CTB file is produced with the correct 8K specs (7680×4320, 330×185×300) — verified end-to-end; on failure, the `.sl1` fallback applies (US-7).

**Status**: Data + wiring implemented (PhrozenSLA.ini/.idx + thumbnails, ElegooSLA.ini/.idx + thumbnails; Anycubic ABS-like grey resin default material). Conversion verified end-to-end for the Mega 8K S (slice -> .sl1 -> CTB v4, correct specs, re-opens in UVtools). Config Wizard appearance requires no C++ change (confirmed via context analysis of ConfigWizard BundleMap::load + repo_id gate). Rendered-wizard appearance to be confirmed on a GUI session per ui-verification-checklist.md.
