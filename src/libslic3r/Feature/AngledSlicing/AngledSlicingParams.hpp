///|/ Copyright (c) 2026 Paul Chase
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef slic3r_AngledSlicingParams_hpp_
#define slic3r_AngledSlicingParams_hpp_

#include <cmath>

#include "libslic3r/PrintConfig.hpp"

namespace Slic3r::AngledSlicing {

/// Immutable value object holding angled slicing configuration.
/// Constructed once per PrintObject::slice_volumes() call and passed
/// by const reference to all angled slicing components.
struct Params
{
    /// Create from the object's print configuration.
    static Params from_config(const PrintObjectConfig &config)
    {
        return Params(config.angled_slicing_angle.value,
                      config.angled_slicing_direction.value);
    }

    /// Returns a disabled (identity) configuration.
    static Params disabled() { return Params(0.0, 0.0); }

    /// True when angled slicing is active (angle > epsilon).
    bool   enabled()              const { return m_angle_deg > 1e-6; }

    double tilt_angle_deg()       const { return m_angle_deg; }
    double tilt_direction_deg()   const { return m_direction_deg; }
    double tilt_angle_rad()       const { return m_angle_deg * M_PI / 180.0; }
    double tilt_direction_rad()   const { return m_direction_deg * M_PI / 180.0; }

private:
    double m_angle_deg     { 0.0 };
    double m_direction_deg { 0.0 };

    Params(double angle, double direction)
        : m_angle_deg(angle), m_direction_deg(direction) {}
};

} // namespace Slic3r::AngledSlicing

#endif // slic3r_AngledSlicingParams_hpp_
