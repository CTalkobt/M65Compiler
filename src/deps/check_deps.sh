#!/bin/sh
# check_deps.sh: what the build needs and what this machine has (#619).
#   check_deps.sh              the report of `make check-deps`: each dependency
#                              found or missing, and the command that installs
#                              what is missing on this distribution. Exit 1
#                              only when a required one is missing.
#   check_deps.sh --probe      the Makefile's detection: the words of what is
#                              there, of "cxx cc zlib zlib-bundled zlib-git
#                              sqlite fuse3" (zlib: the system's headers;
#                              zlib-bundled: third_party/zlib has its source;
#                              zlib-git: a git checkout, whose submodule git
#                              can fetch; cc: the C compiler, which only the
#                              bundled zlib needs)
#   check_deps.sh --hint G...  the install command for the groups G (base zlib
#                              sqlite fuse test), for the Makefile's messages
#   check_deps.sh --add G      the same as "to add it: ..." (the summary's line
#                              for an optional feature left out)
# Environment: CXX (default g++), CC (default cc), EXTRA_CXXFLAGS,
# SYSTEM_ZLIB (1: the system's zlib and not the bundled one, as make
# SYSTEM_ZLIB=1), GNU_MAKE_VERSION (the running make's, from the Makefile;
# else gmake or make is asked), DEPS_FAMILY (a family of deps_table.sh
# instead of the detected one).
# POSIX sh: runs on macOS (BSD userland) and Alpine (busybox).

. "$(dirname "$0")/deps_table.sh"

CXX=${CXX:-g++}
CC=${CC:-cc}
# the tree (this file is src/deps/check_deps.sh) and its bundled zlib (#620)
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
ZLIB_DIR=third_party/zlib
ZLIB_FETCH="git submodule update --init $ZLIB_DIR"
# GNU make 3.81 is the oldest the Makefile is checked with (README.md)
MIN_MAKE_MAJOR=3
MIN_MAKE_MINOR=81

have() { command -v "$1" >/dev/null 2>&1; }

# has_header H: the compiler finds <H> (preprocessed only: nothing is written)
has_header() {
    # shellcheck disable=SC2086
    printf '#include <%s>\n' "$1" | $CXX $EXTRA_CXXFLAGS -E -x c++ - >/dev/null 2>&1
}

# has_cxx17: the compiler is there and has C++17 with <filesystem>
has_cxx17() {
    # shellcheck disable=SC2086
    printf '#include <filesystem>\n#include <optional>\n#include <string_view>\n' |
        $CXX $EXTRA_CXXFLAGS -std=c++17 -E -x c++ - >/dev/null 2>&1
}

# has_cc: the C compiler works
has_cc() {
    # shellcheck disable=SC2086
    printf 'int x;\n' | $CC -E -x c - >/dev/null 2>&1
}

# has_bundled_zlib: the submodule third_party/zlib has zlib's source (it is an
# empty directory after a clone without --recurse-submodules, and in a source
# archive)
has_bundled_zlib() { [ -f "$ROOT/$ZLIB_DIR/zlib.h" ] && [ -f "$ROOT/$ZLIB_DIR/inflate.c" ]; }

# is_git_checkout: git can fetch the submodule here (not in an unpacked
# source archive, nor in a container given the files alone)
is_git_checkout() {
    [ -e "$ROOT/.git" ] && [ -f "$ROOT/.gitmodules" ] && have git &&
        (cd "$ROOT" && git rev-parse --git-dir) >/dev/null 2>&1
}

has_fuse3() { have pkg-config && pkg-config --exists fuse3 2>/dev/null; }

detect_family() {
    if [ -n "$DEPS_FAMILY" ]; then echo "$DEPS_FAMILY"; return 0; fi
    if [ "$(uname -s)" = Darwin ]; then echo macos; return 0; fi
    [ -r /etc/os-release ] || return 1
    for id in $(. /etc/os-release; echo "$ID $ID_LIKE"); do
        case $id in
        debian|ubuntu|linuxmint|pop|raspbian) echo debian; return 0 ;;
        fedora) echo fedora; return 0 ;;
        rhel|centos|rocky|almalinux) echo rhel; return 0 ;;
        arch|manjaro|endeavouros) echo arch; return 0 ;;
        opensuse*|suse|sles) echo suse; return 0 ;;
        alpine) echo alpine; return 0 ;;
        esac
    done
    return 1
}

