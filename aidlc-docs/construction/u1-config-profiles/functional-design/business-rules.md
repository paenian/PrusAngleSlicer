# U1 Business Rules — Config & Bundled Profiles

## BR-1 — Format-id prefix routing
- Any `sla_archive_format` value beginning with `uvtools:` (case-insensitive prefix) is a convert target routed to the new path.
- Any other value retains existing behavior (SL1, SL1SVG, Anycubic native, etc.). U1 must not change how non-`uvtools:` values behave.

## BR-2 — Format-id grammar
- Grammar: `uvtools:` `<EncoderName>` ( `@` `<version>` )?
  - `EncoderName`: non-empty, alphanumeric (UVtools class name, with or without trailing `File`).
  - `version`: optional positive integer.
- A malformed `uvtools:` value (empty encoder, bad version) is invalid → surfaced as a config/export error (not a silent wrong conversion). (Validation detail lands in U2; U1 only defines the grammar.)

## BR-3 — UVtools path resolution precedence (defines the data; logic in U3)
- Effective UVtools executable = `override_path` if non-empty and valid; else `cached_path` if non-empty and valid; else acquire.
- U1 provides the storage for `override_path` and `cached_path` as a global app setting and the `UVToolsPaths` seam.

## BR-4 — Release pin integrity
- The pinned UVtools `version` and its per-arch `sha256` are the single source of truth for acquisition verification (U3). Changing the targeted UVtools version means updating this data (version + checksums together).
- No acquisition may proceed against a version lacking a pinned checksum.

## BR-5 — Profile validity
- A bundled SLA profile must set `printer_technology = SLA`, a valid `bed_shape`/`max_print_height`/display geometry (existing soft minimums), and a `sla_archive_format` that is either a known existing format or a well-formed `uvtools:<EncoderName>[@version]`.
- Arbitrary build-volume sizes are permitted (existing schema minimums only) — supports custom printers (US-2).

## BR-6 — No per-printer UVtools path
- The UVtools executable override is global (Q3=B); profiles must NOT carry a per-printer UVtools path key.
