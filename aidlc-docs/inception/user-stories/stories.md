# User Stories — Angled Slicing Feature

## Story Overview

| ID | Title | Priority | MVP |
|----|-------|----------|-----|
| US-1 | Core Angled Slicing | Critical | Yes |
| US-2 | Angle Configuration UI | Critical | Yes |
| US-3 | Preview Visualization | High | Yes |
| US-4 | Backward Compatibility | Critical | Yes |
| US-5 | Support Material Compatibility | High | Post-MVP |
| US-6 | Multi-Material Compatibility | Medium | Post-MVP |
| US-7 | Edge Case & Error Handling | Medium | Post-MVP |

---

## US-1: Core Angled Slicing

**As a** 3D printing user,  
**I want to** slice my model with tilted layer planes at a user-specified angle and direction,  
**so that** the printed part has layer lines oriented for improved strength or aesthetics without modifying the model geometry.

### Acceptance Criteria

**Scenario 1: Basic angled slice**
```
Given a model loaded on the build plate
And the slicing angle is set to 30° with direction 0° (toward +X)
When I slice the model
Then the resulting layers are computed as intersections of the mesh with planes tilted 30° from horizontal
And the layer thickness measured perpendicular to the tilted plane equals the configured layer height
And the G-code contains only standard G0/G1 moves
```

**Scenario 2: Multiple first layers**
```
Given a model loaded on the build plate
And the slicing angle is set to 15° with direction 90° (toward +Y)
When I slice the model
Then multiple layers intersect the build plate (Z=0)
And each part of a layer that touches Z=0 is treated with first-layer settings (speed, temperature, bed adhesion) but runs at full speed when the angle takes it away from the bed.
And there are no layer regions extending below Z=0 because the model is unchanged
```

**Scenario 3: Geometry preservation**
```
Given a model sliced at angle 0° (standard)
And the same model sliced at angle 20°
When I compare the outer envelope of both G-code outputs
Then the printed external geometry is identical within tolerance (±0.1mm)
And only the layer line orientation differs
```

**Scenario 4: Standard output format**
```
Given any non-zero slicing angle
When I export G-code
Then the output file contains only standard G0/G1/G28/M-code commands
And the file is compatible with any Marlin/Klipper firmware without modification
```

---

## US-2: Angle Configuration UI

**As a** user configuring print settings,  
**I want to** specify the slicing angle and tilt direction in the Print Settings panel,  
**so that** I can control the layer orientation without editing config files manually.

### Acceptance Criteria

**Scenario 1: Angle parameter**
```
Given I am in the Print Settings tab
When I navigate to the Layer height / Angled Slicing section
Then I see a numeric input for "Slicing Angle" in degrees
And the valid range is 0° to 89° (0 = standard horizontal slicing)
And the default value is 0°
```

**Scenario 2: Direction parameter**
```
Given I am in the Angled Slicing section
When I view the direction control
Then I see a numeric input for "Tilt Direction" in degrees (0-360° azimuth)
And 0° means the layers tilt toward the +X axis (right side of bed)
And 90° means the layers tilt toward the +Y axis (back of bed)
And a visual indicator shows which direction the layers will lean
```

**Scenario 3: Config persistence**
```
Given I have set slicing angle to 25° and direction to 45°
When I save the print profile
And I reload PrusaSlicer
Then the angle and direction values are restored from the saved profile
```

**Scenario 4: 3MF project roundtrip**
```
Given I have configured angled slicing parameters
When I save a .3mf project file
And I open it in a fresh PrusaSlicer instance
Then the angled slicing parameters are restored correctly
```

---

## US-3: Preview Visualization

**As a** user who has sliced a model with angled layers,  
**I want to** see the tilted layer lines in the preview pane,  
**so that** I can verify the slicing angle is correct and identify issues before printing.

### Acceptance Criteria

**Scenario 1: Layer coloring follows tilt**
```
Given a model has been sliced with a 20° angle
When I view the preview pane
Then each colored layer band follows the tilted plane orientation
And layer boundaries are visually diagonal relative to the build plate
And the layer select goes by angled layer not z-height
```

**Scenario 2: Layer slider navigation**
```
Given angled slicing is active in the preview
When I move the layer slider
Then it steps through tilted layers in sequence
And the highlighted layer shows its tilted cross-section
```

**Scenario 3: Multiple bed-contact layers visible**
```
Given a model sliced at an angle that produces multiple first layers
When I view the lowest layers in preview
Then I can see multiple layers touching the build plate
And first-layer regions are visually distinguished (e.g., different color or highlight)
```