FAMILY=$(detect_family) || FAMILY=
deps_family "$FAMILY" || FAMILY=

# hint GROUP...: the command that installs those groups here
hint() {
    pk= pre=
    for g in "$@"; do
        case $g in
        base)   pk="$pk $DEP_BASE" ;;
        zlib)   pk="$pk $DEP_ZLIB" ;;
        sqlite) pk="$pk $DEP_SQLITE" ;;
        fuse)   pk="$pk $DEP_FUSE"; pre=$DEP_PRE ;;
        test)   pk="$pk $DEP_TEST" ;;
        esac
    done
    pk=$(echo $pk)   # (the spaces of empty groups)
    if [ "$FAMILY" = macos ]; then
        case " $* " in
        *" fuse "*) echo "(there is no FUSE 3 on macOS)"; return ;;
        *" zlib "*|*" sqlite "*) echo "xcode-select --install   (the macOS SDK has it)"; return ;;
        *" base "*) echo "xcode-select --install   (Apple clang; a newer GNU make: brew install make, then gmake)"; return ;;
        esac
    fi
    if [ -z "$FAMILY" ] || [ -z "$pk" ]; then
        echo "see \"Building from Source\" in README.md for the packages"
        return
    fi
    echo "${pre:+$pre && }$DEP_INSTALL $pk"
}

case $1 in
--probe)
    out=
    has_cxx17 && out="$out cxx"
    has_cc && out="$out cc"
    has_header zlib.h && out="$out zlib"
    has_bundled_zlib && out="$out zlib-bundled"
    is_git_checkout && out="$out zlib-git"
    has_header sqlite3.h && out="$out sqlite"
    has_fuse3 && out="$out fuse3"
    echo $out
    exit 0 ;;
--hint)
    shift
    hint "$@"
    exit 0 ;;
--add)   # the Makefile's "no - ..." for an optional feature left out
    if [ "$FAMILY" = macos ] && [ "$2" = fuse ]; then echo "there is no FUSE 3 on macOS"
    else echo "to add it: $(hint "$2")"; fi
    exit 0 ;;
esac

# ---- the report ----
required_missing=0
missing_groups=
optional_groups=

row() {   # row STATE NAME NOTE
    printf '  %-8s %-16s %s\n' "$1" "$2" "$3"
}

echo "MEGA65 C Compiler Suite: build dependencies (${DEP_NAME:-distribution not recognized})"
echo "Required:"
if has_cxx17; then
    row ok "C++17 compiler" "$CXX: $($CXX --version 2>/dev/null | head -n 1)"
elif have "$CXX"; then
    row MISSING "C++17 compiler" "$CXX is too old (C++17 with <filesystem>; the suite is built with GCC 11 and newer, and clang)"
    required_missing=1; missing_groups="$missing_groups base"
else
    row MISSING "C++17 compiler" "no '$CXX' (another compiler: make CXX=clang++)"
    required_missing=1; missing_groups="$missing_groups base"
fi

mv=$GNU_MAKE_VERSION
if [ -z "$mv" ]; then
    for m in gmake make; do
        if have $m && $m --version 2>/dev/null | head -n 1 | grep -q 'GNU Make'; then
            mv=$($m --version | head -n 1 | sed 's/^GNU Make \([0-9][0-9.]*\).*/\1/')
            break
        fi
    done
fi
if [ -z "$mv" ]; then
    row MISSING "GNU make" "no GNU make (make or gmake)"
    required_missing=1; missing_groups="$missing_groups base"
else
    major=${mv%%.*}; rest=${mv#*.}; minor=${rest%%.*}
    case $minor in ''|*[!0-9]*) minor=0 ;; esac
    if [ "$major" -gt $MIN_MAKE_MAJOR ] 2>/dev/null ||
       { [ "$major" -eq $MIN_MAKE_MAJOR ] 2>/dev/null && [ "$minor" -ge $MIN_MAKE_MINOR ]; }; then
        row ok "GNU make" "$mv"
    else
        row MISSING "GNU make" "$mv is older than $MIN_MAKE_MAJOR.$MIN_MAKE_MINOR"
        required_missing=1; missing_groups="$missing_groups base"
    fi
fi

