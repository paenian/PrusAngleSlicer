# Story → Unit Map — 3rd-Party SLA Printer Support

## Mapping
| Story | Primary unit | Supporting units | Notes |
|---|---|---|---|
| US-1 Select bundled 3rd-party printer | U1 (profiles) | U2 (format id resolves) | Profiles expose Anycubic/Phrozen/Elegoo with `uvtools:*` targets |
| US-2 Define custom arbitrary-size printer | U1 (config) | U2 (target-format mapping) | Existing schema allows arbitrary sizes; target set via `sla_archive_format` |
| US-3 Export directly to manufacturer format | U4 (writer) | U2 (map), U3 (convert) | Core happy path |
| US-4 Existing outputs unaffected | U4 (isolation) | — | Registry entry only; byte-compatible existing writers |
| US-5 UVtools auto-acquired on first use | U3 (acquire) | U1 (paths seam), U5 (progress) | Lazy download + verify + cache |
| US-6 Override UVtools with own install | U1 (global config), U5 (field) | U3 (uses path) | Global setting (Q3=B) |
| US-7 Graceful fallback + warning | U4 (fallback) | U3 (failure status), U5 (surfacing) | Keep `.sl1`, non-fatal warning |

## Coverage check
- All 7 stories (US-1..US-7) assigned. ✔
- Every unit carries at least one story. ✔
- Acceptance-spine stories US-3 + US-7 both land primarily in U4 (writer), integrating U2 + U3. ✔

## Per-unit story summary
- **U1**: US-1, US-2 (config), US-6 (config)
- **U2**: US-1, US-2, US-3 (routing)
- **U3**: US-5, US-6 (uses path)
- **U4**: US-3, US-4, US-7
- **U5**: US-6, US-7 (surfacing)
