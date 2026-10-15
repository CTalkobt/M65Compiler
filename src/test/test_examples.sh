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
    timeout "$timeout_sec" $XEMU -headless -besure -sleepless -prgmode 65 -prgexit \
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
    timeout "$timeout_sec" $XEMU -headless -besure -sleepless -prgmode 65 -prgexit \
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
test_text "multi_module" "$EXAMPLES/c/multi_module/program.prg" "^6/6 TESTS PASSED"

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

build_example "$EXAMPLES/c/far_scroller"
test_screenshot "far_scroller" "$EXAMPLES/c/far_scroller/far_scroller.prg" 15

build_example "$EXAMPLES/c/invaders"
test_screenshot "invaders" "$EXAMPLES/c/invaders/invaders.prg" 15

build_example "$EXAMPLES/c/mandelbrot"
test_screenshot "mandelbrot" "$EXAMPLES/c/mandelbrot/mandelbrot.prg" 15

build_example "$EXAMPLES/c/musicgen"
test_screenshot "musicgen" "$EXAMPLES/c/musicgen/musicgen.prg" 10
# the generator test: songs of every style checked against chords and scales
if make -C "$EXAMPLES/c/musicgen" check >"$BUILDDIR/musicgen_check.txt" 2>&1; then
    pass "musicgen songs"
else
    fail "musicgen songs" "$(tail -1 "$BUILDDIR/musicgen_check.txt")"
fi

build_example "$EXAMPLES/c/calc_graph"
test_screenshot "calc_graph" "$EXAMPLES/c/calc_graph/calc_graph.prg" 10
# the expression compiler and evaluator, the raster interrupt during floats
if make -C "$EXAMPLES/c/calc_graph" check >"$BUILDDIR/calc_graph_check.txt" 2>&1; then
    pass "calc_graph check"
else
    fail "calc_graph check" "$(tail -1 "$BUILDDIR/calc_graph_check.txt")"
fi

build_example "$EXAMPLES/c/keyboard_viewer"
test_screenshot "keyboard_viewer" "$EXAMPLES/c/keyboard_viewer/keyboard_viewer.prg" 10

build_example "$EXAMPLES/c/rtc_clock"
test_screenshot "rtc_clock" "$EXAMPLES/c/rtc_clock/rtc_clock.prg" 10
# the RTC read twice, a second apart: valid BCD values in range
if make -C "$EXAMPLES/c/rtc_clock" check >"$BUILDDIR/rtc_clock_check.txt" 2>&1; then
    pass "rtc_clock read"
else
    fail "rtc_clock read" "$(tail -1 "$BUILDDIR/rtc_clock_check.txt")"
fi

build_example "$EXAMPLES/c/rogue"
test_screenshot "rogue" "$EXAMPLES/c/rogue/rogue.prg" 10
# levels from fixed seeds (reachability, placement), scripted moves, a fight, a random walk
if make -C "$EXAMPLES/c/rogue" check >"$BUILDDIR/rogue_check.txt" 2>&1; then
    pass "rogue levels"
else
    fail "rogue levels" "$(tail -1 "$BUILDDIR/rogue_check.txt")"
fi

build_example "$EXAMPLES/c/forth"
test_text "forth" "$EXAMPLES/c/forth/forth.prg" "COUNT 0 1 2 3 4" 10
# the Forth test build: 21 lines of Forth and their output
if make -C "$EXAMPLES/c/forth" check >"$BUILDDIR/forth_check.txt" 2>&1; then
    pass "forth checks"
else
    fail "forth checks" "$(tail -1 "$BUILDDIR/forth_check.txt")"
fi

# The examples disk: every example on a D81 with a menu (AUTOBOOT.C65);
# a program DLOADed from it runs
make -C "$EXAMPLES/disk" -B examples.d81 >"$BUILDDIR/disk.txt" 2>&1
if grep -q "Status: OK" "$BUILDDIR/disk.txt" && ./bin/disk45 list "$EXAMPLES/disk/examples.d81" | grep -q '"AUTOBOOT.C65"'; then
    pass "examples disk"
else
    fail "examples disk" "$(tail -1 "$BUILDDIR/disk.txt")"
fi
printf 'start:\n  dload "hello world"\n' > "$BUILDDIR/chain.bas"
./bin/basic45 "$BUILDDIR/chain.bas" -o "$BUILDDIR/chain.prg" >/dev/null 2>&1
rm -f "$BUILDDIR/chain.screen.txt"
timeout 25 $XEMU -headless -besure -sleepless -prgmode 65 -prgexit -8 "$EXAMPLES/disk/examples.d81" \
    -dumpscreen "$BUILDDIR/chain.screen.txt" -prg "$BUILDDIR/chain.prg" </dev/null >/dev/null 2>&1
if grep -q "HELLO WORLD" "$BUILDDIR/chain.screen.txt" 2>/dev/null; then
    pass "examples disk: DLOAD runs a program"
else
    fail "examples disk: DLOAD runs a program" "no HELLO WORLD"
fi

echo ""
echo "Example tests: $passed passed, $failed failed, $skipped skipped"
[ "$failed" -eq 0 ] || exit 1
