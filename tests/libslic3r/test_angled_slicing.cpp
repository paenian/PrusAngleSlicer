///|/ Copyright (c) 2026 Paul Chase
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "libslic3r/PrintConfig.hpp"
#include "libslic3r/Feature/AngledSlicing/AngledSlicingParams.hpp"

using namespace Slic3r;
using namespace Slic3r::AngledSlicing;
using Catch::Approx;

TEST_CASE("AngledSlicing::Params - disabled when angle is zero", "[AngledSlicing]") {
    auto params = Params::disabled();
    REQUIRE_FALSE(params.enabled());
    REQUIRE(params.tilt_angle_deg() == Approx(0.0));
    REQUIRE(params.tilt_direction_deg() == Approx(0.0));
    REQUIRE(params.tilt_angle_rad() == Approx(0.0));
    REQUIRE(params.tilt_direction_rad() == Approx(0.0));
}

TEST_CASE("AngledSlicing::Params - from_config reads values correctly", "[AngledSlicing]") {
    // Create a full config with defaults, then override
    DynamicPrintConfig dynamic_config = DynamicPrintConfig::full_print_config();
    dynamic_config.set_deserialize_strict("angled_slicing_angle", "30");
    dynamic_config.set_deserialize_strict("angled_slicing_direction", "45");

    PrintObjectConfig obj_config;
    obj_config.apply(dynamic_config, true);

    auto params = Params::from_config(obj_config);
    REQUIRE(params.enabled());
    REQUIRE(params.tilt_angle_deg() == Approx(30.0));
    REQUIRE(params.tilt_direction_deg() == Approx(45.0));
    REQUIRE(params.tilt_angle_rad() == Approx(30.0 * M_PI / 180.0));
    REQUIRE(params.tilt_direction_rad() == Approx(45.0 * M_PI / 180.0));
}

TEST_CASE("AngledSlicing::Params - from_config with zero angle is disabled", "[AngledSlicing]") {
    DynamicPrintConfig dynamic_config = DynamicPrintConfig::full_print_config();
    // defaults should be 0
    PrintObjectConfig obj_config;
    obj_config.apply(dynamic_config, true);

    auto params = Params::from_config(obj_config);
    REQUIRE_FALSE(params.enabled());
}

TEST_CASE("AngledSlicing config - defaults are zero", "[AngledSlicing]") {
    PrintObjectConfig config = PrintObjectConfig::defaults();
    REQUIRE(config.angled_slicing_angle.value == Approx(0.0));
    REQUIRE(config.angled_slicing_direction.value == Approx(0.0));
}

TEST_CASE("AngledSlicing config - serialization roundtrip", "[AngledSlicing]") {
    DynamicPrintConfig config;
    config.set_deserialize_strict("angled_slicing_angle", "25.5");
    config.set_deserialize_strict("angled_slicing_direction", "135");

    // Verify values were stored correctly
    REQUIRE(config.opt_float("angled_slicing_angle") == Approx(25.5));
    REQUIRE(config.opt_float("angled_slicing_direction") == Approx(135.0));
}

TEST_CASE("AngledSlicing::Params - radian conversion invariants", "[AngledSlicing][PBT]") {
    // Property: tilt_angle_rad() == tilt_angle_deg() * PI / 180 for all valid angles
    for (double angle = 0.0; angle <= 89.0; angle += 5.0) {
        DynamicPrintConfig dynamic_config = DynamicPrintConfig::full_print_config();
        dynamic_config.set_deserialize_strict("angled_slicing_angle", std::to_string(angle));

        PrintObjectConfig obj_config;
        obj_config.apply(dynamic_config, true);

        auto params = Params::from_config(obj_config);
        REQUIRE(params.tilt_angle_rad() == Approx(angle * M_PI / 180.0));
    }
}

TEST_CASE("AngledSlicing::Params - enabled predicate invariant", "[AngledSlicing][PBT]") {
    // Property: enabled() == (angle > 1e-6) for all valid angles
    for (double angle : {0.0, 0.0000001, 0.001, 1.0, 45.0, 89.0}) {
        DynamicPrintConfig dynamic_config = DynamicPrintConfig::full_print_config();
        dynamic_config.set_deserialize_strict("angled_slicing_angle", std::to_string(angle));

        PrintObjectConfig obj_config;
        obj_config.apply(dynamic_config, true);

        auto params = Params::from_config(obj_config);
        REQUIRE(params.enabled() == (angle > 1e-6));
    }
}
