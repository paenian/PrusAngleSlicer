# Personas — 3rd-Party SLA Printer Support

Four personas: three resin-printer users by experience level, plus the maintainer/developer. Each drives a different set of needs around selecting a printer, exporting to a manufacturer format, and dealing with the UVtools dependency.

---

## P1 — Nadia, the Novice Resin User
- **Experience**: New to resin printing; owns one 3rd-party printer (e.g. an Elegoo Mars or Anycubic Photon). Comfortable with slicer basics but not with file formats or command lines.
- **Goals**: Pick her printer from a list, slice a model, and get a file her printer will actually print. No fuss.
- **Frustrations**: Doesn't know what "CTB" or "sl1" mean; terrified of settings she doesn't understand; gives up if something errors cryptically.
- **Needs from this feature**:
  - Her printer available as a ready-made profile (correct build volume + output format preset).
  - Export "just works" and produces the right file type without her choosing a format manually.
  - If something goes wrong, a plain-language message and a file she can still use or hand to another tool.
- **Tech comfort**: Low-to-medium. Will never edit a config file or install a tool by hand.

## P2 — Ian, the Intermediate Resin User
- **Experience**: A year or two of resin printing; owns one or two printers, maybe across brands. Knows his printer's format and build volume; has used other slicers.
- **Goals**: Use PrusaSlicer for resin and still get his brand's native file. Tweak build volume / exposure when needed.
- **Frustrations**: Having to round-trip through a separate conversion tool manually; profiles that don't match his exact model; silent failures.
- **Needs from this feature**:
  - Bundled profiles for common models, but also the ability to adjust build volume / display settings for his exact unit.
  - A clear indication of which output format a profile targets.
  - Confidence that the conversion happened (or a clear warning if it didn't).
- **Tech comfort**: Medium. Will read a tooltip, won't write scripts.

## P3 — Priya, the Advanced / Power User
- **Experience**: Deep resin experience; multiple printers; already uses UVtools directly; may script her workflow.
- **Goals**: Full control. Wants PrusaSlicer to use her existing UVtools install and specific version, and to define fully custom printers with arbitrary build volumes.
- **Frustrations**: Tools that force their own bundled/downloaded binary when she already manages one; black boxes she can't point at a known path; being unable to define a non-standard printer.
- **Needs from this feature**:
  - Define a completely custom SLA printer with arbitrary build volume and the right target format.
  - Override the auto-downloaded UVtools and point the slicer at her own install.
  - Deterministic, inspectable behavior (knows where the intermediate/output files are, what UVtools was invoked).
- **Tech comfort**: High. Comfortable with paths, versions, and external tools.

## P4 — Marco, the Maintainer / Developer
- **Experience**: Maintains this PrusaSlicer fork. Cares about code isolation, licensing, and not breaking existing behavior.
- **Goals**: Add 3rd-party SLA support with minimal, additive changes; keep core SLA slicing untouched; respect UVtools' AGPL by never bundling it; keep the feature testable.
- **Frustrations**: Invasive changes that ripple through core slicing; legal risk from bundling AGPL code; untested glue code; breaking the existing SL1/Anycubic outputs.
- **Needs from this feature**:
  - New code isolated in its own modules; no edits to core rasterization.
  - UVtools acquired at arm's length (download + verify, never redistributed).
  - Mapping and command-construction logic covered by tests; existing outputs unchanged.
- **Tech comfort**: Expert (C++, build system, the codebase).

---

## Persona → Primary Concerns Matrix
| Concern | Nadia (novice) | Ian (intermediate) | Priya (advanced) | Marco (maintainer) |
|---|---|---|---|---|
| Pick a bundled printer | High | High | Medium | — |
| Define custom/arbitrary printer | Low | Medium | High | — |
| One-click export to manufacturer format | High | High | Medium | — |
| Auto-download of UVtools | High (invisible) | Medium | Low (prefers own) | High (license-safe) |
| Manual UVtools override | — | Low | High | Medium |
| Clear failure fallback + warning | High | High | Medium | Medium |
| Minimal/additive code, core untouched | — | — | — | High |
