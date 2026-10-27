# U1 Tech Stack Decisions

| Concern | Decision | Rationale |
|---|---|---|
| Config storage | Reuse existing `PrintConfig` (`sla_archive_format` is already `coString`) + global AppConfig for UVtools paths | Minimal footprint (NFR-1); Q3=B global setting |
| Format-id convention | `uvtools:<EncoderName>[@version]` string | Unambiguous, strict-encoder targeting avoids UVtools shared-extension error (spike §1/§2) |
| Pinned release data | Static table (version + per-arch SHA256 + URL template) shipped as data/resource | Data-driven version bumps; SECURITY-10/13 |
| Seam | `UVToolsPaths` struct + persistence callback typedef in libslic3r | Keeps libslic3r GUI-agnostic (Q3=B consequence) |
| Profiles | INI under `resources/profiles/` (new `AnycubicSLA.ini` extension / `Phrozen.ini` / `Elegoo.ini` as needed) | Matches existing vendor-profile pattern; data-only |
| PBT framework | RapidCheck + Catch2 (test-only, pinned) | Project-wide choice (PBT-09); aligns with existing Catch2 tests |
| Arch detection | Compile/runtime arch check (x64 / arm64) for pin lookup | macOS-first (CON-3), both Apple Silicon + Intel |

## Project-wide note
The PBT framework (RapidCheck) is selected here at the first unit and applies to U2/U3/U4 as well. Recorded in aidlc-state.md Extension Configuration notes.
