# U1 Tech Stack Decisions

| Concern | Decision | Rationale |
|---|---|---|
| Config storage | Reuse existing `PrintConfig` (`sla_archive_format` is already `coString`) + global AppConfig for UVtools paths | Minimal footprint (NFR-1); Q3=B global setting |
| Format-id convention | `uvtools:<EncoderName>[@version]` string | Unambiguous, strict-encoder targeting avoids UVtools shared-extension error (spike §1/§2) |
| Pinned release data | Static table (version + per-arch SHA256 + URL template) shipped as data/resource | Data-driven version bumps; SECURITY-10/13 |
| Seam | `UVToolsPaths` struct + persistence callback typedef in libslic3r | Keeps libslic3r GUI-agnostic (Q3=B consequence) |
| Profiles | INI under `resources/profiles/` (new `AnycubicSLA.ini` extension / `Phrozen.ini` / `Elegoo.ini` as needed) | Matches existing vendor-profile pattern; data-only |
| PBT framework | **Catch2 v3 generators + a small shrinking helper** (test-only) — REVISED from RapidCheck | See decision note below |
| Arch detection | Compile/runtime arch check (x64 / arm64) for pin lookup | macOS-first (CON-3), both Apple Silicon + Intel |

## Project-wide note / PBT framework decision (REVISED at T7)
Original choice was RapidCheck. On implementation (T7) this was reversed for concrete reasons:
- RapidCheck's test-framework integration targets **Catch2 v2**, but this repo uses **Catch2 v3.8** — RapidCheck does not plug into `TEST_CASE` here.
- RapidCheck is **not in the deps**; adding it means a FetchContent (needs network at configure) or vendoring a sizable library for a **test-only** concern — against the minimal-footprint constraint (NFR-1).

**Revised decision**: use **Catch2 v3 generators** (`GENERATE`, random generators, `Catch::getSeed()` for seed-based reproducibility — already present) plus a **small in-test shrinking helper** that, on a failing property, reduces the input toward a minimal counterexample and reports it with the seed. This satisfies the practical intent of PBT-09/PBT-08 (domain generators, seed reproducibility, CI integration, minimal reproducing case) using the framework already in the build. The gap vs RapidCheck is automatic/library-grade shrinking, which the helper approximates for these inputs (format-id strings, convert-arg paths).
This applies to U2/U3/U4 property tests. Recorded in aidlc-state.md.