---

## US-4: Backward Compatibility

**As a** user who sometimes prints with standard horizontal slicing,  
**I want** angled slicing at 0° to produce identical results to the current PrusaSlicer behavior,  
**so that** enabling the feature with angle=0 doesn't break my existing workflows.

### Acceptance Criteria

**Scenario 1: Zero angle = standard slicing**
```
Given the slicing angle is set to 0°
When I slice a model
Then the G-code output is bit-for-bit identical to PrusaSlicer without the angled slicing feature
And no additional computation time is incurred
```

**Scenario 2: Legacy project files**
```
Given a .3mf project file saved by stock PrusaSlicer (without angled slicing parameters)
When I open it in this modified PrusaSlicer
Then the file loads without errors
And the slicing angle defaults to 0°
And slicing produces identical results to the stock version
```

**Scenario 3: No test regressions**
```
Given the existing PrusaSlicer unit test suite
When I run all tests with the angled slicing code present
Then all tests pass without modification
```

---

## US-5: Support Material Compatibility

**As a** user printing a model with overhangs at a non-zero slicing angle,  
**I want** support material to generate correctly relative to the tilted layers,  
**so that** overhangs are properly supported even when layer orientation changes.

### Acceptance Criteria

**Scenario 1: Overhang detection relative to tilted layers**
```
Given a model with a 45° overhang (relative to horizontal)
And the slicing angle is set to 20°
When I enable support material with a 45° threshold
Then supports are generated based on overhang angle relative to the tilted layer plane
And the effective overhang changes based on the slicing direction
```

**Scenario 2: Support on build plate**
```
Given a model requiring supports with angled slicing active
When supports are generated
Then support pillars extend down to Z=0 (the actual build plate)
And supports are printed with standard horizontal layers (not tilted)
Or supports follow the same tilted layer scheme as the object (design decision TBD)
```

**Scenario 3: Brim and skirt at Z=0**
```
Given angled slicing is active
When brim or skirt is enabled
Then brim/skirt is generated on the Z=0 plane
And it surrounds the footprint of the tilted object correctly
```

---

## US-6: Multi-Material Compatibility

**As a** user printing a multi-color model with angled slicing,  
**I want** color/material boundaries to be preserved correctly in the tilted layers,  
**so that** my multi-color designs (like chevron patterns on a cat) render accurately.

### Acceptance Criteria

**Scenario 1: Material boundaries preserved**
```
Given a multi-color model with painted material regions
And the slicing angle is set to 15°
When I slice the model
Then each tilted layer correctly segments material regions
And color boundaries align with the original model geometry
And no material bleeds across boundaries
```

**Scenario 2: Wipe tower interaction**
```
Given a multi-material print with angled slicing
When tool changes occur
Then the wipe tower operates at the correct Z height for each layer
And purge volumes are calculated correctly
```

**Scenario 3: Multi-color validation model**
```
Given the multi-color cat model with chevron patterns
When sliced at a 10° angle
Then the chevron pattern boundaries are preserved
And the result looks correct in preview
```

---

## US-7: Edge Case & Error Handling

**As a** user who may configure extreme or unusual angled slicing parameters,  
**I want** the slicer to handle edge cases gracefully,  
**so that** I don't get crashes, corrupted G-code, or unexpected behavior.

### Acceptance Criteria

**Scenario 1: Extreme angle warning**
```
Given the user sets the slicing angle to 80°
When I attempt to slice
Then the slicer displays a warning that extreme angles significantly increase print height and time
And slicing proceeds if the user confirms
```

**Scenario 2: Tall model + steep angle**
```
Given a tall model (200mm) with a 45° slicing angle
When I slice
Then the slicer correctly handles the increased effective height (200/cos(45°) ≈ 283mm)
And warns if the result exceeds the printer's maximum Z height
```

**Scenario 3: Thin model edge case**
```
Given a very thin model (1mm height) with a 30° slicing angle
When I slice
Then the slicer produces valid output with at least one complete layer
And does not crash or produce empty G-code
```

**Scenario 4: Angle = 0 performance**
```
Given the slicing angle is 0°
When I slice a model
Then there is zero measurable performance overhead compared to stock PrusaSlicer
And no angled slicing code paths are executed
```

---

## Story Priority & Delivery Order