# zlib (disk45's .gz and .zip images), as the Makefile chooses it: the bundled
# one (the submodule third_party/zlib) unless SYSTEM_ZLIB=1, else the
# system's; with neither, make fetches the submodule in a git checkout
if [ "$SYSTEM_ZLIB" = 1 ]; then
    if has_header zlib.h; then
        row ok "zlib" "the system's (SYSTEM_ZLIB=1)"
    else
        row MISSING "zlib" "SYSTEM_ZLIB=1, but the system's headers (zlib.h) were not found"
        required_missing=1; missing_groups="$missing_groups zlib"
    fi
elif has_bundled_zlib; then
    zv=$(sed -n 's/^#define ZLIB_VERSION "\(.*\)".*/\1/p' "$ROOT/$ZLIB_DIR/zlib.h")
    note="bundled: $ZLIB_DIR ($zv)"
    # (a "+": the submodule is not at the commit the tree pins, after a pull)
    if is_git_checkout && (cd "$ROOT" && git submodule status "$ZLIB_DIR") 2>/dev/null | grep -q '^+'; then
        note="$note, not the commit the tree pins: git submodule update $ZLIB_DIR"
    fi
    if has_cc; then
        row ok "zlib" "$note"
    else
        row MISSING "zlib" "$note, but no C compiler '$CC' to build it (make CC=..., or make SYSTEM_ZLIB=1)"
        required_missing=1; missing_groups="$missing_groups base"
    fi
elif has_header zlib.h; then
    if is_git_checkout; then row ok "zlib" "the system's ($ZLIB_DIR is empty; the bundled one: $ZLIB_FETCH)"
    else row ok "zlib" "the system's ($ZLIB_DIR is empty)"; fi
elif is_git_checkout; then
    row missing "zlib" "$ZLIB_DIR is empty and the system has none: make fetches it ($ZLIB_FETCH)"
else
    row MISSING "zlib" "$ZLIB_DIR is empty (zlib's source, github.com/madler/zlib, goes there) and the system has none"
    required_missing=1; missing_groups="$missing_groups zlib"
fi

echo "Optional (the build leaves the feature out without them):"
if has_header sqlite3.h; then
    row ok "SQLite" "disk45 catalog"
else
    row missing "SQLite" "disk45 is built without its catalog command"
    optional_groups="$optional_groups sqlite"
fi
if has_fuse3; then
    row ok "FUSE 3" "disk45 mount"
elif [ "$FAMILY" = macos ]; then
    row missing "FUSE 3" "not available on macOS: disk45 is built without mount"
else
    row missing "FUSE 3" "disk45 is built without its mount command (fuse3 through pkg-config)"
    optional_groups="$optional_groups fuse"
fi

test_missing=
for t in bash python3 xxd hexdump diff cmp zip unzip; do
    have $t || test_missing="$test_missing $t"
done
echo "For the tests (make test), not for the build:"
if [ -z "$test_missing" ]; then
    row ok "test tools" "bash python3 xxd hexdump diff cmp zip unzip"
else
    row missing "test tools" "not found:$test_missing"
fi

echo "At run time, each for one feature (not checked by the build):"
for t in "xemu-xmega65:make test-xemu, dbg45, --emulator options" "pandoc:make man" \
         "ffmpeg:pack45 on MP4 / M4A sound"; do
    have "${t%%:*}" && row ok "${t%%:*}" "${t#*:}" || row missing "${t%%:*}" "${t#*:}"
done
player=
for t in pw-play paplay aplay; do have $t && { player=$t; break; }; done
[ -n "$player" ] && row ok "$player" "tune45 play" ||
    row missing "audio player" "tune45 play (pw-play, paplay or aplay; --wav works without)"

echo
if [ -n "$missing_groups" ]; then
    echo "Required and missing; install with:"
    # shellcheck disable=SC2086
    echo "  $(hint $missing_groups)"
fi
if [ -n "$optional_groups" ]; then
    echo "Optional; to add what is left out:"
    # shellcheck disable=SC2086
    echo "  $(hint $optional_groups)"
fi
if [ -n "$test_missing" ] && [ "$FAMILY" != macos ]; then
    echo "For make test:"
    echo "  $(hint test)"
fi
if [ $required_missing -eq 0 ]; then
    echo "The build has what it needs: make"
else
    echo "The build cannot run until the required ones are installed."
fi
exit $required_missing
