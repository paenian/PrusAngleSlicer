# User Personas — Angled Slicing Feature

## Persona 1: Alex — The Functional Parts Hobbyist

| Attribute | Description |
|-----------|-------------|
| **Role** | Advanced hobbyist / maker |
| **Experience** | 2+ years of FDM printing, comfortable with slicer settings |
| **Goal** | Print parts with layer lines oriented for maximum strength in a specific direction |
| **Motivation** | FDM parts are weak along layer boundaries; aligning layers to expected stress directions improves durability |
| **Tech Comfort** | Understands print settings, layers, infill, supports. Can read G-code basics. |
| **Frustration** | Current workaround (skew model → slice → de-skew) breaks supports and preview |
| **Key Stories** | US-1, US-2, US-3, US-4, US-5 |

---

## Persona 2: Sam — The Professional Engineer

| Attribute | Description |
|-----------|-------------|
| **Role** | Mechanical/manufacturing engineer using FDM for prototypes and jigs |
| **Experience** | Deep CAD knowledge, uses multiple slicers, prints functional production parts |
| **Goal** | Precisely control layer orientation for predictable mechanical properties (anisotropy management) |
| **Motivation** | Needs to validate that angled slicing produces geometrically correct output before committing to production runs |
| **Tech Comfort** | Can inspect G-code, understands coordinate systems, knows material properties |
| **Frustration** | No slicer offers proper non-planar layer orientation without hacky workarounds |
| **Key Stories** | US-1, US-2, US-3, US-4, US-6, US-7 |

---

## Persona 3: Jordan — The Multi-Color Creative

| Attribute | Description |
|-----------|-------------|
| **Role** | Designer printing decorative and multi-color objects (MMU user) |
| **Experience** | Comfortable with PrusaSlicer's multi-material workflow |
| **Goal** | Use angled slicing for aesthetic effects — diagonal layer lines as a visible design element |
| **Motivation** | Layer lines are typically an unwanted artifact; angled slicing turns them into a deliberate visual feature |
| **Tech Comfort** | Uses GUI confidently, less interested in G-code internals |
| **Frustration** | Multi-color models lose registration if workarounds deform geometry |
| **Key Stories** | US-1, US-3, US-5, US-6 |

---

## Persona 3: Paul — The Ancient One

| Attribute | Description |
|-----------|-------------|
| **Role** | Printer pioneer building crazy new styles of 3d printer |
| **Experience** | Used to date SkeinForge |
| **Goal** | Use angled slicing to improve 3d printing in general - in particular machines that can print infinitely large objects or a series of parts without user intervention |
| **Motivation** | Printers have too much down time waiting for users to remove parts, and require too much monitoring |
| **Tech Comfort** | used to write gcode with a magnetic needle |
| **Frustration** | The slicer takes a while to accommodate printer features |
| **Key Stories** | US-1, US-2, US-3, US-4, US-5, US-6, US-7 |

---

## Persona-to-Story Matrix

| Story | Alex (Hobbyist) | Sam (Engineer) | Jordan (Creative) | Paul (Ancient) |
|-------|:---:|:---:|:---:|:---:|
| US-1: Core Angled Slicing | x | x | x | x |
| US-2: Angle Configuration | x | x | | x |
| US-3: Preview Visualization | x | x | x | x |
| US-4: Backward Compatibility | x | x | | x |
| US-5: Support Material Compat | x | | x | x |
| US-6: Multi-Material Compat | | x | x | x |
| US-7: Edge Case Handling | | x | | x |
