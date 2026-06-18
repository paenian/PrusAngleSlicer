///|/ Copyright (c) 2026 Paul Chase
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "AngledSlicingFirstLayer.hpp"

#include <algorithm>

namespace Slic3r::AngledSlicing {

size_t FirstLayer::count_first_layers(const std::vector<LayerZInfo> &schedule)
{
    return std::count_if(schedule.begin(), schedule.end(),
        [](const LayerZInfo &info) { return info.is_first_layer; });
}

} // namespace Slic3r::AngledSlicing
