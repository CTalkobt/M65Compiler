#!/bin/bash
# package.sh KIND [OUTDIR]: builds the suite and packs it as a release
# archive (#621), m65compiler-<version>-<os>-<arch>.tar.gz, in OUTDIR
# (default build/release). The archive holds the tree `make install PREFIX=`
# writes (bin/, lib/cc45/, include/cc45/, ...), which the tools find relative
# to their own location: unpack it anywhere and run bin/cc45.
#
#   linux-static  in an Alpine container (needs Docker): statically linked
#                 with musl, so the binaries run on any Linux of the
#                 machine's architecture. zlib is the bundled one
#                 (third_party/zlib). disk45 has the catalog (SQLite)
#                 and `mount` (libfuse 3), both linked in; mounting needs
#                 the system's /dev/fuse and fusermount3, which disk45
#                 asks for when `mount` is used.
#   native        with this machine's compiler and libraries (macOS: run it
#                 with GNU make first on the PATH as `make`, or MAKE=gmake).
#
#   VERSION       the archive's version (default: SUITE_VERSION of
#                 include/Version.hpp; the release workflow passes the tag)
#   JOBS          make -j (default 4)
#
# The files git tracks are built, as they are in the working tree, with the
# bundled zlib (the submodule third_party/zlib, which must be checked out:
# a release has the zlib the tree pins, not the build machine's). Each
# archive is checked before it is kept: unpacked in a directory of its own,
# its cc45 builds a program from another directory.
set -e
cd "$(dirname "$0")/../.."

kind=$1
out=${2:-build/release}
[ "$kind" = linux-static ] || [ "$kind" = native ] || { echo "usage: package.sh linux-static|native [OUTDIR]" >&2; exit 2; }

version=${VERSION:-$(sed -n 's/^#define SUITE_VERSION "\(.*\)"/\1/p' include/Version.hpp)}
version=${version#v}
[ -n "$version" ] || { echo "package.sh: no version (VERSION, or SUITE_VERSION in include/Version.hpp)" >&2; exit 1; }
hash=$(git rev-parse --short HEAD 2>/dev/null || echo unknown)
jobs=${JOBS:-4}
mkdir -p "$out"
out=$(cd "$out" && pwd)

arch=$(uname -m)
case "$arch" in amd64) arch=x86_64 ;; aarch64) arch=arm64 ;; esac
case "$(uname -s)" in Darwin) os=macos ;; Linux) os=linux ;; *) os=$(uname -s | tr 'A-Z' 'a-z') ;; esac

[ -f third_party/zlib/zlib.h ] && [ -f third_party/zlib/LICENSE ] ||
    { echo "package.sh: third_party/zlib is empty: git submodule update --init third_party/zlib" >&2; exit 1; }

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
# (a submodule's files are in the list only with --recurse-submodules)
git ls-files -z --recurse-submodules | tar --null -T - -cf "$work/src.tar"

