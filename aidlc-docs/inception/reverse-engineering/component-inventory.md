# Component Inventory — PrusaSlicer

## Core Pipeline Components

| Component | File(s) | Responsibility |
|-----------|---------|----------------|
| **Print** | `Print.hpp/cpp` | FFF print orchestration. Owns all PrintObjects and PrintRegions. Entry: `process()`, `export_gcode()` |
| **PrintObject** | `PrintObject.cpp`, `Print.hpp` | Per-object slicing pipeline. Steps: slice → perimeters → prepare_infill → infill → ironing → supports → G-code |
| **PrintRegion** | `Print.hpp` | Groups model volumes sharing the same print config. Carries `PrintRegionConfig` |
| **PrintBase** | `PrintBase.hpp/cpp` | Abstract base with step state machine (Fresh/Started/Done/Invalidated) and invalidation cascade |
| **GCodeGenerator** | `GCode.hpp/cpp` | Translates extrusion paths to G-code strings. Entry: `do_export()` |
| **Layer** | `Layer.hpp/cpp` | Single horizontal slice of a PrintObject. Contains LayerRegions, LayerSlices, LayerIslands |
| **LayerRegion** | `LayerRegion.hpp/cpp` | Per-region per-layer data: slices, perimeters, fills, fill_expolygons |
| **PrintConfig** | `PrintConfig.hpp/cpp` | Configuration hierarchy. PrintObjectConfig, PrintRegionConfig, PrintConfig, GCodeConfig |
| **Model** | `Model.hpp/cpp` | Scene graph. Model → ModelObject → ModelVolume/ModelInstance |
| **TriangleMeshSlicer** | `TriangleMeshSlicer.hpp/cpp` | Slices a 3D mesh at given Z heights into ExPolygons |

## Perimeter Components

| Component | File(s) | Responsibility |
|-----------|---------|----------------|
| **PerimeterGenerator** | `PerimeterGenerator.hpp/cpp` | Generates perimeter loops from layer slices. Two modes: Classic (Clipper offset) and Arachne (variable-width) |
| **Arachne Engine** | `Arachne/` dir | Variable-width contour toolpath engine (ported from UltiMaker Cura) |
| **ExtrusionEntity** | `ExtrusionEntity.hpp/cpp` | Abstract extrusion path. Subtypes: ExtrusionPath, ExtrusionLoop, ExtrusionMultiPath |
| **Flow** | `Flow.hpp/cpp` | Computes extrusion flow width, cross-section area, mm3/mm given nozzle + layer height |

## Infill Components

| Component | File(s) | Responsibility |
|-----------|---------|----------------|
| **Fill (base)** | `Fill/FillBase.hpp/cpp` | Abstract fill base. `fill_surface()` virtual method. Factory: `Fill::new_from_type()` |
| **FillRectilinear** | `Fill/FillRectilinear.hpp/cpp` | Rectilinear, Monotonic, MonotonicLines, AlignedRectilinear patterns |
| **FillConcentric** | `Fill/FillConcentric.hpp/cpp` | Concentric infill (with Arachne support) |
| **FillAdaptive** | `Fill/FillAdaptive.hpp/cpp` | Adaptive Cubic — density varies by overhang proximity |
| **FillLightning** | `Fill/FillLightning.hpp/cpp` | Lightning infill — minimal material branching structure |
| **FillGyroid** | `Fill/FillGyroid.hpp/cpp` | Gyroid (triply periodic minimal surface) infill |
| **FillHoneycomb** | `Fill/FillHoneycomb.hpp/cpp` | 2D honeycomb infill |
| **FillLine / FillGrid** | `Fill/Fill*.hpp/cpp` | Additional 2D patterns |
| **FillEnsuring** | `Fill/FillEnsuring.hpp/cpp` | Ensures minimum solid material count in walls |

## G-code Subsystem Components

