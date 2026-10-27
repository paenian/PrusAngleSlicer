# Services & Orchestration — 3rd-Party SLA Printer Support

This feature has one orchestration flow (SLA export → convert) and one supporting service (UVtools acquisition). No server-side services; these are in-process libslic3r services.

## S1 — SLA Export/Convert Orchestration
- **Owner**: `SLAConvertArchiveWriter` (C1), reached via the existing seam `SLAPrint::export_print` → `SLAArchiveWriter::create("uvtools:...", cfg)`.
- **Trigger**: user exports in SLA mode with a profile whose `sla_archive_format` is `uvtools:<id>` (GUI this pass; CLI works via same seam — Assumption A-1).
- **Flow**:
  1. Rasterize + write intermediate `.sl1` (reuse SL1 path — core slicing unchanged).
  2. `SLAFormatMap::lookup(sla_archive_format)` → `{uvtools_id, extension}`.
  3. `UVToolsService::ensure_available(paths, progress)` → resolve/download/verify (S2).
  4. `UVToolsService::convert(sl1, final_path, uvtools_id, opts)`.
  5. On `Success`: remove intermediate; final manufacturer file remains at output path.
  6. On any non-success: leave `.sl1`; emit non-fatal warning via SLA export status channel (US-7).
- **Orchestration pattern**: synchronous within the background slicing/export thread; progress + warnings via existing status callbacks (no new threading model).

## S2 — UVtools Acquisition
- **Owner**: `UVToolsService` (C2).
- **Trigger**: lazily, the first time S1 needs UVtools and no verified executable is resolved (Q2=A).
- **Flow**: resolve override (global config) → else cached path → else download from official source (HTTPS) → verify integrity (checksum/signature) → cache resolved path (persist via app-config seam) → return path; progress reported throughout; typed failure on any problem.
- **Idempotence**: once cached+verified, subsequent runs skip download (NFR-3).

## Service interactions (sequence, text)
```
User (GUI) -> SLAPrint.export_print -> SLAConvertArchiveWriter.export_print
  SLAConvertArchiveWriter -> SL1 path: write intermediate .sl1
  SLAConvertArchiveWriter -> SLAFormatMap.lookup(archive_format) : {uvtools_id, ext}
  SLAConvertArchiveWriter -> UVToolsService.ensure_available(paths, progress)
      UVToolsService -> [resolve override/cache | download+verify] : exec path or error
  SLAConvertArchiveWriter -> UVToolsService.convert(sl1, dst, uvtools_id, opts) : ConvertResult
  alt Success:
      SLAConvertArchiveWriter -> filesystem: remove intermediate .sl1
      SLAConvertArchiveWriter -> status: "exported <manufacturer file>"
  else Failure:
      SLAConvertArchiveWriter -> status: non-fatal warning; keep .sl1
```

## Boundaries
- libslic3r services do not depend on GUI types; the resolved UVtools paths are injected (app-config seam) so both GUI and CLI can supply them.
- No network service, DB, or auth is introduced (SECURITY server-side rules N/A).
