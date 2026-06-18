# Requirements — Angled Slicing Feature for PrusaSlicer

## Intent Analysis

| Attribute | Value |
|-----------|-------|
| **User Request** | Add angled (non-horizontal) slicing to PrusaSlicer, where the slicing plane is tilted at a user-specified angle. Printed on an unmodified Cartesian printer. |
| **Request Type** | New Feature |
| **Scope Estimate** | System-wide (new pipeline behavior, config UI, preview integration) |
| **Complexity Estimate** | Complex — fundamentally changes the slicing geometry while preserving all downstream behavior (supports, perimeters, infill, multi-color) |
| **Code Change Strategy** | Mostly additive with minimal registration changes. Designed for eventual upstream PR contribution. |

---

## Concept Overview

**Angled slicing** means the model is sliced by planes that are tilted relative to the horizontal build plate, rather than the standard horizontal planes at fixed Z intervals. The result is:

- Layer planes are no longer parallel to the XY build plane
- Instead of one "first layer" touching the bed, multiple layers intersect the bed at different Z heights
- The printed object retains its original shape — only the internal layer structure changes
- G-code output uses standard G0/G1 commands (no rotary axes or special hardware)
- All existing features (supports, infill, perimeters, multi-material) continue to work

**Key insight**: The model geometry is NOT modified. The slicing planes are rotated. This is fundamentally different from the "skew → slice → de-skew" workaround because it preserves correct model dimensions, support generation, and preview rendering.

---

## Functional Requirements

### FR-1: Angled Slicing Plane Configuration

| ID | Requirement |
|----|-------------|
| FR-1.1 | The user shall be able to specify a slicing angle (tilt angle) as a numeric parameter in degrees |
| FR-1.2 | The angle defines the rotation of the slicing plane around one or more horizontal axes |
| FR-1.3 | An angle of 0° produces standard horizontal slicing (existing behavior, fully backward-compatible) |
| FR-1.4 | Valid angle range: 0° to < 90° (exclusive). At 90° slicing would be degenerate |
| FR-1.5 | The user shall be able to specify the tilt direction (azimuth / rotation axis) to control which direction the layers lean |
| FR-1.6 | Configuration shall live in Print Settings panel alongside existing layer height settings |

### FR-2: Angled Slice Computation

| ID | Requirement |
|----|-------------|
| FR-2.1 | The slicer shall compute intersection of the model mesh with tilted planes instead of horizontal planes |
| FR-2.2 | Layer thickness (measured perpendicular to the tilted plane) shall respect the configured layer height |
| FR-2.3 | The vertical Z-spacing between successive layers at any given XY point will vary based on the tilt angle |
| FR-2.4 | Multiple layers may intersect the build plate (Z=0 plane) simultaneously — these all act as "first layers" for bed adhesion purposes |
| FR-2.5 | The effective build height increases when printing at an angle: actual print Z = model height / cos(angle) |
| FR-2.6 | Slicing must handle partial layers — regions of a slice that extend below Z=0 are clipped to the bed |

### FR-3: Downstream Feature Compatibility

| ID | Requirement |
|----|-------------|
| FR-3.1 | Perimeter generation (both Classic and Arachne) shall operate normally on angled slices |
| FR-3.2 | All infill patterns shall generate correctly within angled slice regions |
| FR-3.3 | Support material generation shall detect overhangs correctly relative to the tilted layer orientation |
| FR-3.4 | Brim and skirt shall generate on the build plate at Z=0 |
| FR-3.5 | Multi-material / multi-color printing shall function normally with angled slicing |
| FR-3.6 | Ironing, fuzzy skin, and other post-perimeter features shall apply to angled layers |
| FR-3.7 | Variable layer height painting shall interact correctly with angled slicing (TBD — may be restricted) |
| FR-3.8 | The feature must produce identical outer geometry when printed — two prints of the same model at different angles should result in the same external shape with different internal layer lines |

### FR-4: G-code Output

| ID | Requirement |
|----|-------------|
| FR-4.1 | Output G-code uses only standard G0/G1 moves — no firmware modifications required |
| FR-4.2 | Layer changes in G-code reflect the actual Z height where each tilted layer prints |
| FR-4.3 | The G-code shall be compatible with standard Cartesian FFF printers (Prusa MK4, XL, MINI, etc.) |
| FR-4.4 | Print time and filament estimates shall remain accurate |

### FR-5: User Interface

| ID | Requirement |
|----|-------------|
| FR-5.1 | A configuration section in Print Settings for angled slicing parameters (angle, direction) |
| FR-5.2 | Preview pane shall show the angled layer lines correctly (layer colors follow tilted planes) |
| FR-5.3 | Layer slider in preview shall navigate through tilted layers |
| FR-5.4 | The 3D viewport should visually indicate the slicing plane orientation when the feature is enabled |

### FR-6: Validation Models

| ID | Requirement |
|----|-------------|
| FR-6.1 | MVP shall be validated with a 3D Benchy in single-color mode |
| FR-6.2 | Multi-color validation shall use a multi-color cat model with chevron patterns |

---

## Non-Functional Requirements

