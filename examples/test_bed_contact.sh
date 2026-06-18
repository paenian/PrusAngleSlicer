#!/bin/bash
# Test: verify bed contact and examine edge layers for angled slicing.
#
# For a 20mm cube at any slicing angle:
# - Some extrusion points should be at Z=0 (bed contact)
# - Z=0 points should eventually span the full base (X and Y extent)
# - First and last layers should be examined for correctness
#
# Usage: ./test_bed_contact.sh [path_to_prusaslicer_binary]

SLICER="${1:-../build/src/PrusaSlicer}"
CUBE="$(dirname $0)/cube_20mm.stl"

echo "=== Bed Contact & Edge Layer Test: 20mm Cube ==="
echo "    Model base: X[90, 110] Y[90, 110] (bed center=100,100)"
echo ""

run_test() {
    local angle=$1
    local direction=$2
    local desc="angle=${angle} dir=${direction}"
    local gcode="/tmp/bed_contact_${angle}_${direction}.gcode"

    echo "--- Test: $desc ---"
    
    "$SLICER" --export-gcode \
        --angled-slicing-angle "$angle" \
        --angled-slicing-direction "$direction" \
        --output "$gcode" \
        "$CUBE" 2>/dev/null

    if [ ! -f "$gcode" ]; then
        echo "  FAIL: no output file"
        return 1
    fi

    local total_layers=$(grep -c "^;LAYER_CHANGE" "$gcode")
    echo "  Total layers: $total_layers"

    if [ "$angle" = "0" ]; then
        echo "  (Normal slicing: no Z variation within layers)"
        echo ""
        return 0
    fi

    # Find Z=0 extrusion points (bed contact)
    local z0_count=$(grep -c "^G1.*Z0 .*E" "$gcode")
    local z0_x_min=$(grep "^G1.*Z0 .*E" "$gcode" | sed 's/.*X\([0-9.]*\).*/\1/' | sort -n | head -1)
    local z0_x_max=$(grep "^G1.*Z0 .*E" "$gcode" | sed 's/.*X\([0-9.]*\).*/\1/' | sort -rn | head -1)
    local z0_y_min=$(grep "^G1.*Z0 .*E" "$gcode" | sed 's/.*Y\([0-9.]*\).*/\1/' | sort -n | head -1)
    local z0_y_max=$(grep "^G1.*Z0 .*E" "$gcode" | sed 's/.*Y\([0-9.]*\).*/\1/' | sort -rn | head -1)

    echo "  Bed contact (Z=0) points: $z0_count"
    echo "  Z=0 X range: [$z0_x_min, $z0_x_max] (model base: [90, 110])"
    echo "  Z=0 Y range: [$z0_y_min, $z0_y_max] (model base: [90, 110])"

    if [ "$z0_count" = "0" ]; then
        echo "  WARNING: No Z=0 points! Base not touching bed."
    fi

    # First 3 layers detail
    echo ""
    echo "  First 3 layers (edge case - entering the model):"
    for layer_n in 1 2 3; do
        local layer_z_min=$(awk -v n=$layer_n '/^;LAYER_CHANGE/{c++} c==n && /^G1.*Z[0-9].*E/{print}' "$gcode" | sed 's/.*Z\([0-9.]*\).*/\1/' | sort -n | head -1)
        local layer_z_max=$(awk -v n=$layer_n '/^;LAYER_CHANGE/{c++} c==n && /^G1.*Z[0-9].*E/{print}' "$gcode" | sed 's/.*Z\([0-9.]*\).*/\1/' | sort -rn | head -1)
        local layer_x_min=$(awk -v n=$layer_n '/^;LAYER_CHANGE/{c++} c==n && /^G1.*X.*E/{print}' "$gcode" | sed 's/.*X\([0-9.]*\).*/\1/' | sort -n | head -1)
        local layer_x_max=$(awk -v n=$layer_n '/^;LAYER_CHANGE/{c++} c==n && /^G1.*X.*E/{print}' "$gcode" | sed 's/.*X\([0-9.]*\).*/\1/' | sort -rn | head -1)
        local layer_moves=$(awk -v n=$layer_n '/^;LAYER_CHANGE/{c++} c==n && /^G1.*E/{cnt++} c==(n+1){exit} END{print cnt+0}' "$gcode")
        echo "    Layer $layer_n: $layer_moves moves, X=[$layer_x_min,$layer_x_max] Z=[$layer_z_min,$layer_z_max]"
    done

    # Last 3 layers detail
    echo ""
    echo "  Last 3 layers (edge case - exiting the model):"
    for offset in 2 1 0; do
        local layer_n=$((total_layers - offset))
        local layer_z_min=$(awk -v n=$layer_n '/^;LAYER_CHANGE/{c++} c==n && /^G1.*Z[0-9].*E/{print}' "$gcode" | sed 's/.*Z\([0-9.]*\).*/\1/' | sort -n | head -1)
        local layer_z_max=$(awk -v n=$layer_n '/^;LAYER_CHANGE/{c++} c==n && /^G1.*Z[0-9].*E/{print}' "$gcode" | sed 's/.*Z\([0-9.]*\).*/\1/' | sort -rn | head -1)
        local layer_x_min=$(awk -v n=$layer_n '/^;LAYER_CHANGE/{c++} c==n && /^G1.*X.*E/{print}' "$gcode" | sed 's/.*X\([0-9.]*\).*/\1/' | sort -n | head -1)
        local layer_x_max=$(awk -v n=$layer_n '/^;LAYER_CHANGE/{c++} c==n && /^G1.*X.*E/{print}' "$gcode" | sed 's/.*X\([0-9.]*\).*/\1/' | sort -rn | head -1)
        local layer_moves=$(awk -v n=$layer_n '/^;LAYER_CHANGE/{c++} c==n && /^G1.*E/{cnt++} c==(n+1){exit} END{print cnt+0}' "$gcode")
        echo "    Layer $layer_n: $layer_moves moves, X=[$layer_x_min,$layer_x_max] Z=[$layer_z_min,$layer_z_max]"
    done
    echo ""
}

run_test 0 0
run_test 15 0
run_test 30 0
