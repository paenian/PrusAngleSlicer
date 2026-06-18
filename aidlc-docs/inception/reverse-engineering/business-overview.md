# Business Overview — PrusaSlicer

## What Is It?

PrusaSlicer is an open-source 3D printing preparation application (a "slicer"). It converts 3D models (STL, OBJ, 3MF, AMF, STEP, etc.) into machine-readable G-code files that 3D printers execute to physically reproduce the model layer by layer.

## Primary User Workflows (Business Transactions)

### 1. FFF (Fused Filament Fabrication) Print Preparation
The main workflow for desktop FDM printers (Prusa MK4, XL, MINI, etc.):
1. **Import** — User loads one or more 3D model files
2. **Arrange** — Models are positioned on the virtual build plate
3. **Configure** — User selects printer profile, filament profile, print profile (layer height, speeds, supports, infill, etc.)
4. **Slice** — Application slices the model into layers and generates perimeters, infill, support structures
5. **Preview** — User inspects the layer-by-layer toolpath preview with time/filament estimates
6. **Export / Send** — G-code is saved to file, SD card, or sent directly to printer via PrusaLink/PrusaConnect/OctoPrint

### 2. SLA (Stereolithography) Print Preparation
For resin printers (Prusa SL1, SL1S, SL2):
1. Import → Arrange → Configure (exposure times, supports, hollowing) → Slice → Export `.sl1` / `.sl2` archive

### 3. Multi-Material Print Preparation
For MMU-equipped printers: configure filament assignments per model volume, paint multi-material regions, configure wipe tower.

### 4. G-code Analysis & Re-slicing
Import existing G-code for time estimation and toolpath visualization without re-slicing.

### 5. Command-Line / Headless Slicing
Batch G-code export from `.3mf` project files via CLI (used in CI/automation workflows).

## Key Business Rules

- Layer heights must be compatible with nozzle diameter (0.1–0.35mm typical range)
- Support material is generated automatically based on overhang angle threshold
- Wipe tower is required for multi-material prints to purge filament between tool changes
- Seam placement (random, aligned, rear) affects print quality visibility
- Infill density and pattern determine part strength vs. print time trade-off
- First-layer settings (height, speed, temperature) are critical for bed adhesion

## Value Proposition

PrusaSlicer differentiates with:
- Tight integration with Prusa hardware profiles
- Arachne variable-width perimeter engine for thin-wall quality
- Advanced support structures (grid, snug, tree, organic)
- Multi-material painting workflow
- Open-source, actively maintained, AGPLv3
