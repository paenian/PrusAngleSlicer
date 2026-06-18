# Unit of Work Story Map — Angled Slicing Feature

## Story-to-Unit Assignment

| Story | Title | Unit | Rationale |
|-------|-------|------|-----------|
| US-4 | Backward Compatibility | U1 + U2 | Config default (U1) + slice identity at 0° (U2) |
| US-2 | Angle Configuration UI | U1 | Config options and UI panel |
| US-1 | Core Angled Slicing | U2 | Core algorithm in posSlice |
| US-3 | Preview Visualization | U3 | Visual layer rendering |
| US-5 | Support Material Compat | U4 | Overhang detection + brim |
| US-7 | Edge Case & Error Handling | U4 | Validation and warnings |
| US-6 | Multi-Material Compat | U5 | Wipe tower + material boundaries |

---

## Acceptance Criteria Traceability

### US-4 → U1 + U2

| Scenario | Unit | Acceptance Criterion |
|----------|------|---------------------|
| Zero angle = standard slicing | U2 | `angle_params.enabled() == false` → skip all new code paths; slicing is byte-identical |
| Legacy project files load | U1 | `angled_slicing_angle` defaults to 0.0 when missing from .3mf; no error |
| No test regressions | U2 | Existing test suite passes with no modifications |

### US-2 → U1

| Scenario | Unit | Acceptance Criterion |
|----------|------|---------------------|
| Angle parameter visible | U1 | Print Settings → Layers section → "Slicing Angle" numeric input, range [0, 89], default 0 |
| Direction parameter visible | U1 | "Tilt Direction" input, range [0, 360), default 0 |
| Config persistence | U1 | Values survive profile save/load |
| 3MF roundtrip | U1 | Values survive .3mf save/load |

### US-1 → U2

| Scenario | Unit | Acceptance Criterion |
|----------|------|---------------------|
| Basic angled slice | U2 | Tilted rotation composed into `MeshSlicingParams::trafo`; layers at angle-correct Z heights |
| Multiple first layers | U2 | `LayerZInfo::is_first_layer` set for all layers intersecting Z≤0; first-layer settings applied |
| Geometry preservation | U2 | Outer envelope of G-code identical to standard slicing within ±0.1mm |
| Standard G-code output | U2 | G-code contains only G0/G1/G28/M-codes; no rotary axis commands |

### US-3 → U3

| Scenario | Unit | Acceptance Criterion |
|----------|------|---------------------|
| Layer coloring follows tilt | U3 | Layer color bands are visually diagonal in preview viewport |
| Layer slider navigation | U3 | Slider steps through tilted layers; highlighted cross-section is tilted |
| Multiple bed-contact layers | U3 | Multiple layers at bottom show first-layer distinction in preview |

### US-5 → U4

| Scenario | Unit | Acceptance Criterion |
|----------|------|---------------------|
| Overhang detection relative to tilt | U4 | Support threshold applied relative to tilted layer plane orientation |
| Support reaches build plate | U4 | Support pillars extend to Z=0 |
| Brim at Z=0 | U4 | Brim/skirt generated on bed plane, surrounding full angled-object footprint |

### US-7 → U4

| Scenario | Unit | Acceptance Criterion |
|----------|------|---------------------|
| Extreme angle warning | U4 | Warning displayed when angle > 60° |
| Height overflow check | U4 | Warning when `model_height / cos(angle)` > printer max Z |
| Thin model edge case | U4 | At least 1 valid layer produced; no crash |
| Angle=0 performance | U2 | No overhead when disabled (verified in U2 unit tests) |

### US-6 → U5

| Scenario | Unit | Acceptance Criterion |
|----------|------|---------------------|
| Material boundaries preserved | U5 | Per-layer material segmentation correct in tilted slices |
| Wipe tower Z alignment | U5 | Tool changes at correct Z heights per tilted layer |
| Multi-color validation model | U5 | Cat with chevrons slices correctly at 10° |

---

## Persona Coverage by Unit

| Unit | Alex (Hobbyist) | Sam (Engineer) | Jordan (Creative) | Paul (Ancient) |
|------|:---:|:---:|:---:|:---:|
| U1: Config | x | x | | x |
| U2: Engine | x | x | x | x |
| U3: Preview | x | x | x | x |
| U4: Support | x | x | | x |
| U5: Multi-Mat | | x | x | x |

---

## MVP Completeness Check

MVP is declared when U1 + U2 + U3 are complete and the following are verified:

- [ ] Benchy sliced at 15° produces valid G-code with tilted layer lines (US-1, US-4)
- [ ] User can configure angle and direction in Print Settings (US-2)
- [ ] Preview shows diagonal layer bands (US-3)
- [ ] Angle=0 is byte-identical to stock (US-4)
- [ ] G-code loads in G-code viewer without errors (US-1)
- [ ] Existing unit test suite passes (US-4)
