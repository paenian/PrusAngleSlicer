# Component Dependencies — Angled Slicing Feature

## Dependency Matrix

| Component | Depends On | Used By |
|-----------|-----------|---------|
| `AngledSlicing::Params` | `PrintObjectConfig` (read-only) | Transform, ZSchedule, FirstLayer, slice_volumes() |
| `AngledSlicing::Transform` | `Params`, Eigen | slice_volumes() |
| `AngledSlicing::ZSchedule` | `Params`, `SlicingParameters` | slice_volumes() |
| `AngledSlicing::FirstLayer` | `Params`, `LayerZInfo`, `ExPolygon` | slice_volumes() |
| `PrintObjectConfig` (modified) | PrintConfigDef (existing) | All pipeline stages (existing, unchanged) |
| `PrintObject::slice_volumes()` (modified) | All 4 new components + existing deps | `PrintObject::make_perimeters()` (trigger) |

## Dependency Graph

```
PrintObjectConfig  ──────────────────────────┐
      │                                       │
      ▼                                       │
AngledSlicing::Params                         │
      │                                       │
      ├──────────────────────────────────┐    │
      │                                  │    │
      ▼                                  ▼    ▼
AngledSlicing::Transform    AngledSlicing::ZSchedule
      │                               │
      │                               │ (produces LayerZInfo[])
      │                               │
      └──────────────┬────────────────┘
                     │
                     ▼
          AngledSlicing::FirstLayer
                     │
                     ▼
          PrintObject::slice_volumes()
             (existing, minimally modified)
                     │
                     ▼
          TriangleMeshSlicer::slice_mesh_ex()
             (existing, UNCHANGED)
                     │
                     ▼
          Layer[] with correct ExPolygons
                     │
                     ▼
          Existing downstream pipeline:
          make_perimeters() → infill() → supports() → GCode
          (ALL UNCHANGED)
```

## Communication Patterns

### Data Flow (slice_volumes)
```
1. PrintObjectConfig → AngledSlicing::Params (value copy, read-only config)
2. Params → Transform::build_rotation() → Transform3d (pure function, no state)
3. Params + SlicingParameters → ZSchedule::compute() → vector<LayerZInfo> (pure function)
4. Transform3d composed into existing MeshSlicingParams::trafo (existing field)
5. vector<float> slice_zs derived from LayerZInfo (replaces standard zs)
6. slice_mesh_ex(mesh, zs, params) called (EXISTING FUNCTION, unchanged signature)
7. FirstLayer::clip_to_bed() applied to clipped-layer ExPolygons (pure function)
8. Layer::is_first_layer flags set from LayerZInfo
```

### Coupling Assessment

| Coupling | Type | Risk |
|----------|------|------|
| Params ↔ PrintObjectConfig | Read-only config access | Low — config is immutable during slicing |
| Transform ↔ MeshSlicingParams | Uses existing `trafo` field | Low — existing field, no signature change |
| ZSchedule ↔ SlicingParameters | Read-only struct access | Low — POD struct, trivially copyable |
| slice_volumes hook ↔ existing code | Conditional insertion | Low — zero impact when disabled |
| FirstLayer ↔ Layer | Sets is_first_layer flag | Low — Layer already has this concept |

All new components are pure (no global state, no I/O). The only stateful modification is composing the rotation into `MeshSlicingParams::trafo`, which is a local variable inside `slice_volumes()`.

## Files Modified vs. Files Added

### Modified (existing files, minimal changes)
| File | Change Type | Lines Estimated |
|------|-------------|-----------------|
| `src/libslic3r/PrintConfig.hpp` | Add 2 ConfigOptionFloat fields | +4 lines |
| `src/libslic3r/PrintConfig.cpp` | Register 2 config options in PrintConfigDef | +30 lines |
| `src/libslic3r/PrintObject.cpp` | Integration hook in slice_volumes() | +20 lines |
| `src/libslic3r/CMakeLists.txt` | Add new source files | +6 lines |

### Added (new files)
| File | Type |
|------|------|
| `src/libslic3r/Feature/AngledSlicing/AngledSlicingParams.hpp` | Header only |
| `src/libslic3r/Feature/AngledSlicing/AngledSlicingTransform.hpp` | Header |
| `src/libslic3r/Feature/AngledSlicing/AngledSlicingTransform.cpp` | Implementation |
| `src/libslic3r/Feature/AngledSlicing/AngledSlicingZSchedule.hpp` | Header |
| `src/libslic3r/Feature/AngledSlicing/AngledSlicingZSchedule.cpp` | Implementation |
| `src/libslic3r/Feature/AngledSlicing/AngledSlicingFirstLayer.hpp` | Header |
| `src/libslic3r/Feature/AngledSlicing/AngledSlicingFirstLayer.cpp` | Implementation |
| `tests/libslic3r/test_angled_slicing.cpp` | Unit + PBT tests |

**Total: ~60 lines changed in existing files, ~500-800 lines of new code.**
