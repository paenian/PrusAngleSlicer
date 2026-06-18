# Unit of Work Dependencies — Angled Slicing Feature

## Dependency Matrix

| Unit | U1 Config | U2 Engine | U3 Preview | U4 Support | U5 Multi-Mat |
|------|:---------:|:---------:|:----------:|:----------:|:------------:|
| **U1 Config** | — | needs | needs | needs | needs |
| **U2 Engine** | required | — | needs | needs | needs |
| **U3 Preview** | required | required | — | — | — |
| **U4 Support** | required | required | — | — | needs |
| **U5 Multi-Mat** | required | required | — | required | — |

Legend: `required` = hard dependency (must be done first), `needs` = consumes output, `—` = no dependency

## Build Order

```
Phase 1 — MVP (sequential, each depends on prior)
┌──────────────────────────────────────────────┐
│  U1: Config & Parameters                      │  Week 1
│      └─► provides: Params, config options     │
├──────────────────────────────────────────────┤
│  U2: Angled Slice Engine                      │  Week 2-3
│      └─► provides: working tilted layers      │
├──────────────────────────────────────────────┤
│  U3: Preview Adaptation                       │  Week 3
│      └─► provides: visual verification        │
└──────────────────────────────────────────────┘

Phase 2 — Post-MVP (can be parallelized after U2)
┌──────────────────────────────────────────────┐
│  U4: Support & Edge Cases  (after U2)         │
└──────────────────────────────────────────────┘
          │
          ▼
┌──────────────────────────────────────────────┐
│  U5: Multi-Material  (after U2 + U4)          │
└──────────────────────────────────────────────┘
```

## Dependency Details

### U1 → U2
- U2 reads `AngledSlicing::Params` (defined in U1)
- U2 reads `PrintObjectConfig::angled_slicing_angle/direction` (added in U1)
- U2 uses the invalidation hook skeleton added in U1

### U2 → U3
- U3 consumes `Layer::print_z` and `Layer::height` populated correctly by U2
- U3 validates that the visual output of U2 is correct
- No new data types — all data flows through existing `Layer` struct

### U2 → U4
- U4 verifies support generation on layers produced by U2
- U4 adds validation logic that checks U2 config params (angle range, height overflow)
- U4 brim/skirt handling depends on correct multi-first-layer output from U2

### U2 + U4 → U5
- U5 depends on multi-first-layer handling (U2) and validation infrastructure (U4)
- U5 verifies wipe tower integration with tilted layer Z heights from U2

## Shared Resources / Integration Points

| Resource | Owner | Consumers |
|----------|-------|-----------|
| `AngledSlicing::Params` | U1 | U2, U3, U4, U5 |
| `AngledSlicingTransform` | U2 | U3 (read-only), U4 (read-only) |
| `AngledSlicingZSchedule` | U2 | U4 (reads for height checks) |
| `Layer::print_z / height` | Existing (populated by U2) | U3, U4, U5 |
| `PrintConfig` options | U1 | All units |
| `tests/libslic3r/test_angled_slicing.cpp` | U2 (creates) | U4, U5 (extend) |

## Risk Dependencies

| Risk | Affected Units | Mitigation |
|------|---------------|------------|
| Elephant foot compensation interacts with multi-first-layer | U2, U4 | Investigate in U2 Functional Design; disable if needed |
| Variable layer height + angled slicing | U1, U4 | Add mutual exclusion validator in U1/U4 |
| TBB parallel slicing thread safety | U2 | ZSchedule computed once before parallel loop, passed const |
| Preview rendering doesn't automatically show tilt | U3 | May need explicit tilt metadata in Layer for preview |
