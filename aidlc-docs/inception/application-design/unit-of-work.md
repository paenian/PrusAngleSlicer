# Units of Work — Angled Slicing Feature

## Overview

The feature is decomposed into 5 units delivered in dependency order. Units 1–3 constitute the MVP. Units 4–5 are post-MVP enhancements.

```
U1: Config & Parameters
        │
        ▼
U2: Angled Slice Engine  ◄──────────── (core algorithm, all downstream depends on this)
        │
        ├──► U3: Preview Adaptation    [MVP]
        │
        ├──► U4: Support & Edge Cases  [Post-MVP]
        │
        └──► U5: Multi-Material        [Post-MVP, also depends on U4]
```

---

## Unit 1: Config & Parameters

**Status**: MVP  
**Goal**: Establish the configuration foundation. Everything downstream depends on the config being available and serializable.

### Responsibility
- Add `angled_slicing_angle` and `angled_slicing_direction` config options to `PrintObjectConfig`
- Register options in `PrintConfigDef` (label, tooltip, range, default)
- Wire into the invalidation system (`posSlice` on change)
- Implement `AngledSlicing::Params` value object that reads from config
- Add Print Settings UI section ("Angled Slicing") with the two inputs
- Ensure config serializes/deserializes in .3mf and print profiles
- Validate: angle range [0, 89], direction range [0, 360)

### Files Produced
| File | Type |
|------|------|
| `src/libslic3r/Feature/AngledSlicing/AngledSlicingParams.hpp` | New |
| `src/libslic3r/PrintConfig.hpp` | Modified (+4 lines) |
| `src/libslic3r/PrintConfig.cpp` | Modified (+30 lines) |
| `src/libslic3r/PrintObject.cpp` | Modified (+5 lines, invalidation hook) |
| `src/slic3r/GUI/Tab.cpp` | Modified (add UI section) |
| `src/libslic3r/CMakeLists.txt` | Modified (+2 lines) |

### Acceptance Gate
- Compile successfully with angle=0 (identity behavior)
- Config serializes to .3mf and restores correctly
- UI shows two numeric inputs in Print Settings
- Changing angle triggers `posSlice` invalidation

---

## Unit 2: Angled Slice Engine

**Status**: MVP  
**Goal**: The core algorithm. Produces correctly tilted layers as output of `posSlice`. All downstream pipeline stages work unchanged on the resulting Layer objects.

### Responsibility
- Implement `AngledSlicing::Transform` — builds `Transform3d` rotation from Params
- Implement `AngledSlicing::ZSchedule` — computes full Z-height schedule for tilted layers
- Implement `AngledSlicing::FirstLayer` — bed-plane clipping, first-layer tagging
- Hook into `PrintObject::slice_volumes()` — compose rotation into `MeshSlicingParams::trafo`, replace Z-heights, apply clipping
- Ensure `Layer::is_first_layer` is set correctly for multiple bed-contact layers
- Ensure backward compat: angle=0 produces bit-for-bit identical slicing
- Write unit tests and PBT tests for all geometric functions

### Files Produced
| File | Type |
|------|------|
| `src/libslic3r/Feature/AngledSlicing/AngledSlicingTransform.hpp` | New |
| `src/libslic3r/Feature/AngledSlicing/AngledSlicingTransform.cpp` | New |
| `src/libslic3r/Feature/AngledSlicing/AngledSlicingZSchedule.hpp` | New |
| `src/libslic3r/Feature/AngledSlicing/AngledSlicingZSchedule.cpp` | New |
| `src/libslic3r/Feature/AngledSlicing/AngledSlicingFirstLayer.hpp` | New |
| `src/libslic3r/Feature/AngledSlicing/AngledSlicingFirstLayer.cpp` | New |
| `src/libslic3r/PrintObject.cpp` | Modified (+15 lines, slice hook) |
| `src/libslic3r/CMakeLists.txt` | Modified (+4 lines) |
| `tests/libslic3r/test_angled_slicing.cpp` | New |

### Acceptance Gate
- Benchy sliced at 15° produces G-code with visibly different layer heights per Z level
- G-code contains only G0/G1 commands
- Angle=0 produces byte-identical output to stock slicing
- All PBT invariants pass (rotation round-trip, ascending Z schedule, etc.)

---

## Unit 3: Preview Adaptation

**Status**: MVP  
**Goal**: Visual feedback for the user. The preview should show tilted layer lines so users can verify the angle before printing.

### Responsibility
- Ensure the G-code preview layer color rendering reflects tilted layer boundaries
- Layer slider navigates through tilted layers correctly
- Multiple first-layer regions visually distinguished
- Print time and filament estimates updated to reflect actual tilted layer count

### Note on Implementation
PrusaSlicer's preview (`libvgcode`, `GCodeProcessor`) renders layers based on `Layer::print_z` and `Layer::height`, which are already correctly set by Unit 2. The visual diagonal effect may emerge automatically. This unit verifies that behavior and fixes any issues — it is primarily a verification + polish unit.

### Files Produced
| File | Type |
|------|------|
| `src/slic3r/GUI/3DScene.cpp` | Modified (if needed, minimal) |
| `src/libslic3r/GCode/GCodeProcessor.cpp` | Modified (if needed, minimal) |

### Acceptance Gate
- Preview shows diagonal layer band boundaries for angle > 0
- Layer slider correctly navigates tilted layers
- First-layer regions are visually distinct
- Time/filament estimates are plausible (accounting for 1/cos(angle) layer count increase)

---

## Unit 4: Support & Edge Cases

**Status**: Post-MVP  
**Goal**: Robust support material generation for angled prints, and defensive handling of edge cases.

### Responsibility
- Verify overhang detection works correctly relative to tilted layer planes
- Ensure brim/skirt generates on Z=0 plane correctly for multi-first-layer objects
- Add validator warnings: height overflow, extreme angle, variable layer height conflict, sequential print conflict
- Handle thin model edge case (model shorter than one full tilted layer)
- Handle very steep angle warning (> 60°)

### Files Produced
| File | Type |
|------|------|
| `src/libslic3r/Print.cpp` | Modified (validation warnings) |
| `src/libslic3r/PrintObject.cpp` | Modified (brim/support adjustments if needed) |
| `tests/libslic3r/test_angled_slicing.cpp` | Modified (edge case tests) |

### Acceptance Gate
- US-5 acceptance criteria all pass
- US-7 acceptance criteria all pass
- Validator produces appropriate warnings for edge cases
- Benchy with supports at 15° generates valid support structures

---

## Unit 5: Multi-Material

**Status**: Post-MVP  
**Goal**: Multi-color and multi-material prints work correctly with angled slicing.

### Responsibility
- Verify material boundary painting is correctly segmented in tilted layers
- Verify wipe tower Z heights align correctly with tilted layer Z values
- Validate multi-material cat model with chevron pattern at 10°
- Ensure tool change ordering is correct across tilted layers

### Files Produced
| File | Type |
|------|------|
| `src/libslic3r/GCode/WipeTower.cpp` | Modified (if needed) |
| `src/libslic3r/GCode/ToolOrdering.cpp` | Modified (if needed) |
| `tests/libslic3r/test_angled_slicing.cpp` | Modified (multi-material tests) |

### Acceptance Gate
- US-6 acceptance criteria all pass
- Multi-color cat with chevrons slices correctly at 10°
- Wipe tower operates at correct Z heights
- No material bleeding across boundaries in tilted layers
