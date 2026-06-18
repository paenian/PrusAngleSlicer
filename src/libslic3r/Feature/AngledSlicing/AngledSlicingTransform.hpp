///|/ Copyright (c) 2026 Paul Chase
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef slic3r_AngledSlicingTransform_hpp_
#define slic3r_AngledSlicingTransform_hpp_

#include <Eigen/Geometry>
#include "AngledSlicingParams.hpp"

namespace Slic3r::AngledSlicing {

/// Builds Transform3d matrices for angled slicing.
/// 
/// The key insight: instead of computing tilted plane intersections directly,
/// we apply a Z-shear to the mesh. A Z-shear modifies only the Z coordinate
/// based on XY position: z_new = z - x*tan(angle)*cos(dir) - y*tan(angle)*sin(dir)
///
/// This maps tilted planes to horizontal ones while preserving XY coordinates.
/// After the shear, standard horizontal slicing produces contours that are
/// geometrically identical to what you'd get intersecting the original mesh
/// with tilted planes — but the XY coordinates are unchanged.
///
/// This means:
/// - The printed geometry is identical (XY paths don't change)
/// - Only layer boundaries are tilted
/// - Support material works correctly (overhang detection uses layer adjacency)
/// - All downstream features operate normally on the resulting 2D contours
class Transform
{
public:
    /// Build the rotation matrix that tilts the mesh for angled slicing.
    /// Compose this AFTER the existing object trafo:
    ///   final_trafo = existing_trafo * build_rotation(params)
    ///
    /// When params.enabled() == false, returns identity.
    static Eigen::Transform<double, 3, Eigen::Affine> build_rotation(const Params &params);

    /// Inverse of build_rotation — used to transform coordinates back to world frame.
    static Eigen::Transform<double, 3, Eigen::Affine> build_inverse_rotation(const Params &params);

    /// Compute the Z-height of the rotated mesh bounding box.
    /// The mesh occupies more Z range after rotation:
    ///   new_z_range ≈ original_height / cos(angle) + width_in_tilt_direction * sin(angle)
    /// 
    /// @param original_bbox  Bounding box of the mesh AFTER the standard object trafo (centered)
    /// @param params         Angled slicing parameters
    /// @return               The Z extent of the bounding box after rotation
    static double compute_rotated_z_extent(const Eigen::AlignedBox<double, 3> &original_bbox,
                                           const Params &params);
};

} // namespace Slic3r::AngledSlicing

#endif // slic3r_AngledSlicingTransform_hpp_
