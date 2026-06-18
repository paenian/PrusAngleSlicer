# Code Structure — PrusaSlicer

## Repository Root

```
PrusAngleSlicer/
├── src/                    Main source code
├── deps/                   External dependency bootstrap (CMake)
├── bundled_deps/           In-tree third-party libraries
├── resources/              UI assets (icons, fonts, printer profiles, filament profiles)
├── tests/                  Unit and integration test suites
├── cmake/                  CMake utility modules
├── doc/                    Developer documentation
├── sandboxes/              Developer experiment/scratch areas
├── build-utils/            Build helper tools
├── CMakeLists.txt          Top-level build configuration
├── version.inc             Version numbers
└── .github/workflows/      CI pipelines (macOS, Windows, Linux Flatpak)
```

## src/ Directory Breakdown

### src/libslic3r/ — Core Slicing Library (~700 files)
The most important directory. Fully technology-neutral C++17.

**Top-level key files:**
| File(s) | Purpose |
|---------|---------|
| `Print.hpp / Print.cpp` | FFF print orchestration, pipeline entry point |
| `PrintObject.cpp` | Per-object pipeline steps (slice, perimeters, infill, supports) |
| `PrintConfig.hpp / PrintConfig.cpp` | All configuration options, enums, config hierarchy |
| `PrintBase.hpp / PrintBase.cpp` | Abstract base with step state machine |
| `Model.hpp / Model.cpp` | Scene graph: Model, ModelObject, ModelVolume, ModelInstance |
| `Layer.hpp / Layer.cpp` | Per-layer data: slices, islands, extrusion collections |
| `LayerRegion.hpp / LayerRegion.cpp` | Per-region per-layer data (slices, fills, perimeters) |
| `GCode.hpp / GCode.cpp` | G-code generator (final pipeline stage) |
| `GCode.cpp` (Note: `GCodeGenerator`) | process_layers → process_layer → extrude_* |
| `PerimeterGenerator.hpp/cpp` | Perimeter generation (Classic + Arachne entry points) |
| `Slicing.hpp / Slicing.cpp` | Layer height computation, variable layer height |
| `Flow.hpp / Flow.cpp` | Extrusion flow width/area calculations |
| `ClipperUtils.hpp / ClipperUtils.cpp` | Polygon boolean ops wrapper around Clipper |
| `Geometry.hpp / Geometry.cpp` | 2D/3D geometry utilities |
| `ExPolygon.hpp` | Core 2D geometry type: polygon + holes |
| `SLAPrint.hpp / SLAPrintSteps.cpp` | SLA print pipeline |
| `BridgeDetector.hpp/cpp` | Bridge detection for solid infill over gaps |
| `SupportSpotsGenerator.hpp/cpp` | Automatic support placement detection |

**Subdirectories:**
| Dir | Contents |
|-----|---------|
| `Fill/` | 14+ infill pattern implementations (Rectilinear, Grid, Gyroid, Lightning, Adaptive, etc.) |
| `GCode/` | G-code subsystems: SeamPlacer, WipeTower, CoolingBuffer, SpiralVase, PressureEqualizer, ToolOrdering, Travels, ArcWelder, etc. |
| `Support/` | Support material generators: SupportMaterial (grid/snug), TreeSupport, TreeModelVolumes |
| `Arachne/` | Variable-width perimeter engine (ported from Cura) |
| `Geometry/` | Arc detection, ArcWelder, convex hull, rotation calcs |
| `SLA/` | SLA-specific: rasterizer, hollowing, pad, support islands |
| `Feature/` | Opt-in pipeline features: FullSpectrum (multi-extruder), FuzzySkin, Interlocking |
| `Algorithm/` | Reusable algorithms: LineSegmentation |
| `Format/` | File format I/O: STL, OBJ, 3MF, AMF, STEP, SLA, G-code |
| `Optimize/` | Numerical optimization (NLopt wrapper) |
| `Utils/` | Utilities: semver, time, threading, platform detection |
| `Execution/` | Parallel execution policy abstraction (TBB vs sequential) |
| `BranchingTree/` | Branching tree support data structures |
| `CSGMesh/` | Constructive solid geometry mesh operations |

### src/slic3r/ — Application Shell + GUI (~500 files)

**src/slic3r/GUI/** — wxWidgets GUI
| File(s) | Purpose |
|---------|---------|
| `Plater.hpp/cpp` | Main workspace widget (central hub of the UI) |
| `3DScene.hpp/cpp` | OpenGL scene management |
| `GLCanvas3D.hpp/cpp` | 3D viewport with mouse interaction |
| `MainFrame.hpp/cpp` | Top-level application window |
| `BackgroundSlicingProcess.hpp/cpp` | Runs slicing on background thread |
| `Tab.hpp/cpp` | Print/filament/printer settings tabs |
| `PresetBundle.cpp` (in libslic3r) | Profile management |
| `2DBed.hpp/cpp`, `3DBed.hpp/cpp` | Bed visualization |
| `Camera.hpp/cpp` | 3D camera control |
| `Gizmos/` | Interactive 3D manipulation tools (cut, text, seam painting, etc.) |
| `arrange/` | 2D arrangement UI integration |
| `ImGuiWrapper.hpp/cpp` | Dear ImGui integration for overlay UI |

**src/slic3r/Config/** — Config UI widgets
**src/slic3r/Utils/** — GUI utility helpers

### src/CLI/ — Command-Line Interface
`CLI.cpp` — Argument parsing, config loading, headless slicing, G-code export.

### src/libvgcode/ — G-code Visualization
Standalone library for rendering G-code toolpaths in 3D.

### src/libseqarrange/ — Sequential Print Arrangement
Library for computing collision-free sequential print ordering.

### src/clipper/ — Clipper2 Polygon Library
Updated Clipper polygon clipping library.

### tests/ — Test Suites
| Dir | Contents |
|-----|---------|
| `fff_print/` | FFF print pipeline integration tests |
| `libslic3r/` | Core library unit tests |
| `sla_print/` | SLA pipeline tests |
| `arrange/` | Arrangement algorithm tests |
| `slic3rutils/` | Utility tests |
| `thumbnails/` | Thumbnail generation tests |
| `cpp17/` | C++17 feature tests |
| `example/` | Example test structure |
