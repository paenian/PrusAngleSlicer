# Unit Dependency Matrix — 3rd-Party SLA Printer Support

## Dependency matrix
| Unit | Depends on | Depended on by | Rationale |
|---|---|---|---|
| U1 Config & Profiles | existing config/preset system | U2, U3, U4, U5 | Config keys, `uvtools:*` convention, and the `UVToolsPaths` seam must exist first; profiles reference the format ids |
| U2 Format Mapping | U1 (format-id convention) | U4 | Writer routes via the map |
| U3 UVtools Service | U1 (paths seam) | U4, U5 | Writer invokes it; GUI shows its status |
| U4 Convert Writer & Fallback | U2, U3, existing SL1 writer + registry | U5 (status surfacing) | Orchestrates map + service; registered in registry |
| U5 GUI Settings & Status | U1 (global config), U3 (progress), U4 (warnings) | — | User-facing surface over the rest |

## Implementation order (sequential — Q3=A)
```
U1  Config & Bundled Profiles
      |
      v
U2  SLA Format Mapping  ----\
      |                      \
      v                       >  U4  Convert Writer & Fallback  --> U5  GUI Settings & Status
U3  UVtools Service  --------/
```
- Critical path: U1 → (U2 & U3) → U4 → U5.
- U2 and U3 are independent after U1 (could parallelize), but will be done sequentially per Q3=A.

## Coordination / checkpoints
- **After U1**: config keys + seam compile; profiles load in the printer picker.
- **After U2**: mapping unit+property tests green.
- **After U3**: service can locate/acquire/invoke UVtools (integration test, possibly mocked); argument-construction property test green.
- **After U4**: end-to-end export produces a converted file on success and `.sl1`+warning on failure; existing outputs unchanged (regression check).
- **After U5**: GUI override + status + warning verified manually (macOS).

## Rollback
Additive throughout: revert new files (U2/U3/U4), the registry line (U4), config keys (U1), GUI additions (U5), and profile data (U1). No core-slicing changes to unwind.
