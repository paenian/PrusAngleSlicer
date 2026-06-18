# Architecture Overview — PrusaSlicer

## High-Level Architecture

PrusaSlicer is a layered monolith with a clean separation between core library, GUI, and CLI.

```
+----------------------------------------------------------+
|                    User Interface Layer                  |
|   src/slic3r/GUI/        (wxWidgets desktop GUI)         |
|   src/CLI/               (headless command-line)         |
+----------------------------------------------------------+
|                    Orchestration Layer                   |
|   src/libslic3r/Print.hpp / Print.cpp                    |
|   src/libslic3r/SLAPrint.hpp / SLAPrintSteps.cpp         |
|   BackgroundSlicingProcess (GUI background thread)       |
+----------------------------------------------------------+
|                    Slicing Pipeline Layer                |
|   PrintObject (per-object pipeline steps)                |
|   PerimeterGenerator (Classic + Arachne)                 |
|   Fill/* (14+ infill patterns)                           |
|   Support/* (Grid/Snug/Tree/Organic)                     |
|   GCode/* (toolpath → G-code translation)                |
+----------------------------------------------------------+
|                    Core Geometry Layer                   |
|   Geometry.hpp, ClipperUtils, ExPolygon, Polygon         |
|   Arachne (variable-width contour engine)                |
|   TriangleMeshSlicer, AABBTree*, EdgeGrid                |
+----------------------------------------------------------+
|                    Data Model Layer                      |
|   Model / ModelObject / ModelVolume / ModelInstance      |
|   PrintConfig / PrintObjectConfig / PrintRegionConfig    |
+----------------------------------------------------------+
|                    Third-Party Libraries                 |
|   Boost, Eigen, TBB (Intel), OpenVDB, CGAL, Clipper      |
|   wxWidgets (GUI only), imgui (3D overlay UI)            |
+----------------------------------------------------------+
```

## Component Responsibilities

### src/libslic3r/ — Core Library
The heart of PrusaSlicer. Technology-neutral, no GUI dependencies. Responsible for:
- Full FFF slicing pipeline (slice → perimeters → infill → supports → G-code)
- SLA slicing pipeline
- All geometry operations
- Configuration system (PrintConfig hierarchy)
- File format I/O (STL, OBJ, 3MF, AMF, STEP, G-code, SLA formats)

### src/slic3r/GUI/ — Desktop GUI
wxWidgets-based UI (~400+ files). Responsible for:
- 3D viewport (OpenGL via 3DScene, GLCanvas3D)
- Plater (main workspace with object list, sidebar, bed visualization)
- Print settings dialogs
- Background slicing process management (BackgroundSlicingProcess)
- Gizmos (support painting, seam painting, multi-material painting, text embossing, cut)
- Preset management UI (profiles for printer, filament, print settings)

### src/CLI/ — Command-Line Interface
Headless slicing for automation. Accepts `.3mf`, `.stl`, `.obj` + config overrides, outputs `.gcode`.

### bundled_deps/ — In-Tree Third-Party Code
Libraries requiring significant Prusa customization, bundled in source:
- admesh (STL repair), agg (2D rendering), avrdude (firmware flash), clipper (polygon clipping),
  glu-libtess (tessellation), hidapi (USB HID), imgui (in-3D-viewport UI), libigl, libnest2d (arrangement),
  miniz (ZIP/deflate), int128, fast_float, ankerl (hash maps), localesutils

### deps/ — External Dependency Bootstrap
CMake scripts to download, configure, and statically build all external dependencies:
- Boost, Eigen, TBB, OpenVDB, wxWidgets, CGAL, GMP, MPFR, OpenSSL, CURL, EXPAT,
  OpenEXR, Blosc, LibBGCode, NanoSVG, NLopt, OpenCSG, Catch2, Cereal, GLEW, JPEG

## Key Design Patterns

### Step-Based Pipeline with Invalidation
`PrintObjectStep` enum defines ordered pipeline steps: `posSlice → posPerimeters → posPrepareInfill → posInfill → posIroning → posSupportSpotsSearch → posSupportMaterial → posEstimateCurledExtrusions → posCalculateOverhangingPerimeters`

Each step has state (Fresh/Started/Done/Invalidated). When configuration changes, only steps that depend on changed parameters are invalidated and re-run.

### Region-Based Configuration
A `PrintRegion` groups volumes with the same print parameters (extruder assignment, perimeter count, infill density, etc.). `PrintObjectRegions` stores per-layer region assignments. This enables per-object and per-volume config overrides.

### Parallel Processing with TBB
Slicing steps use `oneapi::tbb::parallel_for` for layer-parallel processing. G-code generation uses a parallel pipeline across layers.

### Feature Extension Pattern
New features live in `src/libslic3r/Feature/` (e.g., FullSpectrum virtual extruders, FuzzySkin painting, Interlocking geometry). These are minimally invasive additions that hook into the existing pipeline at specific points.
