# Business Logic Model — U1: Config & Parameters

## Overview

Unit 1 establishes the configuration foundation for angled slicing. It adds two config parameters, validates them, ensures they persist/restore correctly, and creates the `AngledSlicing::Params` value object that downstream units will consume.

---

## Logic Flow

```
User sets angle/direction in Print Settings UI
    │
    ├──► PrintObjectConfig stores values
    │       └── angled_slicing_angle: [0, 89]
    │       └── angled_slicing_direction: [0, 360)
    │
    ├──► Config change triggers posSlice invalidation
    │       └── invalidate_state_by_config_options() handles this
    │
    ├──► On slice start, Params::from_config() creates value object
    │       └── Reads from PrintObjectConfig
    │       └── Computes enabled() = (angle > epsilon)
    │       └── Computes radian equivalents
    │
    └──► Params object passed to U2 components (Transform, ZSchedule, FirstLayer)
```

## Business Logic

### BL-1: Config Parameter Storage
- `angled_slicing_angle` stored as `ConfigOptionFloat` in `PrintObjectConfig`
- `angled_slicing_direction` stored as `ConfigOptionFloat` in `PrintObjectConfig`
- Both are per-object settings (not global), consistent with `layer_height`, `seam_position`, etc.

### BL-2: Validation
- Angle: min=0.0, max=89.0, default=0.0
- Direction: min=0.0, max=360.0, default=0.0 (direction is cyclic; 360° == 0°)
- When angle=0: feature is completely disabled (identity behavior)

### BL-3: Invalidation Cascade
- Changing `angled_slicing_angle` OR `angled_slicing_direction` invalidates `posSlice`
- `posSlice` invalidation cascades automatically to all downstream steps:
  posPerimeters → posPrepareInfill → posInfill → posIroning → posSupportMaterial → psGCodeExport
- No additional invalidation wiring needed

### BL-4: Serialization
- Values serialize into `.ini` print profiles via existing `StaticPrintConfig` mechanism
- Values serialize into `.3mf` project archives via existing XML config embedding
- On load from older files: missing keys default to 0.0 (disabled)

### BL-5: Params Value Object
```cpp
AngledSlicing::Params:
    enabled()          → bool  (angle > 1e-6)
    tilt_angle_deg()   → double
    tilt_direction_deg() → double
    tilt_angle_rad()   → double (angle * M_PI / 180.0)
    tilt_direction_rad() → double (direction * M_PI / 180.0)
```

This is a pure value type. Constructed from config, immutable after creation, passed by const reference.
