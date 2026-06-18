# Requirements Clarification Questions

Please answer each question by filling in the letter choice after the `[Answer]:` tag.
If none of the options fit, choose the last option (Other/X) and describe your answer after the tag.
Let me know when you are done and I will analyze your answers and continue.

---

## SECTION 1: Feature Definition

## Question 1
What is the "new core feature" you want to add to PrusaSlicer? Based on the project name "PrusAngleSlicer," is this feature related to printing at angles (non-horizontal layer lines, angled slicing, or tilted printing)?

A) Yes — angle-based slicing: slice the model at a non-horizontal angle (tilted print plane) to improve strength or surface quality along a specific axis

B) Yes — overhang angle optimization: automatically orient or rotate the model to minimize overhangs based on angle analysis

C) Yes — per-layer rotation/tilt of the print head or bed (for printers with a tilt axis, like belt printers or robotic arms)

D) Yes — angled infill or perimeters: generate toolpaths at specific angles relative to layer or model geometry

E) Something else entirely — please describe after [Answer]:

[Answer]: A and B

The idea is to print at an angle that can be entered by the user.  The parts will be printed on a pure cartesian machine, so gcode output doesn't change.  However, instead of one first layer there will be multiple, without changing the shape of the input parts.

---

## Question 2
How would a user interact with this new feature?

A) A new print setting/parameter in the existing Print Settings panel (like a slider or numeric input)

B) A new gizmo or tool in the 3D viewport (like the existing seam painting or support painting tools)

C) Fully automatic — the feature runs during slicing with no user interaction beyond enabling it

D) A combination of a config parameter to enable it plus automatic behavior during slicing

E) Other (please describe after [Answer]:)

[Answer]: A

---

## Question 3
Which printer technology does this feature target?

A) FFF/FDM only (filament printers — MK4, XL, MINI, etc.)

B) SLA/resin only (Prusa SL1, SL1S, SL2)

C) Both FFF and SLA

D) Other (please describe after [Answer]:)

[Answer]: A

---

## SECTION 2: Scope and Constraints

## Question 4
You said "modifying the existing code as little as possible." What does that mean in practice for this feature?

A) Strictly additive — new files only, zero changes to existing source files

B) Mostly additive — new files, plus minimal changes to existing registration points (e.g., adding an enum value, registering a new class in a factory)

C) New pipeline step — add a new `PrintObjectStep` (like `posInfill` or `posPerimeters`) with its own implementation file, with small hooks in existing orchestration

D) Modify existing pipeline step(s) — extend `make_perimeters()`, `infill()`, or another existing step with conditional new behavior

E) Other (please describe after [Answer]:)

[Answer]: B

I haven't reviewed the PrusaSlicer code enough to answer this properly.  However, I do want to ensure that all existing features (like support material generation) work with the angled slicing feature.  A big consideration is that eventually I would like to make a pull request to integrate this feature.

---

## Question 5
What is the expected scope of changes?

A) Single new file + minimal registration changes (< 5 files touched total)

B) A small cluster of new files + a handful of existing file changes (5–15 files)

C) A new feature module similar to `Feature/FuzzySkin/` or `Feature/Interlocking/` (self-contained directory, ~10-20 new files, minimal existing changes)

D) Larger scope — new UI panel, new pipeline step, new config options, and new algorithm (20+ files)

E) Other (please describe after [Answer]:)

[Answer]: D

It would also require a configuration tab in the Print settings, and some UI integration to show the angled slices in the preview pane

---

## Question 6
Do you have an existing algorithm, paper, or reference implementation you want to follow for this feature?

A) Yes — I have a specific algorithm/paper in mind (please describe after [Answer]:)

B) Yes — there is a reference implementation in another slicer (please name it after [Answer]:)

C) No — design the algorithm from first principles based on the feature description

D) Other (please describe after [Answer]:)

[Answer]: C

I have seen this implemented manually by skewing the model, slicing, and then de-skewing it back to the build plate... but that approach is klunky and breaks slicer features like support material and preview.  We can do better, and integrate it properly.

---

## SECTION 3: Technical Constraints

## Question 7
Where in the slicing pipeline should this feature primarily integrate?

