# Component Methods — Angled Slicing Feature

Note: Detailed business logic and algorithms are specified in Functional Design (Construction phase). This document defines method signatures, responsibilities, and I/O contracts.

---

## AngledSlicingParams

**All methods inline / constexpr, no .cpp needed.**

```cpp
namespace Slic3r::AngledSlicing {

struct Params {
    // Factory: create from PrintObjectConfig values
    static Params from_config(const PrintObjectConfig &config);
    // Factory: create disabled (angle=0, identity transform)
    static Params disabled();

    bool     enabled()             const;  // true iff tilt_angle_deg > epsilon
    double   tilt_angle_deg()      const;  // [0, 90)
    double   tilt_direction_deg()  const;  // [0, 360)
    double   tilt_angle_rad()      const;  // derived
    double   tilt_direction_rad()  const;  // derived

private:
    double m_angle_deg     { 0.0 };
    double m_direction_deg { 0.0 };
};

} // namespace Slic3r::AngledSlicing
```

---

## AngledSlicingTransform

```cpp
namespace Slic3r::AngledSlicing {

class Transform {
public:
    // Build the rotation matrix R that maps the tilted coordinate frame to world frame.
    // Composing existing_trafo * R into MeshSlicingParams::trafo is the integration point.
    // Input:  Params with angle/direction
    // Output: Eigen::Transform3d rotation matrix (pure rotation, no translation)
    // When params.enabled() == false, returns identity.
    static Transform3d build_rotation(const Params &params);

    // Inverse rotation: world frame → tilted frame.
    // Used for converting world-Z heights back to perpendicular-layer heights.
    static Transform3d build_inverse_rotation(const Params &params);

    // Given a target print_z in world coordinates and the tilt params,
    // return the slice_z (the Z coordinate to pass to TriangleMeshSlicer in
    // the rotated mesh frame) for a layer at that print_z.
    // Note: in the rotated frame, all "horizontal" slices are at world Z heights
    // after the rotation is applied — so slice_z == print_z in the standard case.
    static float world_z_to_slice_z(float world_z, const Params &params);
};

} // namespace Slic3r::AngledSlicing
```

---

## AngledSlicingZSchedule

```cpp
namespace Slic3r::AngledSlicing {

struct LayerZInfo {
    coordf_t print_z;      // Top of layer in world Z (what printer moves to)
    coordf_t slice_z;      // Center of layer for slicing (world Z - height/2)
    coordf_t height;       // Layer height (perpendicular to tilt plane)
    bool     is_first_layer; // True if this layer intersects Z=0 (bed plane)
    bool     is_clipped;   // True if bottom of layer is below Z=0 (partial layer)
};

class ZSchedule {
public:
    // Build the complete Z schedule for a tilted print.
    // Replaces generate_object_layers() for angled slicing.
    // Input:  SlicingParameters (existing, provides layer_height, object height, raft info)
    //         Params (angle/direction for effective Z scaling)
    // Output: Ordered vector of LayerZInfo from bottom to top
    // Guarantee: result[0].print_z > 0 always (first layer above bed)
    //            result is sorted ascending by print_z
    static std::vector<LayerZInfo> compute(
        const SlicingParameters &slicing_params,
        const Params            &angle_params);

    // Effective Z height of the full object after tilt expansion.
    // = object_height / cos(tilt_angle)
    // Used to check against printer max Z before slicing.
    static coordf_t effective_print_height(coordf_t object_height, const Params &params);

    // Number of layers that simultaneously intersect Z=0 (first-layer count).
    // = ceil(object_width_in_tilt_direction * tan(angle) / layer_height)
    // Used to warn the user about long first-layer sequences.
    static size_t first_layer_count(const BoundingBoxf3 &bbox, const Params &params,
                                    coordf_t layer_height);
};

} // namespace Slic3r::AngledSlicing
```

---

## AngledSlicingFirstLayer

```cpp
namespace Slic3r::AngledSlicing {

class FirstLayer {
public:
    // Given a Layer and the ZSchedule, return true if this layer qualifies for
    // first-layer treatment (speed, temperature, fan override).
    // A layer qualifies if any part of its underside touches the bed (Z <= 0).
    static bool is_first_layer(const LayerZInfo &layer_info);

    // Clip the ExPolygons of a layer that extend below Z=0 to the bed plane.
    // This produces the valid printable region for partial first layers.
    // Input:  raw ExPolygons from TriangleMeshSlicer, layer_info with print_z/height
    // Output: ExPolygons clipped to region above Z=0
    // When is_clipped == false, returns input unchanged.
    static ExPolygons clip_to_bed(
        const ExPolygons    &raw_slices,
        const LayerZInfo    &layer_info,
        const Params        &angle_params);
};

} // namespace Slic3r::AngledSlicing
```

---

## PrintConfig Changes (additions only)

```cpp
// In PrintObjectConfig class (PrintConfig.hpp):
ConfigOptionFloat   angled_slicing_angle;       // degrees [0, 89], default 0
ConfigOptionFloat   angled_slicing_direction;   // degrees [0, 360), default 0

// In PrintConfig.cpp PrintConfigDef constructor:
// def->group = "Layers and Perimeters"
// def->label = L("Angled Slicing Angle")
// def->tooltip = L("Tilt angle of slicing planes in degrees. 0 = standard horizontal slicing.")
// def->min = 0; def->max = 89;
// def->set_default_value(new ConfigOptionFloat(0.0));
// Same pattern for angled_slicing_direction.
```

---

## PrintObject::slice_volumes() Integration Hook

```cpp
// Location: PrintObject.cpp, inside slice_volumes(), after existing trafo computation
// Existing code (unchanged):
//   MeshSlicingParamsEx params;
//   params.trafo = this->trafo_centered();
//   std::vector<float> zs = ...;  // standard z heights

// New code (added in conditional block):
//   const auto angle_params = AngledSlicing::Params::from_config(this->config());
//   if (angle_params.enabled()) {
//       // Compose tilt rotation into the existing transform
//       params.trafo = params.trafo * AngledSlicing::Transform::build_rotation(angle_params);
//       // Replace standard Z heights with angled schedule
//       auto schedule = AngledSlicing::ZSchedule::compute(m_slicing_params, angle_params);
//       zs = extract_slice_zs(schedule);  // helper to pull slice_z values
//       // Store schedule for first-layer detection after slicing
//       m_angled_slicing_schedule = std::move(schedule);
//   }
```
