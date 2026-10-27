# U1 Domain Entities — Config & Bundled Profiles

Technology-agnostic domain model for U1. U1 establishes the configuration vocabulary and the printer data that the rest of the feature depends on.

## E1 — SLA target-format identifier (value carried in `sla_archive_format`)
- **Form**: `uvtools:<EncoderName>[@<version>]` (per spike §2, Q4=A).
  - `EncoderName`: strict UVtools encoder class name without the trailing `File` is acceptable in profiles for readability, but the canonical stored/resolved value is the strict name (e.g. `ChituboxFile`, `GooFile`, `AnycubicFile`). U2 normalizes.
  - `version` (optional): integer format version passed to UVtools `-v` (e.g. CTB v3).
- **Non-`uvtools:` values** (e.g. `SL1`, `pwmo`): existing behavior, untouched.
- **Examples**: `uvtools:ChituboxFile@3`, `uvtools:GooFile`, `uvtools:AnycubicFile@517`.

## E2 — UVToolsPaths (the libslic3r↔app-config seam)
- **Fields**:
  - `override_path` (string, may be empty) — user-specified UVtools executable (global setting).
  - `cached_path` (string, may be empty) — remembered location after a successful acquire.
- **Ownership**: values live in global app config (Q3=B); this struct is the GUI/CLI→libslic3r carrier. libslic3r never reads AppConfig directly.
- **Persistence callback**: an injected writer stores an updated `cached_path` back to app config after acquisition.

## E3 — UVtools release pin (data)
- **Fields**: `version` (string, e.g. `5.x.y`), and per-arch `sha256` (x64, arm64), and the URL template.
- **Form**: a small pinned table shipped as data (config/resource), so a UVtools bump is a data change (spike §3, SECURITY-10/13).
- **URL template**: `https://github.com/sn4k3/UVtools/releases/download/v{version}/UVtools_osx-{arch}_v{version}.zip`.

## E4 — Bundled SLA printer profile (data, per model)
- **Fields** (existing SLA config keys + the new target-format value):
  - `printer_technology = SLA`, `printer_model`, `printer_vendor`
  - `bed_shape`, `max_print_height`
  - `display_width/height`, `display_pixels_x/y`, `display_orientation`, `display_mirror_x/y`
  - `sla_archive_format = uvtools:<EncoderName>[@version]` (E1)
- **Vendors this pass**: Anycubic, Phrozen, Elegoo (final model list pinned during implementation against the pinned UVtools version).

## Relationships
- E4 (profile) references E1 (its target format id).
- E2/E3 are global/app-level, not per-profile (one UVtools install per machine).
- E1 is consumed by U2 (mapping) and U4 (writer); E2/E3 by U3 (service).