# what goes beside the installed tree: the licences (tune45 contains
# libresidfp, GPL-2+; its source is this repository at the release's tag;
# disk45 contains zlib, whose LICENSE is in its submodule and whose
# provenance is third_party/zlib.PROVENANCE.md) and a word on what the
# archive is
notes() {   # notes STAGE NAME WHAT
    mkdir -p "$1/share/doc/m65compiler"
    cp LICENSE "$1/share/doc/m65compiler/LICENSE"
    for t in third_party/*/; do
        n=$(basename "$t")
        for f in COPYING LICENSE PROVENANCE.md; do
            [ -f "$t$f" ] && cp "$t$f" "$1/share/doc/m65compiler/$n-$f"
        done
        [ -f "third_party/$n.PROVENANCE.md" ] && cp "third_party/$n.PROVENANCE.md" "$1/share/doc/m65compiler/$n-PROVENANCE.md"
    done
    [ -f "$1/share/doc/m65compiler/zlib-LICENSE" ] || { echo "package.sh: zlib's LICENSE is not in the archive" >&2; exit 1; }
    cat > "$1/README.txt" <<EOF
MEGA65 C Compiler Suite $version ($2, built from $hash)
https://github.com/CTalkobt/M65Compiler

$3

Unpack anywhere and run the tools from bin/ (or put bin/ on the PATH): they
find the headers (include/cc45) and libraries (lib/cc45) relative to their
own location.

    bin/cc45 -O2 hello.c -o hello.prg

Each tool prints its options with -?; the reference pages are in the
repository's doc/bin/. Licences: share/doc/m65compiler/ (tune45 contains
libresidfp, GPL-2+; disk45 contains zlib; the source of this build is the
repository at tag v$version, with its submodule third_party/zlib).
EOF
}

# the installed tree builds a program, started from another directory
check() {   # check ARCHIVE NAME
    local t="$work/check"
    rm -rf "$t"; mkdir -p "$t/unpacked" "$t/elsewhere"
    tar -xzf "$1" -C "$t/unpacked"
    printf '#include <stdio.h>\nint main(void) { puts("hello"); return 0; }\n' > "$t/elsewhere/hello.c"
    ( cd "$t/elsewhere" && "$t/unpacked/$2/bin/cc45" -O2 hello.c -o hello.prg ) && [ -s "$t/elsewhere/hello.prg" ] ||
        { echo "package.sh: the unpacked cc45 did not build hello.prg" >&2; exit 1; }
    "$t/unpacked/$2/bin/disk45" create "$t/elsewhere/t.d81" -n TEST > /dev/null &&
        "$t/unpacked/$2/bin/disk45" add "$t/elsewhere/t.d81" "$t/elsewhere/hello.prg" hello > /dev/null ||
        { echo "package.sh: the unpacked disk45 did not make a disk image" >&2; exit 1; }
    # (the zlib linked in: a compressed image written and read back)
    "$t/unpacked/$2/bin/disk45" create "$t/elsewhere/z.d81.gz" -n TEST > /dev/null &&
        "$t/unpacked/$2/bin/disk45" add "$t/elsewhere/z.d81.gz" "$t/elsewhere/hello.prg" ZHELLO > /dev/null &&
        "$t/unpacked/$2/bin/disk45" list "$t/elsewhere/z.d81.gz" | grep -q ZHELLO && gzip -t "$t/elsewhere/z.d81.gz" ||
        { echo "package.sh: the unpacked disk45 did not write and read a .gz disk image" >&2; exit 1; }
    if [ "$kind" = linux-static ]; then
        "$t/unpacked/$2/bin/disk45" 2>&1 | grep -q 'disk45 mount <image>' && "$t/unpacked/$2/bin/disk45" 2>&1 | grep -q 'disk45 catalog build' ||
            { echo "package.sh: the static disk45 lacks mount or catalog" >&2; exit 1; }
    fi
    echo "checked: $(basename "$1") builds hello.prg ($(wc -c < "$t/elsewhere/hello.prg" | tr -d ' ') bytes) and a disk image"
}

if [ "$kind" = linux-static ]; then
    command -v docker > /dev/null || { echo "package.sh: linux-static needs docker" >&2; exit 2; }
    name="m65compiler-$version-linux-$arch"
    mkdir -p "$work/stage"
    # (-static through EXTRA_CXXFLAGS, which every link line has; libfuse3.a
    # is linked into disk45, with what pkg-config --static says it needs)
    docker run --rm -v "$work:/work" alpine:3.20 sh -c "
        set -e
        apk add --no-cache g++ make binutils sqlite-dev sqlite-static fuse3-dev fuse3-static pkgconf > /tmp/apk.log 2>&1 || { tail -20 /tmp/apk.log; exit 1; }
        mkdir /build && cd /build && tar -xf /work/src.tar
        fuse=\$(pkg-config --static --libs fuse3)
        make -j$jobs GIT_HASH=$hash EXTRA_CXXFLAGS=-static FUSE3_LIBS=\"\$fuse\" all > /work/all.log 2>&1 || { grep -E ':[0-9]+:[0-9]+: (fatal )?error|ld: |undefined reference' /work/all.log | sort -u | head -40; tail -5 /work/all.log; echo 'make all failed'; exit 1; }
        grep -q '^  zlib.*: bundled' /work/all.log || { echo 'the build did not use the bundled zlib'; exit 1; }
        make GIT_HASH=$hash lib > /work/lib.log 2>&1 || { tail -20 /work/lib.log; echo 'make lib failed'; exit 1; }
        make GIT_HASH=$hash EXTRA_CXXFLAGS=-static FUSE3_LIBS=\"\$fuse\" install PREFIX=/work/stage/$name > /work/install.log 2>&1 || { tail -20 /work/install.log; echo 'make install failed'; exit 1; }
        strip /work/stage/$name/bin/* 2> /dev/null || true
        chown -R $(id -u):$(id -g) /work/stage /work/*.log
    "
    for b in "$work/stage/$name"/bin/*; do
        [ -L "$b" ] && continue
        # (statically linked: no interpreter, no shared libraries)
        if readelf -l "$b" 2> /dev/null | grep -q 'INTERP\|NEEDED'; then echo "package.sh: $(basename "$b") is not statically linked" >&2; exit 1; fi
    done
    notes "$work/stage/$name" "Linux $arch, statically linked" \
"The binaries are statically linked (musl): they run on any Linux of this
architecture with no library to install. disk45 has its catalog and
'mount'; mounting uses the system's FUSE (/dev/fuse and the fusermount3
helper of the fuse3 package), and says so where they are missing. Linked
in: musl (MIT), libstdc++ (GPL-3 with the runtime exception), zlib (the
zlib licence), SQLite (public domain), libfuse 3 (LGPL-2.1: relink from the
source to change it)."
else
    name="m65compiler-$version-$os-$arch"
    make=${MAKE:-make}
    mkdir -p "$work/build" "$work/stage"
    tar -xf "$work/src.tar" -C "$work/build"
    (
        cd "$work/build"
        $make -j"$jobs" GIT_HASH="$hash" all > "$work/all.log" 2>&1 || { grep -E ':[0-9]+:[0-9]+: (fatal )?error' "$work/all.log" | sort -u | head -40; tail -5 "$work/all.log"; echo 'make all failed'; exit 1; }
        grep -q '^  zlib.*: bundled' "$work/all.log" || { echo 'the build did not use the bundled zlib'; exit 1; }
        $make GIT_HASH="$hash" lib > "$work/lib.log" 2>&1 || { tail -20 "$work/lib.log"; echo 'make lib failed'; exit 1; }
        $make GIT_HASH="$hash" install PREFIX="$work/stage/$name" > "$work/install.log" 2>&1 || { tail -20 "$work/install.log"; echo 'make install failed'; exit 1; }
    )
    if [ "$os" = macos ]; then
        what="Built with Apple clang; the binaries use only the system's own libraries
(SQLite is part of macOS; zlib is linked in). disk45 has no 'mount' (no
FUSE)."
    else
        what="Built with this machine's compiler and linked with its shared libraries
(SQLite, and FUSE 3 when it was found): they must be installed where the
binaries run. zlib is linked in."
    fi
    notes "$work/stage/$name" "$os $arch" "$what"
fi

tar -czf "$out/$name.tar.gz" -C "$work/stage" "$name"
check "$out/$name.tar.gz" "$name"
echo "$out/$name.tar.gz ($(wc -c < "$out/$name.tar.gz" | tr -d ' ') bytes)"