### MVP (Deliver First)
1. **US-4**: Backward Compatibility — ensures we don't break anything
2. **US-2**: Angle Configuration UI — user must be able to set the angle
3. **US-1**: Core Angled Slicing — the fundamental algorithm
4. **US-3**: Preview Visualization — verify before printing

### Post-MVP (Deliver After)
5. **US-5**: Support Material Compatibility
6. **US-6**: Multi-Material Compatibility
7. **US-7**: Edge Case & Error Handling


---

## US-8: Anchor Line for Angled Slicing

**As a** user printing with angled slicing,  
**I want** an optional "Anchor Line" printed at the intersection of the first tilted layer and the bed plane,  
**so that** the first layer has proper bed adhesion without requiring a skirt or brim (which are incompatible with angled slicing).

### Description

When angled slicing is active, the first tilted layer intersects the bed (Z=0) along a line. This line is where material first contacts the build plate. The Anchor Line feature prints a thickened, wider extrusion along this intersection line to provide bed adhesion — similar to how brim works for normal slicing but adapted for the tilted geometry.

### Acceptance Criteria

**Scenario 1: Anchor line generation**
```
Given angled slicing is enabled with angle > 0
And the Anchor Line feature is enabled
When I slice the model
Then a thickened extrusion line is generated at Z=0
And the line follows the intersection of the first tilted plane with the bed (Z=0)
And the line extends the full length of the model in the direction perpendicular to the tilt
```

**Scenario 2: Wider extrusion**
```
Given the Anchor Line feature is enabled
When the anchor line is generated
Then the extrusion width of the anchor line is larger than the normal first layer width
And the extrusion width is configurable (e.g., 2x or 3x normal width)
```

**Scenario 3: Anchor line at correct position**
```
Given a 20mm cube with angled slicing at 15° direction 0° (tilt toward +X)
When the anchor line is generated
Then the anchor line is at Y=[90, 110] (full Y extent of the model)
And the anchor line X position is at the right edge of the model base (where the first plane touches Z=0)
And the anchor line Z=0 (flat on the bed)
```

**Scenario 4: Disabled by default**
```
Given angled slicing is enabled
And the Anchor Line feature is not explicitly enabled
When I slice the model
Then no anchor line is generated
```

**Scenario 5: Incompatible with skirt/brim**
```
Given the Anchor Line feature is enabled
When the user also enables skirt or brim
Then the slicer shows a warning that skirt/brim are incompatible with angled slicing
And the anchor line takes precedence
```

### Priority
Post-MVP enhancement (requires the base angled slicing to be stable first)

### Notes
- The anchor line is essentially the intersection of the plane `normal · p = d_min` with the bed plane `z = 0`
- For direction=0 (tilt toward +X), this is a line parallel to the Y axis at the rightmost X of the base
- The anchor line could optionally extend slightly beyond the model (like brim does) for better adhesion
- Future enhancement: multiple anchor lines for the first N planes that touch the bed


---

## US-9: Belt Printer Profile (Infinite Y Axis)

**As a** user with a Cartesian belt printer,  
**I want** a printer profile that correctly represents my infinite-Y build volume,  
**so that** I can slice and print parts without false "out of bounds" warnings on the Y axis.

### Description

A Cartesian belt printer has finite X and Z axes but an effectively infinite Y axis (the belt). PrusaSlicer needs a profile that:
- Defines Y as unbounded (or very large) for object placement
- Retains normal X and Z limits
- Supports standard Cartesian G-code (G0/G1 XYZ)
- Includes appropriate start/end G-code for belt initialization and part ejection

### Acceptance Criteria

**Scenario 1: Large Y objects accepted**
```
Given the belt printer profile is active
And a model is placed with Y extent > 200mm
When I prepare to slice
Then no "object out of bounds" warning is shown
And the model is accepted for slicing
```

**Scenario 2: X and Z limits enforced**
```
Given the belt printer profile is active
And a model exceeds the X dimension (e.g., 250mm on a 200mm X bed)
When I prepare to slice
Then an "object out of bounds" warning is shown for X
```

**Scenario 3: Normal rectilinear printing**
```
Given the belt printer profile is active
And angled slicing angle = 0
When I slice a model that fits within X and Z
Then standard horizontal slicing is used
And the G-code is compatible with the belt printer firmware
```

**Scenario 4: Belt advance for part ejection**
```
Given the belt printer profile is active
When a print completes (end G-code runs)
Then the belt advances to eject the part from the build area
And the printer is ready for the next print
```

