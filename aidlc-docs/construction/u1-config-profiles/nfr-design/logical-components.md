# U1 Logical Components — Config & Bundled Profiles

| Component | Kind | Responsibility | NFR tie-in |
|---|---|---|---|
| `uvtools_executable_path`, `uvtools_cached_path` | Global AppConfig keys | Store user override + remembered UVtools path | Q3=B; feeds BR-3 resolution |
| `UVToolsPaths` struct | libslic3r seam type | Carry override+cached into the service | DI/decoupling (maintainability) |
| UVtools-cached-path persistence callback | typedef/interface | Write cached path back after acquire | Decoupling; keeps libslic3r GUI-agnostic |
| UVtools release pin table | Data (resource/config) | version + per-arch SHA256 + URL template | SECURITY-10/13 |
| Arch + pin lookup helper | Thin libslic3r util | Resolve URL + checksum for current arch | Totality property |
| Bundled profiles | INI data | Anycubic/Phrozen/Elegoo presets with `uvtools:*` targets | FR-2, backward-compatible |

## No infrastructure components
No queues/caches/circuit-breakers/etc. — U1 is config + data + a seam. (Infrastructure Design stage is SKIP for the whole feature.)

## Interfaces handed to other units
- U2 consumes the `uvtools:<EncoderName>[@version]` grammar (E1).
- U3 consumes `UVToolsPaths` (E2), the pin table + arch lookup (E3), and the persistence callback.
- U5 (GUI) reads/writes the global config keys and populates `UVToolsPaths`.
