# Application Design — Angled Slicing Feature

## Summary

The angled slicing feature is implemented as a self-contained module under `src/libslic3r/Feature/AngledSlicing/`. The core insight driving this design is that `MeshSlicingParams` already carries a `Transform3d trafo` field. By composing a tilt rotation into this transform, the existing `slice_mesh_ex()` infrastructure handles tilted slicing transparently — no changes to `TriangleMeshSlicer` are required.

All 4 new components are pure functions with no side effects, making them straightforward to unit-test and property-test. Existing code changes are minimal: ~60 lines across 4 existing files.

---

## Architecture Diagram

```
+─────────────────────────────────────────────────────────────────+
│                    NEW: Feature/AngledSlicing/                   │
│                                                                  │
│  AngledSlicing::Params    (value object, from PrintObjectConfig) │
│         │                                                        │
│         ├──► AngledSlicing::Transform  (rotation matrix math)   │
│         │                                                        │
│         ├──► AngledSlicing::ZSchedule  (layer Z computation)    │
│         │         └── LayerZInfo[]  (print_z, slice_z, height,  │
│         │                            is_first_layer, is_clipped) │
│         │                                                        │
│         └──► AngledSlicing::FirstLayer (bed-clip, first-layer   │
│                                         tagging)                 │
+─────────────────────────────────────────────────────────────────+
          │
          │  Used by (minimal hook, ~20 lines)
          ▼
+─────────────────────────────────────────────────────────────────+
│  EXISTING: PrintObject::slice_volumes()  (posSlice step)        │
│                                                                  │
│  if (angle_params.enabled()) {                                   │
│      // 1. Compose rotation into trafo                           │
│      // 2. Replace z-heights with ZSchedule output              │
│      // 3. Call existing slice_mesh_ex() (unchanged)            │
│      // 4. Clip partial layers with FirstLayer::clip_to_bed()   │
│      // 5. Tag first-layer flags on Layer objects               │
│  }                                                               │
+─────────────────────────────────────────────────────────────────+
          │
          │  Produces (unchanged Layer data structures)
          ▼
+─────────────────────────────────────────────────────────────────+
│  EXISTING: Downstream Pipeline (ALL UNCHANGED)                  │
│  make_perimeters() → prepare_infill() → infill() →             │
│  supports() → GCodeGenerator                                    │
+─────────────────────────────────────────────────────────────────+
```

---

## Component Summary

| Component | File(s) | Type | Responsibility |
|-----------|---------|------|----------------|
| `AngledSlicing::Params` | `AngledSlicingParams.hpp` | Value object | User config parameters (angle, direction) |
| `AngledSlicing::Transform` | `AngledSlicingTransform.hpp/cpp` | Pure functions | Rotation matrix from angle+direction |
| `AngledSlicing::ZSchedule` | `AngledSlicingZSchedule.hpp/cpp` | Pure functions | Z-height schedule for tilted layers |
| `AngledSlicing::FirstLayer` | `AngledSlicingFirstLayer.hpp/cpp` | Pure functions | Bed-clipping and first-layer detection |

## Existing Files Modified

| File | Change | Lines |
|------|--------|-------|
| `PrintConfig.hpp` | +2 ConfigOptionFloat fields | +4 |
| `PrintConfig.cpp` | Register 2 new options in PrintConfigDef | +30 |
| `PrintObject.cpp` | Conditional hook in `slice_volumes()` | +20 |
| `libslic3r/CMakeLists.txt` | Add new source files | +6 |

---

## Key Design Decisions

### Decision 1: Direct Tilted-Plane Slicing (NOT rotation-based)
The tilt is implemented by intersecting the mesh with tilted planes directly, NOT by rotating the mesh. The `TriangleMeshSlicer` must be extended (or wrapped) to support arbitrary plane normals. The object mesh is never moved.

### Decision 2: Pure Function Components
All new components are stateless pure functions. No singleton, no global state. This makes testing straightforward and avoids any thread-safety concerns (the pipeline already uses TBB parallel_for).

### Decision 3: Feature Directory Pattern
Follows the existing `Feature/FuzzySkin/` and `Feature/Interlocking/` pattern — a self-contained subdirectory with its own headers and implementation files. This is the PrusaSlicer convention for new optional features.

### Decision 4: Config-Gated at posSlice
The entire feature is gated by `angle_params.enabled()` which is `false` when `angled_slicing_angle == 0`. When disabled, the integration hook adds zero overhead.

### Decision 5: Invalidation via posSlice
Changing either angle parameter invalidates `posSlice`, which automatically cascades through the existing invalidation system.

### Decision 6: Object Must Not Be Moved (HARD CONSTRAINT)
The mesh must remain in its original position and orientation throughout the entire pipeline. No rotation, shear, or translation is applied to the model. Tilted slicing is achieved by computing plane-mesh intersections with non-horizontal planes directly.

---

## PBT Properties Identified (preview for Functional Design)

The following geometric invariants will be verified with property-based tests:

| Property | Category | Component |
|----------|----------|-----------|
| `build_rotation(angle=0)` == identity matrix | Invariant | Transform |
| `build_rotation(angle) * build_inverse_rotation(angle)` == identity | Round-trip | Transform |
| Rotation is a valid rotation matrix (det=1, orthonormal) | Invariant | Transform |
| Z schedule is strictly ascending in print_z | Invariant | ZSchedule |
| All heights in schedule > 0 | Invariant | ZSchedule |
| `sum(heights)` ≈ `object_height / cos(angle)` | Invariant | ZSchedule |
| `clip_to_bed()` with `is_clipped=false` is identity | Idempotence | FirstLayer |
| `clip_to_bed()` never produces regions with negative Z | Invariant | FirstLayer |

---

## Risks and Mitigations

| Risk | Mitigation |
|------|-----------|
| Elephant foot compensation interacts badly with multiple first layers | Investigate in Functional Design; may disable elephant foot when angle > 0 for MVP |
| Variable layer height profile + angled slicing = unclear interaction | Disable variable layer height when angle > 0 (add validator warning) for MVP |
| Sequential print clearance calculations assume horizontal layers | Add warning in validator; post-MVP fix |
| TBB parallel slicing with shared ZSchedule state | ZSchedule is computed once before the parallel loop and passed by const reference — thread safe |
