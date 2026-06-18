# Core Slicing Pipeline — PrusaSlicer FFF

## Pipeline Overview

The FFF slicing pipeline flows through two orchestration levels: `Print` (global) and `PrintObject` (per-object).

```
Model (scene graph)
    |
    v
Print::apply(model, config)         -- Sync model changes, invalidate affected steps
    |
    v
Print::process()
    |
    +-- [psWipeTower / psToolOrdering]  -- Multi-material tool ordering
    |
    +-- For each PrintObject:
    |       PrintObject::make_perimeters()
    |           |-- slice_volumes()           [posSlice]
    |           |       TriangleMeshSlicer::slice() per volume
    |           |       Clipper boolean ops to merge volumes
    |           |       detect_surfaces_type() -- top/bottom/internal
    |           |
    |           |-- make_perimeters()          [posPerimeters]
    |               For each Layer:
    |                 PerimeterGenerator::process_classic()  OR
    |                 PerimeterGenerator::process_arachne()
    |                 --> ExtrusionEntityCollection (loops, thin fills)
    |                 --> ExPolygons for fill regions
    |
    +-- For each PrintObject:
    |       PrintObject::prepare_infill()      [posPrepareInfill]
    |           process_external_surfaces()
    |           discover_vertical_shells()
    |           bridge_over_infill()
    |           clip_fill_surfaces()
    |           discover_horizontal_shells()
    |           combine_infill()
    |
    +-- For each PrintObject:
    |       PrintObject::infill()              [posInfill]
    |           For each Layer, for each LayerRegion:
    |             Fill::new_from_type(pattern)
    |             fill->fill_surface(surface, params)
    |             --> Polylines / ExtrusionEntityCollection
    |
    +-- PrintObject::ironing()                 [posIroning]
    |
    +-- PrintObject::generate_support_spots()  [posSupportSpotsSearch]
    |
    +-- PrintObject::generate_support_material()[posSupportMaterial]
    |       SupportMaterial (grid/snug) OR TreeSupport
    |
    +-- PrintObject::estimate_curled_extrusions()[posEstimateCurledExtrusions]
    |
    +-- PrintObject::calculate_overhanging_perimeters()[posCalculateOverhangingPerimeters]
    |
    +-- [psSkirtBrim]  -- Generate skirt and brim extrusions
    |
    v
Print::export_gcode()
    |
    v
GCodeGenerator::do_export()
    |
    +-- collect_layers_to_print()   -- Build per-Z layer groups
    +-- ToolOrdering                -- Plan tool changes
    +-- process_layers() [parallel pipeline]
    |       For each Z level:
    |         process_layer()
    |           sort_print_object_instances()
    |           get_sorted_extrusions()   -- ExtrusionOrder
    |           extrude_perimeters()
    |           extrude_infill_ranges()
    |           extrude_support()
    |           travel_to()
    |           --> LayerResult { gcode string }
    |
    +-- CoolingBuffer::process_layer()  -- Adjust speeds for cooling
    +-- SpiralVase (if enabled)
    +-- PressureEqualizer (if enabled)
    +-- GCodeProcessor::process_buffer() -- Parse for visualization
    |
    v
.gcode file
```

## Key Data Structures in the Pipeline

### Model / Scene Graph
```
Model
  └── ModelObject[]
        ├── ModelVolume[] (solid, modifier, support enforcer/blocker)
        ├── ModelInstance[] (position, rotation, scale)
        └── layer_height_profile[]
```

### Print / Slicing Graph
```
Print
  ├── PrintObject[]
  │     ├── Layer[]
  │     │     ├── LayerRegion[]  (one per PrintRegion)
  │     │     │     ├── slices: SurfaceCollection
  │     │     │     ├── perimeters: ExtrusionEntityCollection
  │     │     │     ├── fills: ExtrusionEntityCollection
  │     │     │     └── fill_expolygons: ExPolygons
  │     │     └── lslices_ex: LayerSlice[] → LayerIsland[]
  │     └── SupportLayer[]
  └── PrintRegion[]  (shared config groups)
```

### Extrusion Hierarchy
```
ExtrusionEntityCollection
  └── ExtrusionEntity (abstract)
        ├── ExtrusionPath       (open polyline, single flow)
        ├── ExtrusionMultiPath  (connected paths, varying flow)
        └── ExtrusionLoop       (closed perimeter loop)
```

## Configuration Hierarchy

```
FullPrintConfig
  ├── PrintObjectConfig    -- Per-object: supports, seam, layers, perimeters
  ├── PrintRegionConfig    -- Per-region: extruder assignment, infill, perimeters, speeds
  └── PrintConfig          -- Global: printer dims, temperatures, cooling
        └── GCodeConfig    -- G-code flavor, arcs, resolution
```

## Step Invalidation Rules

When a user changes a config parameter, only the steps that depend on that parameter are invalidated:
- Changing layer height → invalidates posSlice and all downstream steps
- Changing perimeter count → invalidates posPerimeters and downstream (NOT posSlice)
- Changing infill density → invalidates posPrepareInfill and posInfill only
- Changing temperature → only invalidates psGCodeExport

This cache-based approach makes re-slicing after minor config changes very fast.

## Extension Points for New Features

The most important hooks for adding new behavior with minimal code changes:

1. **New PrintObjectStep** — Add a new enum value to `PrintObjectStep` and implement a corresponding method on `PrintObject`. Register the invalidation dependencies in `PrintObject::invalidate_state_by_config_options()`.

2. **New Fill pattern** — Subclass `Fill`, implement `fill_surface()`, register in `Fill::new_from_type()` and `InfillPattern` enum in `PrintConfig.hpp`.

3. **New Feature in Feature/** — Follow the `FullSpectrum` or `FuzzySkin` pattern: create `src/libslic3r/Feature/YourFeature/` with a self-contained implementation that hooks into `PrintObject::make_perimeters()` or `PrintObject::prepare_infill()`.

4. **New G-code post-processor** — Add a subsystem class in `src/libslic3r/GCode/` (following `CoolingBuffer`, `SpiralVase`, `PressureEqualizer` patterns) and integrate it into `GCodeGenerator::process_layers()`.

5. **New config parameter** — Add to the appropriate config class in `PrintConfig.hpp` using the `STATIC_PRINT_CONFIG_CACHE` macro pattern, then add invalidation rules.
