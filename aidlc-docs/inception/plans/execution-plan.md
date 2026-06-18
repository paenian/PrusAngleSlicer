# Execution Plan — Angled Slicing Feature

## Detailed Analysis Summary

### Transformation Scope
- **Transformation Type**: New feature integrated into existing pipeline
- **Primary Changes**: Slicing plane orientation (posSlice), new config parameters, preview adaptation
- **Related Components**: TriangleMeshSlicer, PrintObject::slice(), PrintConfig, Layer, GCode preview rendering

### Change Impact Assessment
- **User-facing changes**: Yes — new Print Settings section, preview shows tilted layers
- **Structural changes**: Yes — new slicing geometry computation, new pipeline behavior at posSlice
- **Data model changes**: Yes — new config options (angle, direction), Layer may need additional metadata
- **API changes**: No — G-code output format unchanged, no new APIs
- **NFR impact**: Yes — performance (more layers at steep angles), geometry accuracy

### Component Relationships
```
PrintConfig (new params) ←── PrintObject::slice() (angled plane computation)
                                    │
                                    ├── TriangleMeshSlicer (intersection with tilted planes)
                                    ├── Layer (stores tilted slice results)
                                    │     ├── PerimeterGenerator (operates normally on 2D slices)
                                    │     ├── Fill/* (operates normally on 2D regions)
                                    │     └── Support/* (overhang angle relative to tilt)
                                    │
                                    └── GCode/Preview (layer visualization with tilt metadata)
```

### Risk Assessment
- **Risk Level**: Medium-High (fundamentally changes slicing geometry; affects all downstream)
- **Rollback Complexity**: Easy (config-gated, angle=0 is identity)
- **Testing Complexity**: Moderate (geometric correctness + visual validation)

---

## Workflow Visualization

```mermaid
flowchart TD
    Start(["User Request:<br/>Angled Slicing Feature"])

    subgraph INCEPTION["INCEPTION PHASE"]
        WD["Workspace Detection<br/>COMPLETED"]
        RE["Reverse Engineering<br/>COMPLETED"]
        RA["Requirements Analysis<br/>COMPLETED"]
        US["User Stories<br/>COMPLETED"]
        WP["Workflow Planning<br/>COMPLETED"]
        AD["Application Design<br/>EXECUTE"]
        UG["Units Generation<br/>EXECUTE"]
    end

    subgraph CONSTRUCTION["CONSTRUCTION PHASE"]
        FD["Functional Design<br/>EXECUTE"]
        NFRA["NFR Requirements<br/>SKIP"]
        NFRD["NFR Design<br/>SKIP"]
        ID["Infrastructure Design<br/>SKIP"]
        CG["Code Generation<br/>EXECUTE"]
        BT["Build and Test<br/>EXECUTE"]
    end

    subgraph OPERATIONS["OPERATIONS PHASE"]
        OPS["Operations<br/>PLACEHOLDER"]
    end

    Start --> WD
    WD --> RE
    RE --> RA
    RA --> US
    US --> WP
    WP --> AD
    AD --> UG
    UG --> FD
    FD --> CG
    CG --> BT
    BT --> OPS
    OPS --> End(["Complete"])

    style WD fill:#4CAF50,stroke:#1B5E20,stroke-width:3px,color:#fff
    style RE fill:#4CAF50,stroke:#1B5E20,stroke-width:3px,color:#fff
    style RA fill:#4CAF50,stroke:#1B5E20,stroke-width:3px,color:#fff
    style US fill:#4CAF50,stroke:#1B5E20,stroke-width:3px,color:#fff
    style WP fill:#4CAF50,stroke:#1B5E20,stroke-width:3px,color:#fff
    style AD fill:#FFA726,stroke:#E65100,stroke-width:3px,stroke-dasharray: 5 5,color:#000
    style UG fill:#FFA726,stroke:#E65100,stroke-width:3px,stroke-dasharray: 5 5,color:#000
    style FD fill:#FFA726,stroke:#E65100,stroke-width:3px,stroke-dasharray: 5 5,color:#000
    style NFRA fill:#BDBDBD,stroke:#424242,stroke-width:2px,stroke-dasharray: 5 5,color:#000
    style NFRD fill:#BDBDBD,stroke:#424242,stroke-width:2px,stroke-dasharray: 5 5,color:#000
    style ID fill:#BDBDBD,stroke:#424242,stroke-width:2px,stroke-dasharray: 5 5,color:#000
    style CG fill:#4CAF50,stroke:#1B5E20,stroke-width:3px,color:#fff
    style BT fill:#4CAF50,stroke:#1B5E20,stroke-width:3px,color:#fff
    style OPS fill:#BDBDBD,stroke:#424242,stroke-width:2px,stroke-dasharray: 5 5,color:#000
    style Start fill:#CE93D8,stroke:#6A1B9A,stroke-width:3px,color:#000
    style End fill:#CE93D8,stroke:#6A1B9A,stroke-width:3px,color:#000

    linkStyle default stroke:#333,stroke-width:2px
```

