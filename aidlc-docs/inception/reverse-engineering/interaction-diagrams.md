# Interaction Diagrams — PrusaSlicer

## Diagram 1: GUI-Initiated Slicing Flow

```
User          Plater         BackgroundSlicingProcess    Print           PrintObject
 |               |                    |                    |                 |
 |--change cfg-->|                    |                    |                 |
 |               |--apply(model,cfg)->|                    |                 |
 |               |                   |---Print::apply()-->|                 |
 |               |                   |                    |--invalidate---->|
 |               |                   |                    |   steps()       |
 |               |                   |<---ApplyStatus-----|                 |
 |               |--schedule_bg_-->  |                    |                 |
 |               |   process()       |                    |                 |
 |               |                   |---Print::process()->                 |
 |               |                   |                    |--make_perim()-->|
 |               |                   |                    |                 |--slice()
 |               |                   |                    |                 |--perims()
 |               |                   |                    |<---done---------|
 |               |                   |                    |--prepare_inf-->|
 |               |                   |                    |--infill()----->|
 |               |                   |                    |--supports()---->|
 |               |                   |                    |--skirt/brim()   |
 |               |                   |<---process done----|                 |
 |               |<--notify done-----|                    |                 |
 |<--preview-----|                   |                    |                 |
```

## Diagram 2: Perimeter Generation Detail

```
PrintObject::make_perimeters()
    |
    |-- For each Layer (parallel, TBB)
    |       Layer::make_perimeters()
    |           |
    |           |-- Group LayerRegions by compatible perimeter params
    |           |   --> PerimeterRegion[]
    |           |
    |           |-- For each PerimeterRegion:
    |           |       For each Surface (slice) in region:
    |           |           |
    |           |           |-- if config.perimeter_generator == Arachne:
    |           |           |       PerimeterGenerator::process_arachne(params, surface, ...)
    |           |           |           Arachne::WallToolPaths::generate()
    |           |           |           --> variable-width ExtrusionLoops
    |           |           |
    |           |           |-- else (Classic):
    |           |           |       PerimeterGenerator::process_classic(params, surface, ...)
    |           |           |           Clipper offset inward N times
    |           |           |           --> fixed-width ExtrusionLoops
    |           |           |
    |           |           +--> out_loops (perimeter extrusions)
    |           |                out_gap_fill
    |           |                out_fill_expolygons (passed to infill stage)
    |           |
    |           |-- Layer::sort_perimeters_into_islands()
    |               Assigns each extrusion to its LayerIsland
```

## Diagram 3: Infill Generation Detail

```
PrintObject::infill()
    |
    |-- For each Layer (parallel, TBB)
    |       Layer::make_fills(adaptive_octree, support_octree, lightning_gen)
    |           |
    |           |-- For each LayerRegion:
    |           |       For each Surface in fill_expolygons:
    |           |           |
    |           |           |-- Fill* filler = Fill::new_from_type(pattern)
    |           |           |-- filler->layer_id = layer->id()
    |           |           |-- filler->z = layer->print_z
    |           |           |-- filler->spacing = region.flow(...)
    |           |           |-- filler->angle = computed angle
    |           |           |
    |           |           |-- Polylines paths = filler->fill_surface(surface, params)
    |           |           |
    |           |           +--> ExtrusionEntityCollection (stored in LayerRegion::fills)
```

## Diagram 4: G-code Generation Flow

```
GCodeGenerator::do_export()
    |
    |-- _do_export(print, file, thumbnail_cb)
    |       |
    |       |-- collect_layers_to_print(print)
    |       |   --> vector<pair<z, ObjectsLayerToPrint>>
    |       |
    |       |-- ToolOrdering (multi-extruder planning)
    |       |
    |       |-- SeamPlacer::init()
    |       |
    |       |-- process_layers() [parallel pipeline using TBB]
    |       |       |
    |       |       |-- For each Z level in layers_to_print:
    |       |       |       process_layer(layers, layer_tools, ...)
    |       |       |           |
    |       |       |           |-- For each PrintInstance:
    |       |       |           |     initialize_instance()
    |       |       |           |     get_sorted_extrusions()
    |       |       |           |
    |       |       |           |-- extrude_slices()
    |       |       |           |     |-- extrude_perimeters()
    |       |       |           |     |     extrude_smooth_path() per loop
    |       |       |           |     |-- extrude_infill_ranges()
    |       |       |           |     |-- extrude_support()
    |       |       |           |     |-- travel_to() between extrusions
    |       |       |           |
    |       |       |           +--> LayerResult { gcode string }
    |       |       |
    |       |       |-- CoolingBuffer::process_layer()  [adjust speeds]
    |       |       |-- SpiralVase::process_layer()     [if enabled]
    |       |       |-- PressureEqualizer::process()    [if enabled]
    |       |       |-- GCodeProcessor::process_buffer() [parse for preview]
    |       |       |-- Write to file
```

## Diagram 5: Configuration Change Propagation

```
User changes "infill_density" in UI
    |
    v
Tab::update_config_values()
    |
    v
Plater::changed_objects() or changed_config()
    |
    v
BackgroundSlicingProcess::apply_config()
    |
    v
Print::apply(model, new_config)
    |
    v
PrintObject::invalidate_state_by_config_options(
    old_config, new_config, ["fill_density"])
    |
    v  [only downstream steps invalidated]
    invalidate_step(posPrepareInfill)   <-- infill density affects this
    invalidate_step(posInfill)          <-- and this
    invalidate_step(psGCodeExport)      <-- and ultimately this
    NOT invalidated: posSlice, posPerimeters  <-- these are untouched
    |
    v
Next Print::process() call only re-runs:
    prepare_infill + infill + gcode_export
```
