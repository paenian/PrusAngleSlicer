# Components — Angled Slicing Feature

## Overview

The angled slicing feature is implemented as a self-contained module in `src/libslic3r/Feature/AngledSlicing/` with minimal registration hooks into three existing files. All downstream pipeline stages (perimeters, infill, supports, G-code) operate unchanged on the 2D slice results.

---

## New Components

### Component 1: AngledSlicingParams
**File**: `src/libslic3r/Feature/AngledSlicing/AngledSlicingParams.hpp`  
**Responsibility**: Immutable value object holding the user-configured angled slicing parameters. Passed through the pipeline wherever slicing decisions are made.

| Property | Type | Description |
|----------|------|-------------|
| `enabled` | `bool` | True if angle != 0.0 |
| `tilt_angle_deg` | `double` | Tilt angle in degrees (0–89) |
| `tilt_direction_deg` | `double` | Azimuth direction in degrees (0–360, 0=+X) |
| `tilt_angle_rad` | `double` | Derived: tilt_angle_deg * π/180 |
| `tilt_direction_rad` | `double` | Derived: tilt_direction_deg * π/180 |

---

### Component 2: AngledSlicingTransform
**File**: `src/libslic3r/Feature/AngledSlicing/AngledSlicingTransform.hpp/cpp`  
**Responsibility**: Pure geometric computation. Converts angled slicing parameters into the rotation transform that is composed with the existing mesh transform in `MeshSlicingParams`. Also computes the Z-heights of tilted layer planes and determines which layers intersect the build plate.

This is the core algorithmic component. All methods are pure functions (no side effects), making them straightforward to property-test.

Key insight: `MeshSlicingParams` already contains a `Transform3d trafo` field. By composing a tilt rotation into this transform, the existing `slice_mesh_ex()` infrastructure slices the mesh at the same horizontal Z-heights — but the mesh has been rotated, so the cut planes are effectively tilted relative to the original mesh. No changes to `TriangleMeshSlicer` are needed.

---

### Component 3: AngledSlicingZSchedule
**File**: `src/libslic3r/Feature/AngledSlicing/AngledSlicingZSchedule.hpp/cpp`  
**Responsibility**: Computes the sequence of Z-heights (print_z, slice_z, height) for tilted layers. Handles the expanded Z range (`height / cos(angle)`), identifies which layers intersect Z=0 (first-layer candidates), and clips layer regions below Z=0.

---

### Component 4: AngledSlicingFirstLayer
**File**: `src/libslic3r/Feature/AngledSlicing/AngledSlicingFirstLayer.hpp/cpp`  
**Responsibility**: Determines first-layer membership for layers that intersect the build plate. Multiple layers can be "first layers" simultaneously. Provides logic to tag layers with first-layer settings for flow, speed, and temperature.

---

## Existing Components Modified (Minimally)

### PrintObjectConfig (existing, minimal change)
**File**: `src/libslic3r/PrintConfig.hpp` / `src/libslic3r/PrintConfig.cpp`  
**Change**: Add two new `ConfigOptionFloat` fields: `angled_slicing_angle` and `angled_slicing_direction`. Add corresponding `PrintConfigDef` registrations and invalidation rules pointing to `posSlice`.  
**Impact**: ~25 lines added, no existing lines changed.

### PrintObject::slice_volumes() (existing, minimal change)
**File**: `src/libslic3r/PrintObject.cpp`  
**Change**: After computing the standard mesh transform, check if angled slicing is enabled. If so, compose `AngledSlicingTransform::build_rotation_transform(params)` into `MeshSlicingParams::trafo`. Replace the standard Z-heights vector with `AngledSlicingZSchedule::compute_z_heights(...)`.  
**Impact**: ~15 lines added in one conditional block, no existing logic altered.

### GCodeProcessor / Preview (existing, minor change)
**File**: `src/libslic3r/GCode/GCodeProcessor.cpp` or relevant preview file  
**Change**: Layers already carry `print_z` and `height`. Preview rendering uses these values. No structural change needed — tilted layers naturally produce visually diagonal boundaries because consecutive layers have overlapping XY footprints at different Z values. If an explicit tilt indicator is desired in the viewport, a small addition to the layer metadata may be needed (post-MVP).  
**Impact**: Post-MVP, minimal.
