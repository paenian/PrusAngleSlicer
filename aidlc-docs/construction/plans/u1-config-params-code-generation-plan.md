# Code Generation Plan — U1: Config & Parameters

## Unit Context
- **Stories**: US-2 (Angle Configuration UI), US-4 (Backward Compatibility)
- **Dependencies**: None (this is the foundation unit)
- **Consumers**: U2, U3, U4, U5 (all downstream units read Params)

## Code Location
- **New files**: `src/libslic3r/Feature/AngledSlicing/AngledSlicingParams.hpp`
- **Modified files**: `src/libslic3r/PrintConfig.hpp`, `src/libslic3r/PrintConfig.cpp`, `src/libslic3r/PrintObject.cpp`, `src/libslic3r/CMakeLists.txt`
- **Test file**: `tests/libslic3r/test_angled_slicing.cpp`

## Generation Steps

- [x] Step 1: Create `AngledSlicingParams.hpp` — the Params value object
- [x] Step 2: Modify `PrintConfig.hpp` — add two ConfigOptionFloat fields to PrintObjectConfig
- [x] Step 3: Modify `PrintConfig.cpp` — register options in PrintConfigDef
- [x] Step 4: Modify `PrintObject.cpp` — add invalidation hook for the new config keys
- [x] Step 5: Modify `CMakeLists.txt` — add new file to build
- [x] Step 6: Create unit test `test_angled_slicing.cpp` — validate Params construction and round-trip

## Story Traceability
| Step | Story | Acceptance Criteria Covered |
|------|-------|---------------------------|
| 1-3 | US-2 Sc1,Sc2 | Angle/direction params available in config |
| 4 | US-4 Sc1 | Config change triggers posSlice invalidation |
| 1-3 | US-4 Sc2 | Missing keys default to 0.0 on load |
| 6 | US-4 Sc3 | Tests validate backward-compatible behavior |
