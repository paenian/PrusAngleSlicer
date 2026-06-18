///|/ Copyright (c) 2026 Paul Chase
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef slic3r_AngledSlicingEngine_hpp_
#define slic3r_AngledSlicingEngine_hpp_

#include <vector>
#include <Eigen/Geometry>
#include "libslic3r/ExPolygon.hpp"
#include "libslic3r/TriangleMesh.hpp"
#include "AngledSlicingParams.hpp"

namespace Slic3r::AngledSlicing {

/// Represents a tilted slicing plane.
/// Plane equation: normal · point = distance
struct SlicingPlane {
    Eigen::Vector3d normal;    ///< Unit normal vector of the plane
    double          distance;  ///< Signed distance from origin along normal
    double          print_z;   ///< The Z height at the center of this plane (for G-code layer changes)
};

/// Results from slicing at one tilted plane.
struct TiltedSliceResult {
    ExPolygons  contours;   ///< 2D contours projected onto XY plane
    double      print_z;    ///< Nominal Z height for this layer (center of tilted plane at object center)
    double      height;     ///< Layer thickness (perpendicular distance between adjacent planes)
};

/// Slice a mesh with a set of tilted planes.
///
/// The mesh is NOT moved. Each plane is defined by a normal and distance.
/// The intersection contour is projected onto the XY plane (dropping the Z
/// variation within the contour — Z is handled per-point in G-code generation).
///
/// @param mesh         The triangle mesh to slice (in model-local coordinates)
/// @param planes       Ordered set of tilted planes (in print-position coordinates)
/// @param trafo        Transform from model-local to print-position coordinates
/// @return             One TiltedSliceResult per plane (contours + metadata)
std::vector<TiltedSliceResult> slice_mesh_tilted(
    const indexed_triangle_set   &mesh,
    const std::vector<SlicingPlane> &planes,
    const Eigen::Transform<double, 3, Eigen::Affine> &trafo = Eigen::Transform<double, 3, Eigen::Affine>::Identity());

/// Generate the set of tilted slicing planes for given parameters.
/// Planes are spaced by layer_height (perpendicular distance) and oriented
/// according to the tilt angle and direction.
///
/// @param params       Angled slicing configuration
/// @param layer_height Layer height in mm
/// @param object_bbox  Bounding box of the object (to determine how many planes needed)
/// @return             Ordered planes from bottom to top
std::vector<SlicingPlane> generate_tilted_planes(
    const Params &params,
    double layer_height,
    double first_layer_height,
    const Eigen::AlignedBox<double, 3> &object_bbox);

} // namespace Slic3r::AngledSlicing

#endif // slic3r_AngledSlicingEngine_hpp_
