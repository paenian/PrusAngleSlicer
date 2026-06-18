# Technology Stack — PrusaSlicer

## Language & Standard
- **C++17** — primary language throughout
- **CMake 3.13+** — build system (NOTE: CMake 3.28+ has a known issue; use 3.27 for this project)
- **AGPLv3+** license

## Core Third-Party Libraries

| Library | Version / Source | Role |
|---------|-----------------|------|
| **Boost** | External (deps/) | Filesystem, logging (trivial), serialization, geometry |
| **Eigen** | External (deps/) | Linear algebra, 3D transforms, matrix math |
| **Intel TBB** | External (deps/) | Parallel for, concurrent vectors, task scheduling |
| **Clipper / Clipper2** | Bundled + src/ | 2D polygon boolean operations (union, diff, offset) |
| **CGAL** | External (deps/) | 3D mesh boolean operations (MeshBoolean), convex hull |
| **GMP + MPFR** | External (deps/) | Exact arithmetic for CGAL |
| **OpenVDB** | External (deps/) | Volumetric mesh operations, SDF-based support |
| **wxWidgets 3.2** | External (deps/) | Cross-platform GUI framework |
| **Dear ImGui** | Bundled (bundled_deps/imgui/) | In-viewport overlay UI |
| **OpenGL / GLEW** | External (deps/) | 3D rendering |
| **Arachne** | In-tree (src/libslic3r/Arachne/) | Variable-width perimeter engine (from Cura) |
| **admesh** | Bundled | STL file reading + repair |
| **libnest2d** | Bundled | 2D bin packing for plate arrangement |
| **avrdude** | Bundled | Firmware flashing for Prusa printers |
| **libslic3r-arrange / libseqarrange** | In-tree | Object arrangement and sequential print ordering |
| **libigl** | Bundled (header-only) | Mesh geometry algorithms |
| **miniz** | Bundled | ZIP/deflate compression (for 3MF/SLA archives) |
| **OpenSSL** | External (deps/) | HTTPS for PrusaLink/Connect upload |
| **CURL** | External (deps/) | HTTP client for printer upload |
| **EXPAT** | External (deps/) | XML parsing (AMF, 3MF) |
| **Cereal** | External (deps/) | C++ serialization |
| **NLopt** | External (deps/) | Non-linear optimization (seam placement, etc.) |
| **NanoSVG** | External / Bundled | SVG parsing for custom shapes |
| **LibBGCode** | External (deps/) | Binary G-code format (next-gen .bgcode) |
| **fast_float** | Bundled | Fast string-to-float conversion |
| **ankerl/unordered_dense** | Bundled | High-performance hash maps |
| **Catch2** | External (deps/) | Unit testing framework |
| **OpenCSG** | External (deps/) | CSG rendering for SLA preview |
| **OpenEXR + Blosc** | External (deps/) | OpenVDB dependencies |

## Build Toolchain (macOS)

| Tool | Required Version | Status on This Machine |
|------|-----------------|----------------------|
| Apple Clang | 12+ | v21.0.0 (clang-2100.1.1.101) — OK |
| CMake | 3.13–3.27 (NOT 3.28+) | NOT INSTALLED — needs brew |
| automake | Any | NOT INSTALLED |
| autoconf | Any | NOT INSTALLED |
| git | Any | Available via Xcode CLI |
| gettext | Any | NOT INSTALLED |
| libtool | Any | NOT INSTALLED |
| texinfo | Any | NOT INSTALLED |
| m4 | Any | NOT INSTALLED |
| zlib | Any | INSTALLED via brew |
| ninja | Optional | NOT INSTALLED |
| Xcode Command Line Tools | Any | INSTALLED |

## Hardware (This Machine)
- **CPU**: Apple M4 Pro, 14 cores (ARM64 / apple-silicon)
- **RAM**: 24 GB
- **Disk**: 926 GB total, 12 GB used, 774 GB available — more than sufficient
- **macOS**: 26.5.1 (Build 25F80)

## Key Compilation Notes

1. All external dependencies must be built first via the `deps/` bootstrap
2. The dependency bundle path is hardcoded after build (wxWidgets limitation) — cannot move after build
3. For Apple Silicon (M4 Pro), CMake will auto-detect arm64 architecture
4. Parallel build recommended: `make -j14` (matches core count)
5. Debug build adds `-DCMAKE_BUILD_TYPE=Debug` for full assert testing
6. A full first build (deps + app) can take 60–120 minutes on this hardware
