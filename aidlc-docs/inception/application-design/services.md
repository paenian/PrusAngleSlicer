# Services — Angled Slicing Feature

## Overview

PrusaSlicer uses a pipeline orchestration pattern rather than a service-oriented architecture. "Services" in this context map to the orchestration logic within `PrintObject` and `Print` that calls into the new angled slicing components.

---

## Service 1: AngledSlicingOrchestrator (conceptual, not a class)

**Implemented as**: Logic added inside `PrintObject::slice_volumes()` and `PrintObject::make_perimeters()`  
**Responsibility**: Coordinate the angled slicing components to produce correctly angled layers with minimal changes to the surrounding pipeline.

### Orchestration Flow

```
PrintObject::slice_volumes() [posSlice]
    │
    ├── 1. Create AngledSlicing::Params from PrintObjectConfig
    │         Params::from_config(this->config())
    │
    ├── 2. If disabled (angle == 0): run existing pipeline unchanged
    │
    ├── 3. If enabled:
    │         a. Compute rotation: AngledSlicing::Transform::build_rotation(params)
    │         b. Compose into MeshSlicingParams::trafo
    │         c. Compute Z schedule: AngledSlicing::ZSchedule::compute(slicing_params, params)
    │         d. Extract slice_z values from schedule → replace standard zs vector
    │         e. Call existing slice_mesh_ex() with modified params/zs (UNCHANGED)
    │         f. Apply first-layer clipping: AngledSlicing::FirstLayer::clip_to_bed()
    │                for each layer in schedule where is_clipped == true
    │         g. Tag first layers: propagate is_first_layer from schedule to Layer objects
    │
    └── 4. Continue with existing downstream pipeline (perimeters, infill, etc.) UNCHANGED
```

### What is NOT changed downstream:
- `Layer::make_perimeters()` — operates on already-computed 2D ExPolygons, no change
- `Layer::make_fills()` — same, fully independent of how slices were computed
- `Support::generate()` — overhang detection uses layer adjacency (upper/lower), which works correctly with angled layers
- `GCodeGenerator` — works from Layer print_z/height values, unchanged

---

## Service 2: AngledSlicingValidator (conceptual)

**Implemented as**: Validation logic inside `Print::validate()` or `PrintObject::invalidate_state_by_config_options()`  
**Responsibility**: Validate angled slicing configuration and warn about conditions that may produce unexpected results.

### Validation Rules

| Rule | Trigger | Action |
|------|---------|--------|
| Angle in valid range | angle ≥ 90 | Error: "Slicing angle must be less than 90°" |
| Height overflow | effective_height > printer max Z | Warning: "Print height exceeds printer maximum at this angle" |
| Variable layer height conflict | angle > 0 AND variable layer height enabled | Warning: "Variable layer height is disabled when angled slicing is active" |
| Sequential print conflict | angle > 0 AND sequential print enabled | Warning: "Sequential printing clearance checks may be inaccurate with angled slicing" |
| Extreme angle caution | angle > 60 | Warning: "Angles above 60° significantly increase print time and may affect quality" |

---

## Service 3: PrintConfig Serialization (existing infrastructure, zero new code)

**Responsibility**: Persist and restore `angled_slicing_angle` and `angled_slicing_direction` in print profiles and .3mf files.

The existing `PrintConfig` / `DynamicPrintConfig` serialization infrastructure handles this automatically once the two new `ConfigOptionFloat` fields are registered. No new serialization code is needed.

### Backward Compatibility
- .3mf files without these keys: both params default to 0.0 on load
- Existing profiles: default to 0.0 (standard horizontal slicing)
- Key names follow existing convention: `"angled_slicing_angle"`, `"angled_slicing_direction"`

---

## Service 4: Invalidation Service (existing infrastructure, small addition)

**Responsibility**: Ensure that changing angled slicing parameters invalidates the correct pipeline steps.

```cpp
// Addition to PrintObject::invalidate_state_by_config_options():
if (opt_key == "angled_slicing_angle" || opt_key == "angled_slicing_direction") {
    steps.emplace_back(posSlice);  // Invalidate slicing and everything downstream
}
```

This hooks into the existing invalidation cascade — posSlice invalidation automatically propagates to posPerimeters, posPrepareInfill, posInfill, etc., and ultimately to psGCodeExport. No additional invalidation logic needed.