A) During geometry/slicing (posSlice) — change how the mesh is sliced into layers

B) During perimeter generation (posPerimeters) — change how perimeter loops are generated

C) During infill generation (posInfill) — change how fill patterns are created

D) During G-code generation (GCodeGenerator) — change how toolpaths are translated to G-code

E) New independent pipeline step (between existing steps)

F) Other (please describe after [Answer]:)

[Answer]: A

All layers need to be angled, so each slice will have its perimeters and infill as usual, just at an angle.  You should be able to print the same part twice, at two different angles, and they would have all their plastic and supports at the same spot, but the layer lines wouldn't match.

---

## Question 8
Should the feature be compatible with both the Classic and Arachne perimeter generators?

A) Yes — must work with both

B) Classic perimeter generator only

C) Arachne perimeter generator only

D) Not relevant to perimeter generation

E) Other (please describe after [Answer]:)

[Answer]:  A

---

## Question 9
Does this feature require changes to the G-code output format or introduce new G-code commands?

A) No — existing G-code moves (G0/G1) are sufficient; only the toolpath geometry changes

B) Yes — needs additional G-code commands (e.g., rotary axis commands, tilting commands)

C) Maybe — depends on the final design

D) Other (please describe after [Answer]:)

[Answer]: A

This will print on an un-modified printer - not a belt printer or anything like that.

---

## SECTION 4: Quality and Testing

## Question 10
What does "done" look like for a first working version (MVP)?

A) Feature compiles and produces visibly different G-code output that can be verified by inspection

B) Feature passes a unit test demonstrating the core algorithm produces correct output

C) Feature is usable in the GUI end-to-end: enable it, slice a test model, preview the result, export G-code

D) Feature produces measurably improved print quality on a physical test print

E) Other (please describe after [Answer]:)

[Answer]: A

---

## Question 11
Are there specific test models or scenarios you want the feature validated against?

A) A simple geometric shape (cube, cylinder, sphere) is sufficient for initial validation

B) A specific challenging geometry (overhang, thin walls, bridging span) — please describe after [Answer]:

C) A real-world part from your use case — please describe after [Answer]:

D) No specific model preference; use whatever is standard in the existing test suite

E) Other (please describe after [Answer]:)

[Answer]: C

Validate with a benchy in single color, and a cat with chevrons on it for multicolor.

---

## SECTION 5: Extensions

## Question 12 — Security Extension
Should security extension rules be enforced for this project?

(Note: This is a desktop C++ application. Security concerns are lower than for web services, but still relevant for file parsing, network upload, and input validation.)

A) Yes — enforce all SECURITY rules as blocking constraints (recommended if you plan to add network features or file format changes)

B) No — skip all SECURITY rules (suitable for a local algorithmic feature with no network or new file format impact)

X) Other (please describe after [Answer]:)

[Answer]: B
Can't fix the rest of the project

---

## Question 13 — Resiliency Extension
Should the resiliency baseline be applied to this project?

(Note: For a desktop slicer, this primarily applies to error handling robustness, graceful degradation when the algorithm encounters degenerate geometry, and crash prevention.)

A) Yes — apply the resiliency baseline as directional best practices (recommended if the feature might encounter unusual mesh geometry or edge cases)

B) No — skip the resiliency baseline (suitable if this is a focused algorithmic prototype where error handling can be added later)

X) Other (please describe after [Answer]:)

[Answer]: B

---

## Question 14 — Property-Based Testing Extension
Should property-based testing (PBT) rules be enforced for this project?

(Note: PBT is well-suited for geometric/algorithmic code that transforms polygons, computes angles, or applies mathematical operations — which is exactly what a slicing feature does.)

A) Yes — enforce all PBT rules as blocking constraints (recommended: slicing algorithms have rich invariants that PBT can catch)

B) Partial — enforce PBT rules only for pure functions and geometric transformations

C) No — skip all PBT rules (standard unit tests are sufficient for now)

X) Other (please describe after [Answer]:)

[Answer]: B
Only enforce for new code.

---

*When finished, let me know and I will analyze your answers and generate the full requirements document.*
