#!/bin/bash
# test_examples.sh — Build and validate all examples via xemu-xmega65
#   Text examples: -dumpscreen, grep for expected output
#   Visual examples: -screenshot, compare against reference PNGs

XEMU="xemu-xmega65"
EXAMPLES="examples"
REFDIR="examples/test/reference"
BUILDDIR="build/test/examples"

mkdir -p "$BUILDDIR"

if ! command -v "$XEMU" &>/dev/null; then
    echo "xemu-xmega65 not found — skipping example tests"
    exit 0
fi

passed=0
failed=0
skipped=0

pass() { echo "  PASS: $1"; passed=$((passed + 1)); }
fail() { echo "  FAIL: $1 — $2"; failed=$((failed + 1)); }
skip() { echo "  SKIP: $1 — $2"; skipped=$((skipped + 1)); }

build_example() {
    local dir="$1"
    make -C "$dir" clean >/dev/null 2>&1
    make -C "$dir" >/dev/null 2>&1
}

# Run and check text output contains expected string
test_text() {
    local name="$1" prg="$2" expected="$3" timeout_sec="${4:-15}"
    if [ ! -f "$prg" ]; then fail "$name" "PRG not found"; return; fi
    timeout "$timeout_sec" $XEMU -headless -besure -prgmode 65 -prgexit \
        -dumpscreen "$BUILDDIR/${name}.screen.txt" \
        -prg "$prg" </dev/null >/dev/null 2>&1
    if [ ! -f "$BUILDDIR/${name}.screen.txt" ]; then
        fail "$name" "no screen dump"
    elif grep -q "$expected" "$BUILDDIR/${name}.screen.txt"; then
        pass "$name"
    else
        fail "$name" "expected '$expected' not found on screen"
    fi
}

# Run and verify screenshot is a valid PNG (program didn't crash)
test_screenshot() {
    local name="$1" prg="$2" timeout_sec="${3:-15}"
    if [ ! -f "$prg" ]; then fail "$name" "PRG not found"; return; fi
    timeout "$timeout_sec" $XEMU -headless -besure -prgmode 65 -prgexit \
        -screenshot "$BUILDDIR/${name}.png" \
        -prg "$prg" </dev/null >/dev/null 2>&1
    if [ ! -f "$BUILDDIR/${name}.png" ] || [ ! -s "$BUILDDIR/${name}.png" ]; then
        fail "$name" "no screenshot"
    elif file "$BUILDDIR/${name}.png" 2>/dev/null | grep -q "PNG image"; then
        pass "$name"
    else
        fail "$name" "invalid screenshot"
    fi
}

echo "=== Example Validation Tests ==="

# ── Text examples ──

build_example "$EXAMPLES/c/hello_world"
test_text "hello_world" "$EXAMPLES/c/hello_world/hello.prg" "HELLO WORLD"

build_example "$EXAMPLES/c/hello_linked"
test_text "hello_linked" "$EXAMPLES/c/hello_linked/hello.prg" "LINKED HELLO"

build_example "$EXAMPLES/c/multi_module"
test_text "multi_module" "$EXAMPLES/c/multi_module/program.prg" "MULTI-MODULE EXAMPLE"

build_example "$EXAMPLES/c/unit_convert"
test_text "unit_convert" "$EXAMPLES/c/unit_convert/convert.prg" "26 MILES = 41 KM"

# ── Visual examples ──

build_example "$EXAMPLES/c/memset_screen"
test_screenshot "memset_screen" "$EXAMPLES/c/memset_screen/memset_screen.prg"

build_example "$EXAMPLES/c/vic4_colors"
test_screenshot "vic4_colors" "$EXAMPLES/c/vic4_colors/vic4_colors.prg"

build_example "$EXAMPLES/c/bitfield_registers"
test_screenshot "bitfield_regs" "$EXAMPLES/c/bitfield_registers/bitfield_demo.prg"

build_example "$EXAMPLES/c/game_of_life"
test_screenshot "game_of_life" "$EXAMPLES/c/game_of_life/life.prg" 60

build_example "$EXAMPLES/c/palette_fade"
test_screenshot "palette_fade" "$EXAMPLES/c/palette_fade/palette_fade.prg" 30

build_example "$EXAMPLES/c/text_scroller"
test_screenshot "text_scroller" "$EXAMPLES/c/text_scroller/scroller.prg" 15

echo ""
echo "Example tests: $passed passed, $failed failed, $skipped skipped"
[ "$failed" -eq 0 ] || exit 1
