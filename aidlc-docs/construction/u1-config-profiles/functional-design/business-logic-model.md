# U1 Business Logic Model — Config & Bundled Profiles

U1 is mostly declarative (config schema + data), with a thin seam. Logic is minimal by design.

## Responsibilities
1. Permit `uvtools:*` values in `sla_archive_format` (no schema change needed — it is already `coString`; the value convention is the only new thing). (BR-1, BR-2)
2. Define global app-config entries for the UVtools executable: `uvtools_executable_path` (user override) and `uvtools_cached_path` (remembered). (BR-3, BR-6)
3. Define the pinned UVtools release data: version + per-arch SHA256 + URL template. (BR-4)
4. Provide the `UVToolsPaths` seam struct + a persistence callback type so libslic3r stays GUI-agnostic.
5. Ship bundled SLA profiles (Anycubic/Phrozen/Elegoo) as INI data. (BR-5)

## Key flows
### F1 — Build UVToolsPaths at the boundary
```
(GUI or CLI entry)
  read app config: uvtools_executable_path, uvtools_cached_path
  -> construct UVToolsPaths{ override_path, cached_path }
  -> pass into libslic3r (UVToolsService) when an export needs it
```

### F2 — Persist cached path after acquisition
```
UVToolsService acquires UVtools -> resolved path
  -> invoke persistence callback(resolved_path)
     -> boundary writes uvtools_cached_path to app config
```

### F3 — Resolve pinned release for current arch
```
detect arch (x64 | arm64)
  -> look up pinned { version, sha256[arch] }
  -> build URL from template(version, arch)
  (consumed by U3 acquire)
```

## Data vs code split
- **Data**: bundled profiles (INI), pinned release table (version + checksums + URL template).
- **Code (thin)**: config key registration; `UVToolsPaths` struct + callback typedef; arch detection + pin lookup helper.
- **No business algorithms** here; mapping (U2) and acquisition/invocation (U3) carry the real logic.

## Testable properties (PBT-01 for U1)
- `sla_archive_format` prefix check: `starts_with "uvtools:"` ⇔ routed to convert path (simple invariant; most mapping PBT lives in U2).
- Pin lookup: for every supported arch, `resolve(version, arch)` yields a URL + a non-empty checksum (totality over supported arches).
- Most of U1 is config/data with few algorithmic properties; richer PBT targets are in U2/U3 (noted for PBT-01 completeness).

## Integration points
- Existing config system (`PrintConfig` / AppConfig).
- Preset/profile loader (reads the new INIs).
- Hands E1/E2/E3 to U2 (mapping) and U3 (service).
