# PrusAngleSlicer — Local Build Quickstart

Verified for: macOS 26.x, Apple Silicon (M4 Pro), Apple Clang 21, fresh Homebrew

---

## Prerequisites Check

Your machine has:
- Apple Clang 21 ✅
- Xcode Command Line Tools ✅
- Homebrew ✅ (but most build tools are missing)
- 14 CPU cores, 774 GB free disk ✅

**Expected total time**: ~2 hours (mostly waiting for deps to compile)

---

## Step 1 — Install Build Tools

```bash
brew install cmake automake autoconf git gettext libtool texinfo m4 zlib ninja
```

After installing, verify:
```bash
cmake --version       # Any version works (4.x included) — we use -DCMAKE_POLICY_VERSION_MINIMUM=3.5 to handle compat
clang --version       # should show Apple clang
make --version        # should show GNU make
```

---

## Step 2 — Build External Dependencies

This is a one-time step that downloads and compiles all third-party libraries (Boost, wxWidgets, Eigen, TBB, OpenVDB, CGAL, etc.). It takes 60–90 minutes on first run.

```bash
cd /Users/paulchase/github/PrusAngleSlicer/deps

# Remove previous failed attempt if any
rm -rf build
mkdir build
cd build

cmake .. \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0 \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5
make -j14
```

> **Important**: The `-DCMAKE_POLICY_VERSION_MINIMUM=3.5` flag is required because
> brew installs CMake 4.3+ which removed backward compatibility with cmake_minimum_required < 3.5.
> Several dependencies (Blosc, etc.) still declare older versions. This flag tells CMake to allow them.

> **Tips:**
> - Use `make -j14` to use all 14 cores — this significantly speeds up the build
> - If a dep fails, run `make -j1` to see the actual error (parallel output is harder to read)
> - The `destdir` path is baked into wxWidgets after build — **do not move the build directory** later

When complete, you should see a `destdir/` directory:
```bash
ls /Users/paulchase/github/PrusAngleSlicer/deps/build/destdir/usr/local/lib/
# Should show many .a static library files
```

---

## Step 3 — Build PrusaSlicer

```bash
cd /Users/paulchase/github/PrusAngleSlicer
mkdir -p build
cd build
cmake .. \
  -DCMAKE_PREFIX_PATH="$PWD/../deps/build/destdir/usr/local" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0 \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5
make -j14
```

> **Note:** `CMAKE_PREFIX_PATH` must be an absolute path. If you used a custom `DESTDIR` in Step 2, adjust accordingly.

This takes 20–40 minutes on first build. Incremental rebuilds (after code changes) are much faster — usually under 2 minutes for a few files.

---

## Step 4 — Launch the Slicer

```bash
/Users/paulchase/github/PrusAngleSlicer/build/src/prusa-slicer
```

Or from within the build directory:
```bash
cd /Users/paulchase/github/PrusAngleSlicer/build
src/prusa-slicer
```

The GUI should open. On first launch it will ask you to configure a printer profile.

---

## Step 5 — Run Unit Tests (Optional)

```bash
cd /Users/paulchase/github/PrusAngleSlicer/build
make test
# or to run a specific suite:
tests/fff_print/fff_print_tests
tests/libslic3r/libslic3r_tests
```

---

## Incremental Rebuild After Code Changes

After modifying source files, from the build directory:

```bash
cd /Users/paulchase/github/PrusAngleSlicer/build
make -j14
src/prusa-slicer
```

CMake tracks dependencies automatically — only changed files and their dependents recompile.

---

## Troubleshooting

### `Compatibility with CMake < 3.5 has been removed`
This happens with CMake 4.x. The fix is already applied in two places:
1. `cmake/modules/AddCMakeProject.cmake` — injects `-DCMAKE_POLICY_VERSION_MINIMUM=3.5` into all dep sub-builds
2. Pass `-DCMAKE_POLICY_VERSION_MINIMUM=3.5` on the command line for both deps and app builds

If you see this error, make sure you're using the cmake flags exactly as shown in Steps 2 and 3.

### `CMath::CMath target not found`
This was a CMake 3.28-specific issue. With CMake 4.x + the policy flag, it should not occur.
If it does, ensure `-DCMAKE_POLICY_VERSION_MINIMUM=3.5` is passed.

### `cmake` complains about deployment target
Add `-DCMAKE_OSX_DEPLOYMENT_TARGET=14.0` (or your macOS version) to the cmake command.

### `No CMAKE_CXX_COMPILER could be found`
Run:
```bash
sudo xcode-select --reset
```
Then retry cmake.

### `m4` errors or "incompatible m4 found"
Make sure Homebrew's m4 is first in PATH:
```bash
echo 'export PATH="/opt/homebrew/opt/m4/bin:$PATH"' >> ~/.zprofile
source ~/.zprofile
```

### Dep build fails partway through
Check which dep failed, then:
```bash
cd /Users/paulchase/github/PrusAngleSlicer/deps/build
make -j1 2>&1 | tail -50   # single-threaded for readable errors
```

### App opens but crashes immediately
Try the Debug build for better error messages:
```bash
cd /Users/paulchase/github/PrusAngleSlicer/build
cmake .. -DCMAKE_PREFIX_PATH="$PWD/../deps/build/destdir/usr/local" \
         -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0 \
         -DCMAKE_BUILD_TYPE=Debug
make -j14
src/prusa-slicer
```

---

## Quick Reference

| Task | Command |
|------|---------|
| Install build tools | `brew install cmake automake autoconf git gettext libtool texinfo m4 zlib ninja` |
| Build deps (first time) | `cd deps && rm -rf build && mkdir build && cd build && cmake .. -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0 -DCMAKE_POLICY_VERSION_MINIMUM=3.5 && make -j14` |
| Configure app | `cd build && cmake .. -DCMAKE_PREFIX_PATH="$PWD/../deps/build/destdir/usr/local" -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0 -DCMAKE_POLICY_VERSION_MINIMUM=3.5` |
| Build app | `cd build && make -j14` |
| Launch | `build/src/prusa-slicer` |
| Run tests | `cd build && make test` |
| Rebuild after changes | `cd build && make -j14` |
