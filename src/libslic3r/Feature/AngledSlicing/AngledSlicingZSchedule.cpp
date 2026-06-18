///|/ Copyright (c) 2026 Paul Chase
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "AngledSlicingZSchedule.hpp"

#include <algorithm>
#include <cmath>
#include <cassert>

namespace Slic3r::AngledSlicing {

std::vector<LayerZInfo> ZSchedule::compute(
    const SlicingParameters                    &slicing_params,
    const Params                               &angle_params,
    const Eigen::AlignedBox<double, 3>         &rotated_bbox)
{
    std::vector<LayerZInfo> schedule;

    // Total height of the rotated mesh that needs to be sliced.
    // The rotated_bbox.min().z() may be negative (mesh extends below Z=0 after rotation).
    // We only slice from Z=0 upward (anything below is the bed).
    const double z_bottom = std::max(0.0, rotated_bbox.min().z());
    const double z_top    = rotated_bbox.max().z();

    if (z_top <= z_bottom)
        return schedule; // Degenerate: mesh is entirely below or at bed

    const double total_height   = z_top - z_bottom;
    const double layer_height   = slicing_params.layer_height;
    const double first_layer_h  = slicing_params.first_print_layer_height;

    // How many layers below Z=0? These represent the "multiple first layers"
    // that simultaneously touch the bed. We compute based on how far below 0
    // the rotated mesh extends.
    const double below_bed_extent = std::max(0.0, -rotated_bbox.min().z());

    // Generate layers from bottom (just above bed) to top of rotated mesh.
    // First layer uses first_layer_height, subsequent layers use regular layer_height.
    double current_z = 0.0;
    int layer_id = 0;

    // First layer
    {
        double h = std::min(first_layer_h, total_height);
        double print_z = current_z + h;
        double slice_z = current_z + h * 0.5;

        // A layer is a "first layer" if any part of it (from print_z - h to print_z)
        // corresponds to geometry that was originally touching the bed.
        // In the rotated frame, this means the bottom of the layer overlaps with the
        // region that was at z <= 0 before clipping.
        bool is_first = true; // The actual first layer always touches bed

        schedule.push_back({print_z, slice_z, h, is_first});
        current_z = print_z;
        ++layer_id;
    }

    // Remaining layers
    while (current_z < z_top - layer_height * 0.5) {
        double h = layer_height;
        // Don't overshoot the top
        if (current_z + h > z_top)
            h = z_top - current_z;
        if (h < slicing_params.min_layer_height * 0.5)
            break; // Too thin to be useful

        double print_z = current_z + h;
        double slice_z = current_z + h * 0.5;

        // A layer is a "first layer" if its underside (current_z before this layer)
        // is still within the below-bed zone. In the rotated mesh frame, the region
        // from z_bottom=0 to z_bottom + below_bed_extent originally was below the bed.
        // After rotation and clipping to z>=0, layers whose underside < below_bed_extent
        // are considered "on the bed" (first-layer eligible).
        bool is_first = (current_z < below_bed_extent);

        schedule.push_back({print_z, slice_z, h, is_first});
        current_z = print_z;
        ++layer_id;
    }

    return schedule;
}

coordf_t ZSchedule::effective_print_height(
    const Eigen::AlignedBox<double, 3>  &original_bbox,
    const Params                        &angle_params)
{
    return Transform::compute_rotated_z_extent(original_bbox, angle_params);
}

} // namespace Slic3r::AngledSlicing
