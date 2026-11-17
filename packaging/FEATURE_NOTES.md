
## What's different in PrusAngleSlicer

PrusAngleSlicer is PrusaSlicer with **angled slicing** for regular (FFF) printers: the layers are tilted, so the nozzle follows a sloped surface instead of flat horizontal layers. It also includes profiles for belt printers.

- Switch to **Expert** mode (top right), then go to **Print Settings > Layers and perimeters > Angled slicing**. Set the angle and direction.
- Turn off skirt (set skirt loops to 0) and brim; angled slicing won't slice with them on. An anchor line on the bed holds the print down instead.
- The preview still shows flat layers; the exported G-code is tilted.

This is experimental. Watch the first layers of your first angled print.
