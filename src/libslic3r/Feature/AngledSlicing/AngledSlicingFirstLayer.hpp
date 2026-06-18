///|/ Copyright (c) 2026 Paul Chase
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef slic3r_AngledSlicingFirstLayer_hpp_
#define slic3r_AngledSlicingFirstLayer_hpp_

#include "AngledSlicingZSchedule.hpp"
#include "libslic3r/ExPolygon.hpp"

namespace Slic3r::AngledSlicing {

/// Handles first-layer detection and bed-plane clipping for angled slicing.
///
/// When the mesh is rotated, parts of it extend below Z=0 (the bed plane).
/// These regions need to be clipped, and the layers that originally touched
/// the bed need special treatment (first-layer speed, temperature, etc.).
class FirstLayer
{
public:
    /// Returns true if the given layer info indicates it touches the bed.
    static bool is_first_layer(const LayerZInfo &info) { return info.is_first_layer; }

    /// Count how many layers in the schedule are first-layers (touch bed).
    static size_t count_first_layers(const std::vector<LayerZInfo> &schedule);
};

} // namespace Slic3r::AngledSlicing

#endif // slic3r_AngledSlicingFirstLayer_hpp_
