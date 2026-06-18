///|/ Copyright (c) 2026 Paul Chase
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "AngledSlicingTransform.hpp"

#include <cmath>

namespace Slic3r::AngledSlicing {

Eigen::Transform<double, 3, Eigen::Affine> Transform::build_rotation(const Params &params)
{
    Eigen::Transform<double, 3, Eigen::Affine> result = Eigen::Transform<double, 3, Eigen::Affine>::Identity();

    if (!params.enabled())
        return result;

    const double angle_rad     = params.tilt_angle_rad();
    const double direction_rad = params.tilt_direction_rad();

    // Rotation axis is perpendicular to the tilt direction in the XY plane.
    // direction=0 (tilt toward +X) → axis is Y (0,1,0)
    // direction=90 (tilt toward +Y) → axis is -X (-1,0,0)
    // General: axis = (-sin(direction), cos(direction), 0)
    Eigen::Vector3d axis(-std::sin(direction_rad), std::cos(direction_rad), 0.0);
    axis.normalize();

    // Rotate the mesh by -angle. This makes horizontal slicing planes
    // equivalent to tilted planes in the original mesh frame.
    result.rotate(Eigen::AngleAxisd(-angle_rad, axis));

    return result;
}

Eigen::Transform<double, 3, Eigen::Affine> Transform::build_inverse_rotation(const Params &params)
{
    Eigen::Transform<double, 3, Eigen::Affine> result = Eigen::Transform<double, 3, Eigen::Affine>::Identity();

    if (!params.enabled())
        return result;

    const double angle_rad     = params.tilt_angle_rad();
    const double direction_rad = params.tilt_direction_rad();

    Eigen::Vector3d axis(-std::sin(direction_rad), std::cos(direction_rad), 0.0);
    axis.normalize();

    // Positive angle — inverse of build_rotation.
    result.rotate(Eigen::AngleAxisd(angle_rad, axis));

    return result;
}

double Transform::compute_rotated_z_extent(const Eigen::AlignedBox<double, 3> &original_bbox,
                                           const Params &params)
{
    if (!params.enabled())
        return original_bbox.sizes().z();

    // Rotate all 8 corners and find the new Z range.
    auto rotation = build_rotation(params);

    double z_min = std::numeric_limits<double>::max();
    double z_max = std::numeric_limits<double>::lowest();

    for (int i = 0; i < 8; ++i) {
        Eigen::Vector3d corner(
            (i & 1) ? original_bbox.max().x() : original_bbox.min().x(),
            (i & 2) ? original_bbox.max().y() : original_bbox.min().y(),
            (i & 4) ? original_bbox.max().z() : original_bbox.min().z()
        );
        Eigen::Vector3d rotated = rotation * corner;
        z_min = std::min(z_min, rotated.z());
        z_max = std::max(z_max, rotated.z());
    }

    return z_max - z_min;
}

} // namespace Slic3r::AngledSlicing