### Priority
Post-MVP (requires belt printer hardware for testing)

---

## US-10: Bigger-Than-Bed Printing via Angled Slicing

**As a** user with a belt printer,  
**I want** to print parts that are longer than my X or Z build volume,  
**so that** I can produce large parts on a compact machine by tilting the slicing direction along the belt axis.

### Description

When a part exceeds the X or Z dimensions of the build volume, the slicer can automatically compute a tilt angle that maps the excess dimension onto the infinite Y axis. The part is printed diagonally — each tilted layer fits within X and Z, and the belt carries the part forward as it grows in Y.

### Acceptance Criteria

**Scenario 1: Auto-angle for oversized X**
```
Given a part is 300mm in X on a printer with 200mm X build volume
And the belt axis is Y (infinite)
When I enable "auto-fit angle" mode
Then the slicer computes the minimum angle to fit: arctan(300/200) ≈ 56°
And the angled slicing direction is set to 0° (tilt toward X, extending into Y)
And the part slices successfully within the build volume
```

**Scenario 2: Auto-angle for oversized Z**
```
Given a part is 400mm in Z on a printer with 200mm Z build volume
And the belt axis is Y
When I enable "auto-fit angle" mode  
Then the slicer computes the minimum angle to fit the Z extent
And the part is tilted so its height extends into the Y direction
```

**Scenario 3: Part fits without tilting**
```
Given a part fits within X and Z limits
When auto-fit angle mode is enabled
Then the angle remains 0° (no unnecessary tilting)
And standard horizontal slicing is used
```

**Scenario 4: Manual override**
```
Given the auto-fit angle suggests 45°
When the user manually sets a different angle
Then the manual angle is used
And a warning is shown if the part exceeds build volume at that angle
```

### Priority
Post-MVP enhancement

---

## US-11: Sequential Belt Printing (Continuous Production)

**As a** user operating a belt printer for production,  
**I want** to queue multiple parts for sequential printing with automatic belt advance between each,  
**so that** I can run continuous unattended production without manual intervention.

### Description

After each part finishes, the belt advances to eject the completed part, then the next part begins printing. This enables continuous production where parts roll off the belt one after another.

### Acceptance Criteria

**Scenario 1: Multi-part sequential print**
```
Given 5 copies of a model are queued for printing
And the belt printer profile is active
When I start the print
Then each copy is printed sequentially
And after each copy completes, the belt advances to eject it
And the next copy begins printing at Y=0 (reset position)
```

**Scenario 2: Belt advance G-code between parts**
```
Given a part has completed printing
When the between-objects sequence runs
Then G-code advances the belt by at least (part_Y_extent + clearance)
And the extruder moves to a safe position before belt advance
And the bed is clear for the next part
```

**Scenario 3: Mixed part queue**
```
Given different models are queued (e.g., 3x widget_A + 2x widget_B)
When printing sequentially
Then each model is sliced and printed independently
And belt advance distance matches each part's Y extent
```

**Scenario 4: Continuous operation with angled slicing**
```
Given angled slicing is enabled for oversized parts
And sequential belt printing is active
When each oversized part completes
Then the belt advances by the tilted part's Y footprint
And the next part starts fresh at Y=0
```

### Priority
Future enhancement (requires sequential print + belt integration)

---

## US-12: Belt Printer Part Ejection

**As a** user with a belt printer,  
**I want** reliable part ejection after each print completes,  
**so that** the build area is cleared and ready for the next print without manual intervention.

### Acceptance Criteria

**Scenario 1: Single part ejection**
```
Given a single part has finished printing
When the end G-code executes
Then the nozzle retracts and moves to a safe Z height
And the belt advances forward until the part falls off the end
And a configurable pause allows the part to cool/detach
```

**Scenario 2: Ejection distance calculation**
```
Given a part occupies Y range [0, 150mm] on the belt
When ejection is triggered
Then the belt advances at least 150mm + ejection_clearance (configurable, default 20mm)
And the advance speed is configurable (default: slower than print speed)
```

**Scenario 3: Ejection G-code**
```
Given the belt printer profile is configured
When ejection occurs
Then the G-code sequence is:
  1. Retract filament
  2. Lift Z to clearance height
  3. G1 Y{part_length + clearance} F{belt_speed} ; advance belt
  4. G92 Y0 ; reset Y origin for next part
  5. Optional: pause for cooling
```

### Priority
Post-MVP (part of belt printer profile)
