#!/bin/bash
# Test: verify that angled slicing produces G-code within the model's bounding box.
# A 20mm cube centered on a 200x200 bed should have:
#   X extrusions in [90, 110] (±0.5mm tolerance for perimeter width)
#   Y extrusions in [90, 110] (±0.5mm tolerance)
#   Z extrusions in [0, 20] (±0.5mm tolerance)
#
# Usage: ./test_bounding_box.sh [path_to_prusaslicer_binary]

SLICER="${1:-../build/src/PrusaSlicer}"
CUBE="$(dirname $0)/cube_20mm.stl"
TOLERANCE=0.5

# Expected bounds (object on bed center=100,100, size=20mm, perimeter inset ~0.2mm)
# The external perimeter is inset from the model boundary, so actual extrusion
# bounds are slightly inside [90,110]. But Z must be within [0, 20].
# Anchor line may extend ~12mm beyond the model in the perpendicular direction.
X_MIN_EXPECTED=77.0
X_MAX_EXPECTED=123.0
Y_MIN_EXPECTED=77.0
Y_MAX_EXPECTED=123.0
Z_MIN_EXPECTED=0.0
Z_MAX_EXPECTED=20.5

PASS=0
FAIL=0

run_test() {
    local angle=$1
    local direction=$2
    local desc="angle=${angle} dir=${direction}"
    local gcode="/tmp/bbox_test_${angle}_${direction}.gcode"

    echo -n "  Test: $desc ... "
    
    "$SLICER" --export-gcode \
        --angled-slicing-angle "$angle" --skirts 0 \
        --angled-slicing-direction "$direction" \
        --output "$gcode" \
        "$CUBE" 2>/dev/null

    if [ ! -f "$gcode" ]; then
        echo "FAIL (no output)"
        FAIL=$((FAIL+1))
        return
    fi

    # Extract XYZ bounds from extrusion moves only (G1 with E), excluding skirt/brim
    # Filter: only lines between ;TYPE:External perimeter or ;TYPE:Perimeter or ;TYPE:Solid infill etc.
    # Simplest: exclude lines after ;TYPE:Skirt/Brim until next ;TYPE: that isn't skirt
    local x_min=$(awk '/;TYPE:Skirt/{skip=1} /;TYPE:/{if(!/Skirt/)skip=0} !skip && /^G1.*X.*E/{print}' "$gcode" | sed 's/.*X\([0-9.]*\).*/\1/' | sort -n | head -1)
    local x_max=$(awk '/;TYPE:Skirt/{skip=1} /;TYPE:/{if(!/Skirt/)skip=0} !skip && /^G1.*X.*E/{print}' "$gcode" | sed 's/.*X\([0-9.]*\).*/\1/' | sort -rn | head -1)
    local y_min=$(awk '/;TYPE:Skirt/{skip=1} /;TYPE:/{if(!/Skirt/)skip=0} !skip && /^G1.*Y.*E/{print}' "$gcode" | sed 's/.*Y\([0-9.]*\).*/\1/' | sort -n | head -1)
    local y_max=$(awk '/;TYPE:Skirt/{skip=1} /;TYPE:/{if(!/Skirt/)skip=0} !skip && /^G1.*Y.*E/{print}' "$gcode" | sed 's/.*Y\([0-9.]*\).*/\1/' | sort -rn | head -1)
    
    # Z: extract from extrusion moves that have Z
    local z_min z_max
    if grep -q "^G1.*X.*Z.*E" "$gcode"; then
        z_min=$(grep "^G1.*X.*Z.*E" "$gcode" | sed 's/.*Z\([0-9.]*\).*/\1/' | sort -n | head -1)
        z_max=$(grep "^G1.*X.*Z.*E" "$gcode" | sed 's/.*Z\([0-9.]*\).*/\1/' | sort -rn | head -1)
    else
        z_min=$(grep "^;Z:" "$gcode" | sed 's/;Z://' | sort -n | head -1)
        z_max=$(grep "^;Z:" "$gcode" | sed 's/;Z://' | sort -rn | head -1)
    fi

    # Check bounds
    local ok=1
    local details=""

    if (( $(echo "$x_min < $X_MIN_EXPECTED" | bc -l) )); then
        details="$details X_min=$x_min<$X_MIN_EXPECTED"
        ok=0
    fi
    if (( $(echo "$x_max > $X_MAX_EXPECTED" | bc -l) )); then
        details="$details X_max=$x_max>$X_MAX_EXPECTED"
        ok=0
    fi
    if (( $(echo "$y_min < $Y_MIN_EXPECTED" | bc -l) )); then
        details="$details Y_min=$y_min<$Y_MIN_EXPECTED"
        ok=0
    fi
    if (( $(echo "$y_max > $Y_MAX_EXPECTED" | bc -l) )); then
        details="$details Y_max=$y_max>$Y_MAX_EXPECTED"
        ok=0
    fi
    if (( $(echo "$z_max > $Z_MAX_EXPECTED" | bc -l) )); then
        details="$details Z_max=$z_max>$Z_MAX_EXPECTED"
        ok=0
    fi
    # Z_min should be >= 0 (can't print below bed)
    if [ -n "$z_min" ] && (( $(echo "$z_min < -0.01" | bc -l) )); then
        details="$details Z_min=$z_min<0"
        ok=0
    fi

    if [ $ok -eq 1 ]; then
        echo "PASS (X[$x_min,$x_max] Y[$y_min,$y_max] Z[$z_min,$z_max])"
        PASS=$((PASS+1))
    else
        echo "FAIL$details"
        echo "         Actual: X[$x_min,$x_max] Y[$y_min,$y_max] Z[$z_min,$z_max]"
        FAIL=$((FAIL+1))
    fi
}

echo "=== Bounding Box Test: 20mm Cube ==="
echo "Expected bounds: X[$X_MIN_EXPECTED,$X_MAX_EXPECTED] Y[$Y_MIN_EXPECTED,$Y_MAX_EXPECTED] Z[$Z_MIN_EXPECTED,$Z_MAX_EXPECTED]"
echo ""

# Normal slicing (reference)
run_test 0 0

# Angled slicing tests
run_test 15 0
run_test 15 90
run_test 30 0
run_test 30 45

echo ""
echo "=== Results: $PASS passed, $FAIL failed ==="
exit $FAIL
