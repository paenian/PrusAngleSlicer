///|/ Copyright (c) 2026 Paul Chase
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef slic3r_AngledSlicingZSchedule_hpp_
#define slic3r_AngledSlicingZSchedule_hpp_

#include <vector>
#include <Eigen/Geometry>

#include "AngledSlicingParams.hpp"
#include "AngledSlicingTransform.hpp"
#include "libslic3r/Slicing.hpp"
#include "libslic3r/BoundingBox.hpp"

namespace Slic3r::AngledSlicing {

/// Information about a single layer in the angled Z schedule.
struct LayerZInfo {
    coordf_t print_z;        ///< Top of layer in world Z (what the printer moves to)
    coordf_t slice_z;        ///< Center of layer (for mesh intersection)
    coordf_t height;         ///< Layer thickness
    bool     is_first_layer; ///< True if this layer's underside touches Z=0 in world frame
};

/// Computes the Z-height schedule for angled slicing.
///
/// When the mesh is rotated, it occupies a different Z range. This class
/// computes the layer heights needed to cover that expanded range, while
/// respecting the configured layer_height and first_layer_height.
class ZSchedule
{
public:
    /// Compute the complete layer schedule for an angled print.
    ///
    /// @param slicing_params  Standard slicing parameters (layer heights, raft, etc.)
    /// @param angle_params    Angled slicing configuration
    /// @param rotated_bbox    Bounding box of the mesh AFTER rotation transform is applied
    /// @return                Ordered vector of LayerZInfo (ascending print_z)
    ///
    /// Guarantees:
    /// - result is non-empty (at least one layer)
    /// - result is sorted ascending by print_z
    /// - all heights > 0
    /// - first element has print_z > 0
    static std::vector<LayerZInfo> compute(
        const SlicingParameters                    &slicing_params,
        const Params                               &angle_params,
        const Eigen::AlignedBox<double, 3>         &rotated_bbox);

    /// Compute effective max print height after rotation.
    /// Used to check against printer Z limit before slicing.
    static coordf_t effective_print_height(
        const Eigen::AlignedBox<double, 3>  &original_bbox,
        const Params                        &angle_params);
};

} // namespace Slic3r::AngledSlicing

#endif // slic3r_AngledSlicingZSchedule_hpp_
