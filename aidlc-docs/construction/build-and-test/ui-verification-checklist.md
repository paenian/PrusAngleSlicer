# UI Verification Checklist — 3rd-Party SLA Printer Support (T6)

PrusaSlicer's wxWidgets GUI has no in-repo automated rendering harness, and a
headless wx runner is fragile/unsupported here. So UI verification is split:

- **Automated (data/wiring level)** — `tests/libslic3r/test_uvtools_ui.cpp`:
  - Bundled 3rd-party SLA profiles (Elegoo/Phrozen) exist, declare `printer_technology = SLA`,
    carry build-volume + display keys, and set a well-formed `uvtools:*` archive
    format that `SLAFormatMap::lookup` resolves to a real encoder + extension.
  - `uvtools_executable_path` / `uvtools_cached_path` round-trip through AppConfig
    (what the Preferences field reads/writes).
- **Manual (rendered UI)** — this checklist, run on a build with a display.

## Manual steps

### A. Preferences field (T5)
1. Launch PrusaSlicer (editor). Open Preferences.
2. Go to the **Other** tab.
3. CONFIRM: a text field labeled **"UVtools executable path (SLA conversion)"** is present, with a tooltip mentioning auto-download + AGPL.
4. Enter a path (e.g. `/opt/uvtools/UVtoolsCmd`); click OK.
5. Reopen Preferences > Other. CONFIRM: the value persisted.
6. CONFIRM in `PrusaSlicer.ini`: `uvtools_executable_path = /opt/uvtools/UVtoolsCmd` under `[app]`/root.
7. Clear the field; OK; reopen. CONFIRM: empty again (auto-download mode).

### B. 3rd-party SLA printer profiles (US-1)
1. Switch to SLA mode (or run the Configuration Wizard).
2. CONFIRM: Elegoo (Saturn 4 Ultra / Mars 4) and Phrozen (Sonic Mini 8K / Mega 8K) SLA printers are offered / selectable.
3. Select one. CONFIRM: Printer Settings shows the expected bed size, display resolution, and max height.

### C. End-to-end export + conversion (US-3 / US-5 / US-7) + logging (T4)
1. With a 3rd-party SLA profile selected and NO UVtools installed and NO override set:
   - Slice a model, Export.
   - CONFIRM: UVtools auto-download begins (progress visible); on success, a manufacturer file (e.g. `.goo`/`.ctb`) is written.
   - CONFIRM in the application log: `[UVtools]` lines for download -> verify -> extract -> convert, and a success line with the output path.
2. Point the override at a valid local `UVtoolsCmd`; Export again.
   - CONFIRM: no download; conversion uses the override; manufacturer file produced.
3. Point the override at an invalid path AND disable network; Export.
   - CONFIRM (US-7 fallback): no crash; a `.sl1` is left at the output location; a clear warning is shown; the log has `[UVtools]` error lines explaining why.

### D. Regression
1. Select a Prusa SL1/SL1S printer; Export. CONFIRM: normal `.sl1` output, unchanged behavior.

## Status
- Automated data/wiring tests: implemented (`test_uvtools_ui.cpp`).
- Manual rendered-UI checklist: to be run on a GUI session (macOS verified build available).

### E. UVtools download dialog (added after live GUI test)
Preconditions: a Phrozen/Elegoo SLA printer selected; UVtools NOT installed
(`rm -rf ~/Library/Application\ Support/PrusaSlicer/uvtools` and clear
`uvtools_cached_path` / `uvtools_executable_path` in PrusaSlicer.ini).
1. Slice, click Export. CONFIRM: before the save dialog, a modal "UVtools required for this printer" appears showing the version, ~75 MB, the install folder, and the AGPL note.
2. Click **Download and install**. CONFIRM: progress bar + phase text advance; **Cancel** stops it and returns to the first view ("Download cancelled.").
3. Let it finish. CONFIRM: dialog closes, the save dialog offers `.ctb`, and `<data dir>/uvtools/v7.1.2/UVtools.app/Contents/MacOS/UVtoolsCmd` exists; `uvtools_cached_path` is set in PrusaSlicer.ini.
4. Export again. CONFIRM: no dialog this time (already linked) and a `.ctb` is produced.
5. Reset, click Export, choose **Use my own UVtools...**, pick a UVtoolsCmd. CONFIRM: dialog closes and `uvtools_executable_path` is set.
6. Reset, choose **Export .sl1 instead**. CONFIRM: export proceeds and leaves a plain `.sl1` plus a notification explaining why.
7. Disconnect the network, choose Download. CONFIRM: a failure view shows the reason with **Retry / Use my own / Export .sl1 / Cancel**.

### F. Sonic Mega 8K S profile refresh (vendor config 1.0.1)
1. Start the app. CONFIRM: a configuration update offering "Phrozen SLA" 1.0.1 appears (the installed copy is 1.0.0). Accept it. If it does not appear, run the Configuration Wizard once and finish it.
2. Select the Phrozen Sonic Mega 8K S. CONFIRM: layer-height presets 0.025 Fine / 0.05 Normal / 0.1 Draft are offered and the Anycubic ABS-Like Grey Resin stays compatible with all three.
3. Export. CONFIRM: the save dialog offers "Phrozen PRZ files (*.prz)" and the result is a `.prz`.
4. Open the `.prz` in UVtools (or print-properties). CONFIRM: 7680x4320, 330x185 mm, lift/retract as set in the material notes.
