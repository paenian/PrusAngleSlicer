# Dependencies — PrusaSlicer

## macOS Build Prerequisites

### Tools Required (from brew)

The official macOS build guide requires all of these:

```bash
brew update
brew install automake cmake git gettext libtool texinfo m4 zlib
```

IMPORTANT: CMake 3.28+ has a known incompatibility. The doc recommends CMake 3.27:
- Download from: https://github.com/Kitware/CMake/releases/tag/v3.27.9
- DMG: cmake-3.27.9-macos-universal.dmg
- After install, invoke as: /Applications/CMake.app/Contents/bin/cmake

### Current Tool Status on This Machine

| Tool | Required | Status |
|------|----------|--------|
| Apple Clang (21.0.0) | Yes | INSTALLED |
| Xcode CLI Tools | Yes | INSTALLED |
| cmake (3.13-3.27) | Yes | MISSING — needs CMake 3.27 |
| automake | Yes | MISSING |
| autoconf | Yes | MISSING (pulled in by automake) |
| git | Yes | INSTALLED (via Xcode CLI) |
| gettext | Yes | MISSING |
| libtool | Yes | MISSING |
| texinfo | Yes | MISSING |
| m4 | Yes | MISSING |
| zlib | Yes | INSTALLED (via brew) |
| ninja | Optional | MISSING |

### Missing Packages — Install Command

```bash
brew install automake git gettext libtool texinfo m4
```

Then install CMake 3.27 from DMG:
```
https://github.com/Kitware/CMake/releases/download/v3.27.9/cmake-3.27.9-macos-universal.dmg
```

Or, if brew installs a compatible cmake version (check: brew info cmake to verify it's <=3.27):
```bash
brew install cmake
```

## External Dependencies (Built by deps/ bootstrap)

These are automatically downloaded and built by the deps/CMakeLists.txt:

| Package | Version Range | Purpose |
|---------|--------------|---------|
| Boost | 1.75+ | Filesystem, log, serialization, geometry |
| Eigen | 3.4 | Linear algebra, 3D transforms |
| TBB (Intel oneAPI) | 2021+ | Parallel processing |
| wxWidgets | 3.2+ | GUI framework |
| OpenVDB | 9+ | Volumetric ops, SDF |
| CGAL | 5.5+ | 3D boolean mesh operations |
| GMP | Any | Exact arithmetic (CGAL dep) |
| MPFR | Any | Multi-precision floats (CGAL dep) |
| OpenSSL | 3.x | HTTPS transport |
| CURL | 7+ | HTTP client (printer upload) |
| EXPAT | Any | XML parsing |
| Cereal | Any | C++ serialization |
| NLopt | Any | Non-linear optimization |
| OpenCSG | Any | CSG rendering (SLA) |
| OpenEXR | Any | HDR images (OpenVDB dep) |
| Blosc | Any | Compression (OpenVDB dep) |
| GLEW | Any | OpenGL extension loading |
| JPEG | Any | Image format |
| NanoSVG | Any | SVG import |
| LibBGCode | Any | Binary G-code format |
| Catch2 | 2.x | Unit testing |

## Bundled Dependencies (In-Tree, No Download Needed)

These live in `bundled_deps/` and are always available:

| Package | Purpose |
|---------|---------|
| admesh | STL repair |
| agg | 2D rendering |
| ankerl/unordered_dense | Fast hash maps |
| avrdude | Firmware flashing |
| fast_float | String-to-float parsing |
| glu-libtess | Mesa tessellation |
| hidapi | USB HID (Prusa printer connection) |
| hints | Translation pot generator |
| imgui | In-viewport overlay UI |
| int128 | 128-bit integers for Clipper |
| libigl | Mesh geometry (header-only) |
| libnest2d | 2D bin packing |
| localesutils | Locale string utilities |
| miniz | ZIP/deflate |

## Build Process Summary (macOS)

Step 1 — Install brew tools:
```bash
brew install automake gettext libtool texinfo m4
# plus cmake 3.27 from DMG or brew
```

Step 2 — Build external dependencies (60-90 min, one-time):
```bash
cd deps
mkdir build && cd build
/Applications/CMake.app/Contents/bin/cmake .. -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0
make -j14
```

Step 3 — Build PrusaSlicer application (~30-60 min):
```bash
cd ../..
mkdir build && cd build
/Applications/CMake.app/Contents/bin/cmake .. \
    -DCMAKE_PREFIX_PATH="$PWD/../deps/build/destdir/usr/local" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0
make -j14
```

Step 4 — Run:
```bash
src/prusa-slicer
```

NOTE: The `CMAKE_OSX_DEPLOYMENT_TARGET` flag may be needed on macOS 26.x. Set it to your current macOS version (e.g., 26.0 or 14.0 for Sonoma baseline).
