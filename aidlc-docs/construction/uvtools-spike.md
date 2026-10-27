# UVtools Integration Spike — Findings

Research-only spike (no code) to replace the UVtools placeholders carried from Inception. Feeds U1 (profile format ids), U2 (format map), U3 (service).

Sources: [sn4k3/UVtools CLAUDE.md](https://github.com/sn4k3/UVtools/blob/master/CLAUDE.md), [ConvertCommand.cs](https://github.com/sn4k3/UVtools/blob/master/UVtools.Cmd/Symbols/ConvertCommand.cs), [Homebrew cask uvtools](https://github.com/Homebrew/homebrew-cask) (download URL + checksum pattern). Content rephrased for licensing compliance.

## 1. CLI tool & convert syntax
- The CLI assembly is **`UVtoolsCmd`** (project `UVtools.Cmd`, built on `System.CommandLine`).
- Convert verb:
  ```
  UVtoolsCmd convert <input-file> <target-type/ext> <output-file> [-v|--version <n>] [--no-overwrite]
  ```
  - `<input-file>`: our intermediate `.sl1`.
  - `<target-type/ext>`: EITHER a **strict encoder class name** (e.g. `ChituboxFile`, `GooFile`, `AnycubicFile`) OR a file **extension** (e.g. `ctb`, `goo`) OR the literal `auto`.
  - `<output-file>`: destination path (its extension implies container where relevant).
  - `-v/--version`: sets the format version (several formats are versioned, e.g. CTB v2/v3/v4, Anycubic versions).
  - `--no-overwrite`: refuse to clobber an existing output.
- **Critical behavior**: if a bare extension maps to MORE THAN ONE encoder, UVtools errors out and prints the available encoder names, requiring the **strict encoder name** instead. → Our mapping MUST target strict encoder class names (+ version), not bare extensions, to be unambiguous. This directly confirms the Q4/Q7 design decision.
- **`auto` mode**: for an `.sl1`, UVtools reads a `FILEFORMAT_xxx` keyword stored in the sl1 (SL1File `Keyword_FileFormat`) and self-selects the target. Alternative path: we could embed `FILEFORMAT_<ext>` in the sl1 and call `convert <in> auto <out>` — but explicit strict-name targeting is more deterministic and is our primary approach.

## 2. Format identifiers (encoder class names)
- Formats are `*File` classes in `UVtools.Core/FileFormats/`, registered in `FileFormat.AvailableFormats`. Examples relevant to our 3 vendors:
  - **Chitubox / CTB** → `ChituboxFile` (extensions incl. `.ctb`, `.v2.ctb`, `.v3.ctb`; encrypted CTB is a version/variant). Widely used by Elegoo/Anycubic/others.
  - **GOO** → `GooFile` (`.goo`) — Elegoo Saturn/Mars 4-series native.
  - **Anycubic Photon Workshop** → `AnycubicFile` (`.pws`, `.pwmo`, `.pwmx`, `.pm3n`, etc.; versioned).
- Full canonical list is enumerable at runtime via `UVtoolsCmd convert <in> <bogus> <out>` (it prints "Available N encoders: Name (ext, ext...)") or `print-properties`. The exact per-model mapping for bundled profiles will be finalized in U1 against a pinned UVtools version.
- **Implication for `uvtools:*` convention (Q4=A)**: encode the **strict encoder name** and optional version, e.g.
  - `uvtools:ChituboxFile` (+ default/newest version) or `uvtools:ChituboxFile@3`
  - `uvtools:GooFile`
  - `uvtools:AnycubicFile@...`
  SLAFormatMap (U2) parses `uvtools:<EncoderName>[@<version>]` → `{encoder_name, version?, extension}` and builds the convert argument vector using the **strict name** form.

## 3. macOS acquisition (download + verify)
- Release artifacts (verified pattern from the Homebrew cask):
  ```
  https://github.com/sn4k3/UVtools/releases/download/v{version}/UVtools_osx-{arch}_v{version}.zip
  ```
  with `{arch}` ∈ {`x64`, `arm64`} (Apple Silicon = `arm64`). Linux/Windows have analogous `UVtools_linux-*`, `UVtools_win-*` artifacts (not needed this pass; macOS-first per CON-3).
- **Verification (SECURITY-13)**: GitHub releases publish per-asset SHA256 (Homebrew pins these), so U3 will: download over HTTPS → compute SHA256 → compare against a **pinned expected checksum for a pinned UVtools version** → only then unzip + execute. A version+checksum table lives in config/data (U1), so a UVtools bump is a data change.
- **Runtime note**: UVtools is a self-contained .NET app; the `osx-{arch}` zip bundles its runtime and the native `libcvextern.dylib` (EmguCV). The CLI entry is `UVtoolsCmd` inside the extracted bundle. No .NET install required on the user's machine.
- **Caching**: after verify, extract to the app data dir and remember the `UVtoolsCmd` path (the global cached-path setting, U1/C4).

## 4. License (CON-1 reinforced)
- Every UVtools source file carries the **AGPL-3.0** header. Confirms: we must NOT bundle/redistribute UVtools or link its code. Our posture — download the official release to the user's machine, verify, and invoke as a separate process — keeps it arm's-length. A short AGPL/source notice should accompany the download step in the GUI.

## 5. Impacts on unit designs
- **U1**: bundled profiles use `sla_archive_format = uvtools:<EncoderName>[@version]`; add a pinned UVtools version + per-arch SHA256 table (data).
- **U2 SLAFormatMap**: parse `uvtools:<EncoderName>[@<version>]`; produce `{encoder_name, version, extension, description}`; target the STRICT encoder name in the convert args (avoids the shared-extension ambiguity error). Totality/round-trip = PBT targets.
- **U3 UVToolsService**:
  - acquire: GitHub release URL by arch + version; SHA256 verify against pinned value; extract; cache `UVtoolsCmd` path.
  - invoke: argument vector `["convert", sl1, "<EncoderName>", out, "-v", version, "--no-overwrite"?]` — array form, no shell (SECURITY-05). Parse exit code + stderr → `ConvertResult`.
  - consider `auto` + embedded `FILEFORMAT_` as a fallback strategy only.
- **U4**: unchanged in shape; now has concrete convert invocation + result handling to implement the fallback.

## Open (to pin during U1/U3 implementation)
- Exact pinned UVtools version to target + its per-arch SHA256 values.
- Exact extension/version per bundled model (Anycubic/Phrozen/Elegoo) — enumerate against the pinned version.
- Confirm `UVtoolsCmd` invocation path/name inside the extracted macOS zip for both arches.
