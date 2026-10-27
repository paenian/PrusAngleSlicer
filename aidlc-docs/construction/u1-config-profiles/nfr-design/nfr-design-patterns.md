# U1 NFR Design Patterns — Config & Bundled Profiles

## Patterns applied
- **Dependency injection via seam (decoupling)**: `UVToolsPaths` + persistence callback injected from the GUI/CLI boundary into libslic3r. Keeps the core library free of GUI/AppConfig dependencies (NFR-5 maintainability; honors Q3=B).
- **Data-driven configuration (open/closed)**: pinned UVtools release table and printer profiles are data; adding printers or bumping UVtools requires no code change (NFR-5).
- **Fail-safe validation (security)**: `uvtools:*` grammar validated at the boundary of the convert path; malformed → explicit error, never silent mis-route (SECURITY-05).
- **Pinned trust anchor (supply chain/integrity)**: version + per-arch SHA256 are the sole accepted acquisition target; U3 verifies against them before execution (SECURITY-10/13).
- **Backward-compatible extension (resiliency)**: non-`uvtools:` values and existing profiles behave exactly as before — additive only.

## Security design notes
- No secrets in U1. Checksums are public integrity values, safe to commit.
- No network/subprocess in U1 itself; it only supplies the verified-acquisition inputs to U3.

## Validation
- Grammar validation is pure/total (PBT target). Pin lookup total over supported arches.