### NFR-1: Performance

| ID | Requirement |
|----|-------------|
| NFR-1.1 | Slicing time with angled slicing shall not exceed 2x the standard horizontal slicing time for the same model |
| NFR-1.2 | Memory usage shall not exceed 2x the standard slicing memory for the same model |

### NFR-2: Backward Compatibility

| ID | Requirement |
|----|-------------|
| NFR-2.1 | When angle = 0°, behavior must be identical to current PrusaSlicer (bit-for-bit G-code output) |
| NFR-2.2 | Existing .3mf project files must load without errors (angle defaults to 0°) |
| NFR-2.3 | No existing unit tests shall be broken |

### NFR-3: Code Quality (PR-Readiness)

| ID | Requirement |
|----|-------------|
| NFR-3.1 | Code shall follow existing PrusaSlicer style (see .clang-format) |
| NFR-3.2 | New code shall be mostly additive — minimize changes to existing files |
| NFR-3.3 | Feature shall be behind a config flag — when disabled, zero code paths are affected |
| NFR-3.4 | New config parameters shall integrate into the existing serialization/deserialization system |
| NFR-3.5 | Feature shall work with the existing `PrintObjectStep` invalidation system |

### NFR-4: Testability

| ID | Requirement |
|----|-------------|
| NFR-4.1 | Property-based tests shall be written for new geometric transformation functions (angled plane intersection, layer clipping) |
| NFR-4.2 | Standard unit tests shall cover the main algorithm entry points |
| NFR-4.3 | Existing test suite shall continue to pass |

---

## MVP Definition

The MVP is reached when:
1. The feature compiles successfully
2. A user can set a non-zero angle in configuration
3. Slicing a model produces visibly different G-code with angled layer lines
4. The G-code is valid and can be loaded in a G-code viewer/simulator
5. A Benchy sliced at angle produces recognizable output

GUI preview integration, multi-color support, and full support material compatibility are post-MVP enhancements.

---

## Key Technical Decisions

| Decision | Rationale |
|----------|-----------|
| Integrate at `posSlice` level | Angled slicing fundamentally changes WHERE slices are taken — everything downstream (perimeters, infill, supports) operates on the resulting 2D slice geometry as normal |
| Standard G-code only | Target is unmodified Cartesian hardware — no 5-axis or belt printer support needed |
| Config-gated feature | Feature disabled by default (angle=0). When disabled, zero performance impact. Enables safe upstream contribution. |
| Design from first principles | No clean reference implementation exists. The "skew/de-skew" hack breaks too many features. Proper integration is the goal. |
| Mostly additive changes | Prioritize new files + minimal registration hooks in existing code, to facilitate clean PR upstream |

---

## Extension Configuration

| Extension | Enabled | Decided At | Rationale |
|-----------|---------|------------|-----------|
| Security Baseline | No | Requirements Analysis | Local algorithmic feature, no network or file format changes |
| Resiliency Baseline | No | Requirements Analysis | Focused prototype; error handling can be added in post-MVP |
| Property-Based Testing | Partial (new code only) | Requirements Analysis | Geometric transformations have rich invariants; PBT catches edge cases in angle math |

---

## Open Questions / Risks

| # | Question / Risk | Impact |
|---|----------------|--------|
| 1 | How does variable layer height interact with angled slicing? | May need to be disabled when angle ≠ 0 for MVP |
| 2 | How does elephant foot compensation work when multiple layers touch the bed? | May need special handling |
| 3 | Does sequential printing mode work with angled slicing? | Clearance calculations may need adjustment |
| 4 | How does the wipe tower interact with angled layers? (Tool changes at varying Z heights) | May require wipe tower to use standard horizontal layers even when object uses angled |
| 5 | Performance risk: tilted slicing may produce more layers for tall objects (Z range increases by 1/cos(angle)) | Monitor and optimize if needed |


---

## HARD CONSTRAINT (Added 2026-06-17)

### HC-1: The Object Must Not Be Moved or Rotated

The model geometry on the build plate MUST NOT be transformed (rotated, sheared, translated) at any stage of the pipeline. The object's mesh vertices must remain in their original position throughout slicing and G-code generation.

**What this means:**
- The `MeshSlicingParams::trafo` must NOT include any angled-slicing rotation or shear
- No post-processing of slice contours (inverse rotation) is acceptable
- The model on the plate view must match the model in the preview exactly
- The approach of "rotate mesh → slice → inverse-rotate contours" is rejected

**What must change instead:**
- The `TriangleMeshSlicer` (or a new slicing function) must be extended to intersect the mesh with **arbitrary tilted planes** directly
- Each tilted plane intersection produces a 2D contour in the plane's own coordinate system
- These contours are then projected onto the XY build plate for G-code generation
- Z varies within each layer (the nozzle follows the tilted plane surface)

**Rationale:**
- Any object transformation breaks downstream features (supports, bridging, elephant foot)
- The preview must show the correct geometry at all times
- Inverse transforms introduce rounding errors and coordinate system confusion
- The "skew/rotate then de-skew" approach was the original workaround we're trying to improve upon