| Component | File(s) | Responsibility |
|-----------|---------|----------------|
| **CoolingBuffer** | `GCode/CoolingBuffer.hpp/cpp` | Adjusts print speeds to meet cooling time requirements |
| **SpiralVase** | `GCode/SpiralVase.hpp/cpp` | Transforms single-wall prints into continuous spiral (vase mode) |
| **PressureEqualizer** | `GCode/PressureEqualizer.hpp/cpp` | Smooths extrusion speeds to equalize nozzle pressure |
| **SeamPlacer** | `GCode/SeamPlacer.hpp/cpp` | Selects optimal seam position per layer (random/aligned/rear) |
| **WipeTower** | `GCode/WipeTower.hpp/cpp` | Generates wipe tower G-code for multi-material purging |
| **ToolOrdering** | `GCode/ToolOrdering.hpp/cpp` | Plans extruder/tool change sequence across layers |
| **AvoidCrossingPerimeters** | `GCode/AvoidCrossingPerimeters.hpp/cpp` | Routes travels inside perimeters to reduce visible stringing |
| **ArcWelder** | `Geometry/ArcWelder.hpp/cpp` | Converts polyline segments to G2/G3 arc commands |
| **GCodeWriter** | `GCode/GCodeWriter.hpp/cpp` | Low-level G-code string generation (moves, temps, fans) |
| **GCodeProcessor** | `GCode/GCodeProcessor.hpp/cpp` | Re-parses generated G-code for preview/analysis |
| **ExtrusionOrder** | `GCode/ExtrusionOrder.hpp` | Determines per-layer extrusion print order |
| **Travels** | `GCode/Travels.hpp/cpp` | Travel move elevation and ramping strategies |
| **SmoothPath** | `GCode/SmoothPath.hpp/cpp` | Arc/smooth path caching and interpolation |

## Support Components

| Component | File(s) | Responsibility |
|-----------|---------|----------------|
| **SupportMaterial** | `Support/SupportMaterial.hpp/cpp` | Grid/snug support generation |
| **TreeSupport** | `Support/TreeSupport.hpp/cpp` | Tree support generation |
| **TreeModelVolumes** | `Support/TreeModelVolumes.hpp/cpp` | Collision volumes for tree support |
| **SupportSpotsGenerator** | `SupportSpotsGenerator.hpp/cpp` | Detects areas needing support |
| **BridgeDetector** | `BridgeDetector.hpp/cpp` | Detects bridge spans and computes bridge angle |

## Feature Components (Extensible)

| Component | File(s) | Responsibility |
|-----------|---------|----------------|
| **FullSpectrum** | `Feature/FullSpectrum/` | Virtual extruder system for multi-material mapping |
| **FuzzySkin** | `Feature/FuzzySkin/` | Randomized surface texture via perimeter noise |
| **Interlocking** | `Feature/Interlocking/` | Generates interlocking geometry at multi-material interfaces |

## SLA Components

| Component | File(s) | Responsibility |
|-----------|---------|----------------|
| **SLAPrint** | `SLAPrint.hpp/SLAPrintSteps.cpp` | SLA print pipeline (exposure mask generation) |
| **SLA/Support** | `SLA/` dir | SLA support pillar generation |

## GUI Components

| Component | File(s) | Responsibility |
|-----------|---------|----------------|
| **Plater** | `slic3r/GUI/Plater.hpp/cpp` | Main workspace: object list, 3D viewport, sidebar |
| **GLCanvas3D** | `slic3r/GUI/GLCanvas3D.hpp/cpp` | OpenGL 3D viewport rendering and mouse interaction |
| **BackgroundSlicingProcess** | `slic3r/GUI/BackgroundSlicingProcess.hpp/cpp` | Runs `Print::process()` on background thread |
| **Tab** | `slic3r/GUI/Tab.hpp/cpp` | Settings tabs (Print/Filament/Printer profiles) |
| **Gizmos** | `slic3r/GUI/Gizmos/` | Interactive 3D editing tools |
| **ImGuiWrapper** | `slic3r/GUI/ImGuiWrapper.hpp/cpp` | Dear ImGui integration |
