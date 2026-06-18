# Business Rules — U1: Config & Parameters

## Validation Rules

| ID | Rule | Enforcement |
|----|------|-------------|
| BR-1.1 | `angled_slicing_angle` must be in range [0.0, 89.0] | PrintConfigDef min/max constraints + UI input validation |
| BR-1.2 | `angled_slicing_direction` must be in range [0.0, 360.0) | PrintConfigDef min/max constraints |
| BR-1.3 | Default value for both is 0.0 (feature disabled) | ConfigOptionFloat default_value |
| BR-1.4 | When angle == 0.0, no angled slicing code paths execute | Params::enabled() returns false |

## Invalidation Rules

| ID | Rule | Implementation |
|----|------|---------------|
| BR-2.1 | Changing `angled_slicing_angle` invalidates `posSlice` | Added to opt_key checks in `invalidate_state_by_config_options()` |
| BR-2.2 | Changing `angled_slicing_direction` invalidates `posSlice` | Same as above |
| BR-2.3 | Invalidation cascades: posSlice → all downstream steps | Existing cascade mechanism (no new code) |

## Backward Compatibility Rules

| ID | Rule | Implementation |
|----|------|---------------|
| BR-3.1 | .3mf files without angled slicing keys load without error | Existing handle_legacy() mechanism; missing keys use default (0.0) |
| BR-3.2 | Print profiles without angled slicing keys load without error | Same mechanism; DynamicPrintConfig ignores unknown/missing keys |
| BR-3.3 | When angle=0, G-code output is byte-for-byte identical to stock | Params::enabled()==false → zero code paths affected |

## UI Placement Rules

| ID | Rule | Implementation |
|----|------|---------------|
| BR-4.1 | Angle input appears in Print Settings → "Layers and perimeters" section | Add to Tab.cpp in the layer height group |
| BR-4.2 | Direction input appears immediately below the angle input | Same group, second field |
| BR-4.3 | Tooltip explains the feature purpose and units | PrintConfigDef tooltip string |
| BR-4.4 | Both fields are visible only when Expert mode is active | Category assignment or mode_override flag |

## Testable Properties (PBT)

| Property | Category | Description |
|----------|----------|-------------|
| Params round-trip | Round-trip | `Params::from_config(config).tilt_angle_deg() == config.angled_slicing_angle.value` |
| Radian conversion | Invariant | `tilt_angle_rad() == tilt_angle_deg() * M_PI / 180.0` for all valid inputs |
| Enabled predicate | Invariant | `enabled() == (tilt_angle_deg() > 1e-6)` for all valid inputs |
| Direction wrap | Invariant | Direction 0.0 and 360.0 produce equivalent behavior (though 360 is clamped by UI) |