---

## Phases to Execute

### INCEPTION PHASE
- [x] Workspace Detection (COMPLETED)
- [x] Reverse Engineering (COMPLETED)
- [x] Requirements Analysis (COMPLETED)
- [x] User Stories (COMPLETED)
- [x] Workflow Planning (COMPLETED)
- [ ] Application Design — **EXECUTE**
  - **Rationale**: New algorithm needs component design. Must define: angled plane computation module, its interface to TriangleMeshSlicer, how Layer stores tilt metadata, config parameter definitions, and preview rendering adaptation. Multiple new components needed.
- [ ] Units Generation — **EXECUTE**
  - **Rationale**: Feature decomposes into multiple units: (1) Config/parameters, (2) Core angled slicing algorithm, (3) Preview adaptation, (4) First-layer handling. Each unit has distinct dependencies and can be developed incrementally.

### CONSTRUCTION PHASE (per-unit loop)
- [ ] Functional Design — **EXECUTE**
  - **Rationale**: Complex geometric algorithm needs detailed design before coding. Must specify: tilted plane equation, mesh intersection approach, Z-clipping logic, layer height distribution, and first-layer detection. PBT properties must be identified here.
- [ ] NFR Requirements — **SKIP**
  - **Rationale**: User opted out of resiliency baseline. Performance is tracked as a requirement but doesn't need a separate NFR stage. The 2x performance budget is already in requirements.md.
- [ ] NFR Design — **SKIP**
  - **Rationale**: NFR Requirements skipped → NFR Design skipped.
- [ ] Infrastructure Design — **SKIP**
  - **Rationale**: Desktop C++ application. No cloud infrastructure, no deployment model changes. Build system changes are trivial (add new source files to CMakeLists.txt).
- [ ] Code Generation — **EXECUTE** (ALWAYS)
  - **Rationale**: Implementation of all designed components.
- [ ] Build and Test — **EXECUTE** (ALWAYS)
  - **Rationale**: Compile, run unit tests, validate G-code output.

### OPERATIONS PHASE
- [ ] Operations — **PLACEHOLDER** (future)

---

## Unit Decomposition (Preview)

Based on the requirements and stories, the feature will be decomposed into these units during Units Generation:

| Unit | Description | MVP | Dependencies |
|------|-------------|-----|--------------|
| **Unit 1: Config & Parameters** | New PrintConfig options (angle, direction), serialization, UI binding | Yes | None |
| **Unit 2: Angled Slice Engine** | Tilted plane computation, mesh intersection, Z-clipping, multi-first-layer detection | Yes | Unit 1 |
| **Unit 3: Preview Adaptation** | Layer visualization with tilt, layer slider behavior | Yes | Unit 2 |
| **Unit 4: Support Compatibility** | Overhang detection relative to tilted planes, brim at Z=0 | Post-MVP | Unit 2 |

---

## Estimated Timeline (Active Development Time)

| Stage | Estimate |
|-------|----------|
| Application Design | 1 session |
| Units Generation | 1 session |
| Functional Design (per unit) | 1 session per unit |
| Code Generation (per unit) | 2-4 sessions per unit |
| Build and Test | 1-2 sessions |
| **Total** | **~12-18 sessions** |

---

## Success Criteria

- **Primary Goal**: Slice a Benchy at a non-zero angle and produce valid G-code with visibly tilted layer lines
- **Key Deliverables**: Working angled slicing engine, config UI, basic preview, exportable G-code
- **Quality Gates**:
  - All existing PrusaSlicer unit tests pass
  - Angle=0 produces bit-for-bit identical output
  - PBT tests pass for geometric transformation functions
  - G-code loads in a viewer without errors
