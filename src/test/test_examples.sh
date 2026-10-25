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

# Run with a disk in drive 8 and check text output contains expected string
test_text_disk() {
    local name="$1" prg="$2" disk="$3" expected="$4" timeout_sec="${5:-15}"
    if [ ! -f "$prg" ] || [ ! -f "$disk" ]; then fail "$name" "PRG or disk not found"; return; fi
    timeout "$timeout_sec" $XEMU -headless -besure -sleepless -prgmode 65 -prgexit \
        -dumpscreen "$BUILDDIR/${name}.screen.txt" \
        -8 "$PWD/$disk" -prg "$prg" </dev/null >/dev/null 2>&1
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

build_example "$EXAMPLES/c/ext_tour"
test_text "ext_tour" "$EXAMPLES/c/ext_tour/ext_tour.prg" "^19/19 EXTENSIONS WORK"
# the tour built with both calling conventions (-fno-zpcall: __CC45_STACK__)
if make -C "$EXAMPLES/c/ext_tour" check >"$BUILDDIR/ext_tour_check.txt" 2>&1; then
    pass "ext_tour both conventions"
else
    fail "ext_tour both conventions" "$(tail -1 "$BUILDDIR/ext_tour_check.txt")"
fi

build_example "$EXAMPLES/c/overlays"
test_text_disk "overlays" "$EXAMPLES/c/overlays/ovldemo.prg" "$EXAMPLES/c/overlays/ovldemo.d81" "^3/3 OVERLAYS RAN"

build_example "$EXAMPLES/c/mapped_overlays"
test_text_disk "mapped_overlays" "$EXAMPLES/c/mapped_overlays/mapdemo.prg" "$EXAMPLES/c/mapped_overlays/mapdemo.d81" "^MAPPED OVERLAYS: 6/6 PROVEN"

build_example "$EXAMPLES/c/far_functions"
test_text_disk "far_functions" "$EXAMPLES/c/far_functions/fardemo.prg" "$EXAMPLES/c/far_functions/fardemo.d81" "^6/6 FAR FUNCTIONS RAN"

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

build_example "$EXAMPLES/c/sample_player"
test_screenshot "sample_player" "$EXAMPLES/c/sample_player/sample_player.prg" 10
# the synthesized samples and the audio DMA channels
if make -C "$EXAMPLES/c/sample_player" check >"$BUILDDIR/sample_player_check.txt" 2>&1; then
    pass "sample_player check"
else
    fail "sample_player check" "$(tail -1 "$BUILDDIR/sample_player_check.txt")"
fi

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

build_example "$EXAMPLES/c/hanger"
test_text "hanger" "$EXAMPLES/c/hanger/hanger.prg" "HANGMAN FOR THE MEGA65" 10
# the self-playing build guesses its way through the levels; xemu runs
# -sleepless, so in 20 s it may have lost the game and be back at the title,
# which then shows the best score (only after a game)
make -C "$EXAMPLES/c/hanger" demo >/dev/null 2>&1
test_text "hanger demo" "$EXAMPLES/c/hanger/hangerdemo.prg" "LEVEL [1-6] OF 6\|BEST SCORE [1-9]" 20

build_example "$EXAMPLES/c/fcm_graphics"
# the test build: 100 frames of palette cycling, then fcm_disable and a report
make -C "$EXAMPLES/c/fcm_graphics" check >/dev/null 2>&1
test_text "fcm_graphics" "$EXAMPLES/c/fcm_graphics/fcmcheck.prg" "FCM CHECK OK, 100 FRAMES" 20

build_example "$EXAMPLES/c/tunes"
# the test build: the music from the raster interrupt for 150 frames, then
# the sick tempo for 100
make -C "$EXAMPLES/c/tunes" check >/dev/null 2>&1
test_text "tunes" "$EXAMPLES/c/tunes/tunescheck.prg" "TUNES CHECK OK: 30 TICKS" 20

build_example "$EXAMPLES/c/glamlambs"
test_text_disk "glamlambs" "$EXAMPLES/c/glamlambs/glamlambs.prg" "$EXAMPLES/c/glamlambs/glamlambs.d81" "SHEEP MAKEOVER GAME" 15
# the self-playing build: a season in the field; its music loaded from the
# disk into bank 5 and playing (the demo writes its state to $C0F0-$C0F3)
make -C "$EXAMPLES/c/glamlambs" demo >/dev/null 2>&1
# (the score line, on the field's screen or the salon's: two bytes a cell,
# which xemu's screen dump shows as S@C@O@R@E@)
test_text_disk "glamlambs demo" "$EXAMPLES/c/glamlambs/glamdemo.prg" "$EXAMPLES/c/glamlambs/glamdemo.d81" "SCORE\|S@C@O@R@E@" 15
rm -f "$BUILDDIR/glamdemo.mem"
timeout 15 $XEMU -headless -besure -sleepless -prgmode 65 -prgexit -dumpmem "$BUILDDIR/glamdemo.mem" \
    -8 "$PWD/$EXAMPLES/c/glamlambs/glamdemo.d81" -prg "$EXAMPLES/c/glamlambs/glamdemo.prg" </dev/null >/dev/null 2>&1
if [ -f "$BUILDDIR/glamdemo.mem" ] && python3 - "$BUILDDIR/glamdemo.mem" <<'PY'
import sys
m = open(sys.argv[1], 'rb').read()
sys.exit(0 if m[0xC0F0] == 1 and m[0xC0F1] in (1, 2) and m[0xC0F2] | m[0xC0F3] else 1)
PY
then pass "glamlambs music"; else fail "glamlambs music" "the music did not load or play"; fi
# its salon: GLAMSALON and GLAMSALON2 loaded (into banks 4 and 5), the room
# shown in full-colour characters on a two-byte cell screen (CHR16, FCLRHI)
if [ -f "$BUILDDIR/glamdemo.mem" ] && python3 - "$BUILDDIR/glamdemo.mem" <<'PY'
import sys
m = open(sys.argv[1], 'rb').read()
sys.exit(0 if m[0xC0FA] == 1 and m[0xC0FB] & 0x05 == 0x05 and m[0x52800:0x52802] != b"\0\0" else 1)
PY
then pass "glamlambs salon"; else fail "glamlambs salon" "the salon's files did not load or the room was not shown"; fi
# the styling studio (the demo choosing it on the title): no season, the
# salon shown (the demo writes the studio's flag to $C0FC)
make -C "$EXAMPLES/c/glamlambs" demo-studio >/dev/null 2>&1
rm -f "$BUILDDIR/glamstudio.mem"
timeout 15 $XEMU -headless -besure -sleepless -prgmode 65 -prgexit -dumpmem "$BUILDDIR/glamstudio.mem" \
    -8 "$PWD/$EXAMPLES/c/glamlambs/glamstudio.d81" -prg "$EXAMPLES/c/glamlambs/glamstudio.prg" </dev/null >/dev/null 2>&1
if [ -f "$BUILDDIR/glamstudio.mem" ] && python3 - "$BUILDDIR/glamstudio.mem" <<'PY'
import sys
m = open(sys.argv[1], 'rb').read()
sys.exit(0 if m[0xC0FC] == 1 and m[0xC0FB] & 0x05 == 0x05 and m[0xC0F4] == 0 else 1)
PY
then pass "glamlambs styling studio"; else fail "glamlambs styling studio" "the studio did not start or show the salon"; fi

build_example "$EXAMPLES/c/forth"
test_text "forth" "$EXAMPLES/c/forth/forth.prg" "COUNT 0 1 2 3 4" 10
# the Forth test build: 21 lines of Forth and their output
if make -C "$EXAMPLES/c/forth" check >"$BUILDDIR/forth_check.txt" 2>&1; then
    pass "forth checks"
else
    fail "forth checks" "$(tail -1 "$BUILDDIR/forth_check.txt")"
fi

# The examples disks: the games and the rest on two D81s, each with its
# menu (AUTOBOOT.C65, MENU); the menu shows the disk's title, and a program
# DLOADed from each disk runs (Glam Lambs loading its music into bank 5)
make -C "$EXAMPLES/disk" -B disks >"$BUILDDIR/disk.txt" 2>&1
if [ "$(grep -c "Status: OK" "$BUILDDIR/disk.txt")" = 2 ] &&
   ./bin/disk45 list "$EXAMPLES/disk/games.d81" | grep -q '"CC45 GAMES"' &&
   ./bin/disk45 list "$EXAMPLES/disk/demos.d81" | grep -q '"CC45 DEMOS"' &&
   ./bin/disk45 list "$EXAMPLES/disk/games.d81" | grep -q '"AUTOBOOT.C65"' &&
   ./bin/disk45 list "$EXAMPLES/disk/demos.d81" | grep -q '"AUTOBOOT.C65"'; then
    pass "examples disks"
else
    fail "examples disks" "$(tail -1 "$BUILDDIR/disk.txt")"
fi
test_text_disk "games disk menu" "$EXAMPLES/disk/games_menu.prg" "$EXAMPLES/disk/games.d81" "CC45 GAMES FOR THE MEGA65" 10
test_text_disk "demos disk menu" "$EXAMPLES/disk/demos_menu.prg" "$EXAMPLES/disk/demos.d81" "CC45 DEMOS FOR THE MEGA65" 10
printf 'start:\n  dload "hello world"\n' > "$BUILDDIR/chain.bas"
./bin/basic45 "$BUILDDIR/chain.bas" -o "$BUILDDIR/chain.prg" >/dev/null 2>&1
test_text_disk "demos disk: DLOAD runs a program" "$BUILDDIR/chain.prg" "$EXAMPLES/disk/demos.d81" "HELLO WORLD" 25
printf 'start:\n  dload "glamlambs"\n' > "$BUILDDIR/chain2.bas"
./bin/basic45 "$BUILDDIR/chain2.bas" -o "$BUILDDIR/chain2.prg" >/dev/null 2>&1
rm -f "$BUILDDIR/chain2.mem"
timeout 15 $XEMU -headless -besure -sleepless -prgmode 65 -prgexit -8 "$PWD/$EXAMPLES/disk/games.d81" \
    -dumpmem "$BUILDDIR/chain2.mem" -prg "$BUILDDIR/chain2.prg" </dev/null >/dev/null 2>&1
if [ -f "$BUILDDIR/chain2.mem" ] && python3 - "$BUILDDIR/chain2.mem" <<'PY'
import sys
m = open(sys.argv[1], 'rb').read()
sys.exit(0 if len(m) > 0x50000 and m[0x50000] == 2 else 1)
PY
then pass "games disk: DLOAD runs Glam Lambs, its music loaded"; else fail "games disk: DLOAD runs Glam Lambs" "no music in bank 5"; fi

echo ""
echo "Example tests: $passed passed, $failed failed, $skipped skipped"
[ "$failed" -eq 0 ] || exit 1
