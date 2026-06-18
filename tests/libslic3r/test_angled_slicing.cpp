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


// ============================================================================
// U2: Transform tests
// ============================================================================

#include "libslic3r/Feature/AngledSlicing/AngledSlicingTransform.hpp"
#include "libslic3r/Feature/AngledSlicing/AngledSlicingZSchedule.hpp"
#include "libslic3r/Feature/AngledSlicing/AngledSlicingFirstLayer.hpp"

using AngledTransform = Slic3r::AngledSlicing::Transform;
using Slic3r::AngledSlicing::ZSchedule;
using Slic3r::AngledSlicing::FirstLayer;
using Slic3r::AngledSlicing::LayerZInfo;

TEST_CASE("AngledSlicing::Transform - identity when disabled", "[AngledSlicing][PBT]") {
    auto params = Params::disabled();
    auto rot = AngledTransform::build_rotation(params);
    REQUIRE(rot.matrix().isApprox(Eigen::Matrix4d::Identity(), 1e-12));
}

TEST_CASE("AngledSlicing::Transform - rotation is orthonormal", "[AngledSlicing][PBT]") {
    // Property: rotation matrix determinant == 1 and R * R^T == I
    for (double angle : {5.0, 15.0, 30.0, 45.0, 60.0, 75.0, 89.0}) {
        for (double direction : {0.0, 45.0, 90.0, 180.0, 270.0}) {
            DynamicPrintConfig cfg = DynamicPrintConfig::full_print_config();
            cfg.set_deserialize_strict("angled_slicing_angle", std::to_string(angle));
            cfg.set_deserialize_strict("angled_slicing_direction", std::to_string(direction));
            PrintObjectConfig obj_cfg;
            obj_cfg.apply(cfg, true);
            auto params = Params::from_config(obj_cfg);

            auto rot = AngledTransform::build_rotation(params);
            Eigen::Matrix3d R = rot.rotation();

            // Check orthonormality: R * R^T should be identity
            Eigen::Matrix3d product = R * R.transpose();
            REQUIRE(product.isApprox(Eigen::Matrix3d::Identity(), 1e-10));

            // Check determinant == 1 (proper rotation, not reflection)
            REQUIRE(R.determinant() == Approx(1.0).margin(1e-10));
        }
    }
}

TEST_CASE("AngledSlicing::Transform - round-trip rotation", "[AngledSlicing][PBT]") {
    // Property: build_rotation * build_inverse_rotation == identity
    for (double angle : {10.0, 30.0, 60.0, 89.0}) {
        DynamicPrintConfig cfg = DynamicPrintConfig::full_print_config();
        cfg.set_deserialize_strict("angled_slicing_angle", std::to_string(angle));
        cfg.set_deserialize_strict("angled_slicing_direction", "135");
        PrintObjectConfig obj_cfg;
        obj_cfg.apply(cfg, true);
        auto params = Params::from_config(obj_cfg);

        auto rot = AngledTransform::build_rotation(params);
        auto inv = AngledTransform::build_inverse_rotation(params);

        Eigen::Matrix4d product = (rot * inv).matrix();
        REQUIRE(product.isApprox(Eigen::Matrix4d::Identity(), 1e-10));
    }
}

TEST_CASE("AngledSlicing::Transform - rotated Z extent is non-zero and differs from original", "[AngledSlicing]") {
    Eigen::AlignedBox<double, 3> bbox(Eigen::Vector3d(-10, -10, 0), Eigen::Vector3d(10, 10, 20));

    for (double angle : {5.0, 15.0, 30.0, 45.0, 60.0}) {
        DynamicPrintConfig cfg = DynamicPrintConfig::full_print_config();
        cfg.set_deserialize_strict("angled_slicing_angle", std::to_string(angle));
        cfg.set_deserialize_strict("angled_slicing_direction", "0");
        PrintObjectConfig obj_cfg;
        obj_cfg.apply(cfg, true);
        auto params = Params::from_config(obj_cfg);

        double extent = AngledTransform::compute_rotated_z_extent(bbox, params);
        // Rotated extent should always be positive
        REQUIRE(extent > 0.0);
        // For a box with non-zero width, tilting should change the extent
        REQUIRE(extent != Approx(20.0));
    }
}

TEST_CASE("AngledSlicing::ZSchedule - produces ascending print_z", "[AngledSlicing][PBT]") {
    SlicingParameters sp;
    sp.valid = true;
    sp.layer_height = 0.2;
    sp.first_print_layer_height = 0.2;
    sp.min_layer_height = 0.05;

    DynamicPrintConfig cfg = DynamicPrintConfig::full_print_config();
    cfg.set_deserialize_strict("angled_slicing_angle", "20");
    cfg.set_deserialize_strict("angled_slicing_direction", "0");
    PrintObjectConfig obj_cfg;
    obj_cfg.apply(cfg, true);
    auto params = Params::from_config(obj_cfg);

    // Rotated bbox: mesh originally 0-20mm tall, 20mm wide
    auto rot = AngledTransform::build_rotation(params);
    Eigen::AlignedBox<double, 3> original_bbox(Eigen::Vector3d(-10, -10, 0), Eigen::Vector3d(10, 10, 20));
    Eigen::AlignedBox<double, 3> rotated_bbox;
    for (int i = 0; i < 8; ++i) {
        Eigen::Vector3d corner(
            (i & 1) ? 10.0 : -10.0,
            (i & 2) ? 10.0 : -10.0,
            (i & 4) ? 20.0 : 0.0
        );
        rotated_bbox.extend(rot * corner);
    }

    auto schedule = ZSchedule::compute(sp, params, rotated_bbox);
    
    REQUIRE(!schedule.empty());
    
    // All print_z are positive
    for (const auto &layer : schedule)
        REQUIRE(layer.print_z > 0.0);

    // Strictly ascending
    for (size_t i = 1; i < schedule.size(); ++i)
        REQUIRE(schedule[i].print_z > schedule[i-1].print_z);

    // All heights positive
    for (const auto &layer : schedule)
        REQUIRE(layer.height > 0.0);
}

TEST_CASE("AngledSlicing::ZSchedule - has first layers", "[AngledSlicing]") {
    SlicingParameters sp;
    sp.valid = true;
    sp.layer_height = 0.2;
    sp.first_print_layer_height = 0.2;
    sp.min_layer_height = 0.05;

    DynamicPrintConfig cfg = DynamicPrintConfig::full_print_config();
    cfg.set_deserialize_strict("angled_slicing_angle", "30");
    cfg.set_deserialize_strict("angled_slicing_direction", "0");
    PrintObjectConfig obj_cfg;
    obj_cfg.apply(cfg, true);
    auto params = Params::from_config(obj_cfg);

    auto rot = AngledTransform::build_rotation(params);
    Eigen::AlignedBox<double, 3> rotated_bbox;
    for (int i = 0; i < 8; ++i) {
        Eigen::Vector3d corner(
            (i & 1) ? 10.0 : -10.0,
            (i & 2) ? 10.0 : -10.0,
            (i & 4) ? 20.0 : 0.0
        );
        rotated_bbox.extend(rot * corner);
    }

    auto schedule = ZSchedule::compute(sp, params, rotated_bbox);
    
    // With a 30° tilt on a 20mm wide object, multiple layers should be first layers
    size_t first_count = FirstLayer::count_first_layers(schedule);
    REQUIRE(first_count >= 1);
    
    // First element should always be a first layer
    REQUIRE(schedule.front().is_first_layer);
}
