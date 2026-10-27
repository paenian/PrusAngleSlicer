# U1 NFR Requirements — Config & Bundled Profiles

U1 is config + data + a thin seam, so most runtime NFRs (download/subprocess) belong to U3. U1's NFRs are about correctness, integrity of pinned data, and not regressing existing config.

## Security (enabled)
- **SECURITY-10 (supply chain)**: The pinned UVtools version + per-arch SHA256 are committed as data; they are the only accepted acquisition target (BR-4). No `latest`/floating reference. **Applicable — satisfied by pinned release data.**
- **SECURITY-13 (integrity)**: U1 provides the checksum data U3 uses to verify the downloaded artifact before execution. **Applicable — U1 supplies the pinned checksums.**
- **SECURITY-05 (input validation)**: The `uvtools:<EncoderName>[@version]` grammar is validated (BR-2); malformed values error rather than silently mis-route. **Applicable.**
- SECURITY-01/02/04/06/07/08/12/14 — **N/A** (no data store, network service, auth, web endpoint introduced by U1).
- **SECURITY-03 (logging)**: config/profile load issues logged without sensitive data. **Applicable (minor).**

## Resiliency (enabled, directional)
- Config additions are backward-compatible: existing profiles and formats behave unchanged (graceful, no breakage). 
- Pin lookup is total over supported arches; unknown arch yields a clear error (U3 consumes).

## Property-Based Testing (enabled, full) — framework decision (PBT-09)
- **Framework**: **RapidCheck** (C++ PBT), integrated with the existing **Catch2** test setup in `tests/libslic3r/`. Rationale: the codebase already uses Catch2; RapidCheck has a Catch2 integration, supports custom generators, shrinking, and seed reproducibility (PBT-08/09).
- **Dependency handling**: add RapidCheck as a test-only dependency (bundled under `bundled_deps/` or fetched pinned) wired into `tests/libslic3r/CMakeLists.txt`. Exact wiring finalized in Build & Test; must follow SECURITY-10 (pinned).
- **U1 PBT targets** (small): format-id prefix invariant; pin-lookup totality over arches (per PBT-01 list in business-logic-model.md).

## Maintainability
- UVtools version bump = data change (pin table). New bundled printer = INI data. These must not require touching U2/U3/U4 code.

## Performance / Availability / Scalability
- **N/A** at U1 — no runtime hot path, no load, no service. (Download/subprocess performance considered in U3.)
