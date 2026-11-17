#!/usr/bin/env bash
# Build a double-clickable macOS app bundle + .dmg from a finished CMake build.
#
#   packaging/macos/package.sh <build_dir> <out_dir> <label>
#
# <label> is the release label, e.g. 2.9.7-1. The app name comes from version.inc
# (SLIC3R_APP_NAME), so the same script packages PrusAngleSlicer and PrusSLASlicer.
#
# The result is ad-hoc signed only (no Apple Developer ID). Apple Silicon refuses to
# run code with no signature at all, so the ad-hoc signature is required; users still
# confirm the first launch in System Settings > Privacy & Security (see INSTALL.md).
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/../.." && pwd)
BUILD=$(cd "${1:?build dir}" && pwd)
OUT=${2:?out dir}
LABEL=${3:?release label}
mkdir -p "$OUT"; OUT=$(cd "$OUT" && pwd)

APP_NAME=$(sed -n 's/^set(SLIC3R_APP_NAME "\(.*\)")$/\1/p' "$ROOT/version.inc")
[ -n "$APP_NAME" ] || { echo "error: SLIC3R_APP_NAME not found in version.inc" >&2; exit 1; }

EXE="$BUILD/src/PrusaSlicer"
PLIST="$BUILD/src/Info.plist"
for f in "$EXE" "$PLIST"; do [ -f "$f" ] || { echo "error: missing $f" >&2; exit 1; }; done

APP="$OUT/$APP_NAME.app"
rm -rf "$APP"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources"

# The executable keeps its build name: Info.plist (CFBundleExecutable) and the app's
# own "open new instance" code both refer to Contents/MacOS/PrusaSlicer.
cp "$EXE" "$APP/Contents/MacOS/PrusaSlicer"
cp "$PLIST" "$APP/Contents/Info.plist"
plutil -lint "$APP/Contents/Info.plist" >/dev/null || { echo "error: invalid Info.plist" >&2; exit 1; }
# The app looks for its data in Contents/Resources (= <exe>/../Resources).
cp -R "$ROOT/resources/." "$APP/Contents/Resources/"
# Icons referenced by Info.plist live at the top of Resources.
cp "$ROOT/resources/icons/"*.icns "$APP/Contents/Resources/"

# Sanity checks: the bundle must contain what the app needs at runtime.
for f in Contents/Info.plist Contents/MacOS/PrusaSlicer Contents/Resources/PrusaSlicer.icns \
         Contents/Resources/profiles Contents/Resources/icons Contents/Resources/shaders; do
    [ -e "$APP/$f" ] || { echo "error: bundle is missing $f" >&2; exit 1; }
done
if otool -L "$APP/Contents/MacOS/PrusaSlicer" | tail -n +2 | grep -vqE '^\s*(/System/|/usr/lib/)'; then
    echo "error: executable links non-system dylibs that are not in the bundle:" >&2
    otool -L "$APP/Contents/MacOS/PrusaSlicer" >&2
    exit 1
fi

# Ad-hoc signature (no identity). Required for arm64 code to run at all.
codesign --force --deep --sign - --entitlements "$ROOT/src/platform/osx/entitlements.plist" "$APP"
codesign --verify --deep --strict "$APP"

# Disk image: the app + an Applications shortcut to drag it onto.
STAGE=$(mktemp -d)
trap 'rm -rf "$STAGE"' EXIT
cp -R "$APP" "$STAGE/"
ln -s /Applications "$STAGE/Applications"
DMG="$OUT/$APP_NAME-$LABEL-macOS-AppleSilicon.dmg"
rm -f "$DMG"
hdiutil create -volname "$APP_NAME" -srcfolder "$STAGE" -ov -format UDZO "$DMG" >/dev/null

echo "app: $APP"
echo "dmg: $DMG"
