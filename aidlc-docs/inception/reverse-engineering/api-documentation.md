# API Documentation — PrusaSlicer Key Interfaces

## Primary Pipeline APIs

### Print — Top-Level Orchestrator

```cpp
// Apply model + config changes, returns status indicating what was invalidated.
// Call this before process() whenever the model or config changes.
Print::ApplyStatus Print::apply(const Model &model, DynamicPrintConfig config);

// Execute all invalidated pipeline steps. Blocking. Calls PrintObject steps internally.
// Throws CanceledException via print->throw_if_canceled().
void Print::process();

// Export G-code to file. Returns the output file path.
std::string Print::export_gcode(const std::string& path_template,
                                GCodeProcessorResult* result,
                                ThumbnailsGeneratorCallback thumbnail_cb = nullptr);
```

### PrintObject — Per-Object Pipeline Steps

```cpp
// Publicly called only by Print::process() and friends. Steps run in order:
void PrintObject::slice();                       // posSlice
void PrintObject::make_perimeters();             // posPerimeters (calls slice())
void PrintObject::prepare_infill();              // posPrepareInfill
void PrintObject::infill();                      // posInfill
void PrintObject::ironing();                     // posIroning
void PrintObject::generate_support_spots();      // posSupportSpotsSearch
void PrintObject::generate_support_material();   // posSupportMaterial
void PrintObject::estimate_curled_extrusions();  // posEstimateCurledExtrusions
void PrintObject::calculate_overhanging_perimeters(); // posCalculateOverhangingPerimeters

// Invalidation — used when config changes:
bool PrintObject::invalidate_step(PrintObjectStep step);
bool PrintObject::invalidate_state_by_config_options(
    const ConfigOptionResolver &old_config,
    const ConfigOptionResolver &new_config,
    const std::vector<t_config_option_key> &opt_keys);
```

### PerimeterGenerator — Perimeter Generation

```cpp
namespace Slic3r::PerimeterGenerator {

// Classic mode: fixed-width Clipper offset perimeters
void process_classic(
    const Parameters           &params,     // layer height, flows, configs
    const Surface              &surface,    // the slice to generate perimeters for
    const ExPolygons           *lower_slices,
    const ExPolygons           *upper_slices,
    Polygons                   &lower_slices_polygons_cache,
    ExtrusionEntityCollection  &out_loops,
    ExtrusionEntityCollection  &out_gap_fill,
    ExPolygons                 &out_fill_expolygons);

// Arachne mode: variable-width perimeters
void process_arachne(
    const Parameters           &params,
    const Surface              &surface,
    const ExPolygons           *lower_slices,
    const ExPolygons           *upper_slices,
    Polygons                   &lower_slices_polygons_cache,
    ExtrusionEntityCollection  &out_loops,
    ExtrusionEntityCollection  &out_gap_fill,
    ExPolygons                 &out_fill_expolygons);

} // namespace Slic3r::PerimeterGenerator
```

### Fill — Infill Generation

```cpp
class Fill {
public:
    // Factory method — creates correct Fill subclass for given pattern enum
    static Fill* new_from_type(const InfillPattern type);
    static Fill* new_from_type(const std::string &type);

    // Generate infill polylines for a surface. Override this in new patterns.
    virtual Polylines fill_surface(const Surface *surface, const FillParams &params);

    // Arachne variant for variable-width infill (Concentric, FillEnsuring)
    virtual ThickPolylines fill_surface_arachne(const Surface *surface, const FillParams &params);

    // Configuration properties set before calling fill_surface():
    size_t      layer_id;
    coordf_t    z;
    coordf_t    spacing;
    coordf_t    overlap;
    float       angle;        // in radians, CCW, 0 = East
    BoundingBox bounding_box;
};
```

### GCodeGenerator — G-code Export

```cpp
class GCodeGenerator {
public:
    // Main entry point — exports G-code for the entire print
    void do_export(Print* print, const char* path,
                   GCodeProcessorResult* result = nullptr,
                   ThumbnailsGeneratorCallback thumbnail_cb = nullptr);
};
```

## Configuration API

### Reading Config Values (StaticPrintConfig pattern)

```cpp
// Access from PrintObject context:
const PrintObjectConfig &obj_config = print_object.config();
double layer_height = obj_config.layer_height.value;
int perimeters = obj_config.perimeters.value;

// Access from PrintRegion context:
const PrintRegionConfig &region_config = print_region.config();
InfillPattern pattern = region_config.fill_pattern.value;
double density = region_config.fill_density.value;

// Full config with all values merged:
const FullPrintConfig &full_config = gcode_generator.config();
```

### Adding a New Config Parameter (pattern to follow)

```cpp
// In PrintConfig.hpp, inside the appropriate StaticConfig class:
// 1. Add the field:
ConfigOptionFloat my_new_param;

// 2. In PrintConfig.cpp, register in the STATIC_PRINT_CONFIG_CACHE initialize() function:
OPT_PTR(my_new_param);

// 3. In PrintConfigDef constructor, define the option:
def = this->add("my_new_param", coFloat);
def->label = L("My New Parameter");
def->tooltip = L("Description of what this does");
def->sidetext = L("mm");
def->min = 0;
def->set_default_value(new ConfigOptionFloat(0.0));
```

## Key Data Type APIs

### ExPolygon — Core 2D Geometry

```cpp
struct ExPolygon {
    Polygon         contour;   // Outer boundary (CCW)
    Polygons        holes;     // Inner holes (CW)
};
using ExPolygons = std::vector<ExPolygon>;
```

### Layer Access

```cpp
// Iterate all layers of a PrintObject:
for (const Layer *layer : print_object.layers()) {
    coordf_t z      = layer->print_z;
    coordf_t height = layer->height;
    
    // Access region-specific data:
    for (size_t i = 0; i < layer->region_count(); ++i) {
        const LayerRegion *lr = layer->get_region(i);
        const SurfaceCollection &slices = lr->slices();
        const ExtrusionEntityCollection &perims = lr->perimeters();
        const ExtrusionEntityCollection &fills = lr->fills();
    }
}
```

## File Format API (src/libslic3r/Format/)

| Function | File | Purpose |
|----------|------|---------|
| `load_stl()` | `Format/STL.hpp` | Load STL into TriangleMesh |
| `load_obj()` | `Format/OBJ.hpp` | Load OBJ into TriangleMesh |
| `load_3mf()` | `Format/3MF.hpp` | Load 3MF project (model + config) |
| `store_3mf()` | `Format/3MF.hpp` | Save 3MF project |
| `load_amf()` | `Format/AMF.hpp` | Load AMF (XML-based) |
| `GCodeReader` | `GCodeReader.hpp` | Parse existing G-code file |
