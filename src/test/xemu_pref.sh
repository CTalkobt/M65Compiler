# xemu_pref.sh — a preferences directory of its own for each xemu run
# (sourced by the test scripts that run several xemu at once)
#
# At start xemu rewrites mega65-template.cfg and keymap-default.cfg in its
# preferences directory (through a .TMP file renamed into place): two
# instances starting together race on the rename, and the loser stops with an
# error and waits for its timeout (no memory dump). XEMU_PREF_DIR moves the
# directory: xemu_pref DIR makes one of links to the real one's files, but for
# those two, which each run then writes in its own.
#
#   xemu_pref_init FARM   once: FARM, the links (to $XEMU_PREF_DIR or xemu's
#                         default directory, ~/.local/share/xemu-lgb/mega65)
#   xemu_pref DIR         a run's directory: DIR, a copy of FARM's links;
#                         then XEMU_PREF_DIR="$PWD/DIR" (absolute) for the run
xemu_pref_init() {
    local farm="$1" src="${XEMU_PREF_DIR:-$HOME/.local/share/xemu-lgb/mega65}" f
    rm -rf "$farm"
    mkdir -p "$farm"
    for f in "$src"/*; do
        [ -e "$f" ] || continue
        case "$(basename "$f")" in
            *-template.cfg|keymap-default.cfg) ;;
            *) ln -s "$f" "$farm/" ;;
        esac
    done
    XEMU_PREF_FARM="$farm"
}

xemu_pref() {
    rm -rf "$1"
    cp -as "$PWD/$XEMU_PREF_FARM" "$1" 2>/dev/null || cp -as "$XEMU_PREF_FARM" "$1"
}
