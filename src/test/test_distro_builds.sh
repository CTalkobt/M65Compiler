#!/bin/bash
# test_distro_builds.sh [DISTRO ...]: builds the tree (make all, make lib) in
# a fresh container of each distribution, with the packages README.md lists
# for it, so a compiler newer or stricter than the one in use (a missing
# <cstdint>, musl, clang) is found before a user finds it. Needs Docker.
#   DISTROS: ubuntu2204 ubuntu2404 debian12 debian13 fedora fedora-clang
#            arch alpine opensuse rocky9 (default: all), and alpine-min,
#            debian13-syszlib
#   DISTRO_JOBS (default 3) builds at once, each with make -j4;
#   DISTRO_TEST=1 runs make test after the build too.
#   DISTRO_EXAMPLES=1 builds the examples too (make -C examples all).
# The packages are src/deps/deps_table.sh's (#619), the list `make
# check-deps` prints: the build's (compiler, make, SQLite, FUSE 3), and
# the tests' only with DISTRO_TEST=1 or DISTRO_EXAMPLES=1, so a build here is
# a build with what README.md's "to build" line installs. No zlib package:
# the build's is the bundled one (the submodule third_party/zlib, #620), and
# disk45 must not be linked with a libz; it writes and reads a .gz and a .zip
# image. <distro>-min installs the required ones alone (the compiler and
# make: no SQLite, no FUSE 3, no pkg-config): the build must still give every
# tool, disk45 without catalog and mount. <distro>-syszlib installs the
# distribution's zlib too and builds with SYSTEM_ZLIB=1: disk45 must be
# linked with that libz.
# Builds the files git tracks (the submodule's too), as they are in the
# working tree.
cd "$(dirname "$0")/../.." || exit 1
. src/deps/deps_table.sh

# distribution -> image, family of deps_table.sh, packages the container
# needs beyond the family's, compiler
declare -A IMAGE FAMILY CXXS EXTRA
IMAGE[ubuntu2204]=ubuntu:22.04;        FAMILY[ubuntu2204]=debian
IMAGE[ubuntu2404]=ubuntu:24.04;        FAMILY[ubuntu2404]=debian
IMAGE[debian12]=debian:12;             FAMILY[debian12]=debian
IMAGE[debian13]=debian:13;             FAMILY[debian13]=debian
IMAGE[fedora]=fedora:latest;           FAMILY[fedora]=fedora
IMAGE[fedora-clang]=fedora:latest;     FAMILY[fedora-clang]=fedora; EXTRA[fedora-clang]=clang; CXXS[fedora-clang]=clang++
IMAGE[arch]=archlinux:latest;          FAMILY[arch]=arch
IMAGE[alpine]=alpine:latest;           FAMILY[alpine]=alpine
IMAGE[alpine-min]=alpine:latest;       FAMILY[alpine-min]=alpine
IMAGE[debian13-syszlib]=debian:13;     FAMILY[debian13-syszlib]=debian
IMAGE[opensuse]=opensuse/tumbleweed;   FAMILY[opensuse]=suse;       EXTRA[opensuse]="gzip tar"
IMAGE[rocky9]=rockylinux:9;            FAMILY[rocky9]=rhel
ALL="ubuntu2204 ubuntu2404 debian12 debian13 fedora fedora-clang arch alpine opensuse rocky9"

# install_cmd DISTRO: the command that installs its packages in the container
# (the family's install command as root, without questions)
install_cmd() {
    local d=$1 pk
    deps_family "${FAMILY[$d]}"
    pk="$DEP_BASE"
    case $d in *-min) ;; *) pk="$pk $DEP_SQLITE $DEP_FUSE" ;; esac
    case $d in *-syszlib) pk="$pk $DEP_ZLIB" ;; esac
    if [ "${DISTRO_TEST:-0}" = 1 ] || [ "${DISTRO_EXAMPLES:-0}" = 1 ]; then pk="$pk $DEP_TEST"; fi
    pk="$pk ${EXTRA[$d]}"
    case ${FAMILY[$d]} in
    debian) echo "export DEBIAN_FRONTEND=noninteractive; apt-get update && apt-get install -y --no-install-recommends $pk" ;;
    fedora) echo "dnf install -y $pk" ;;
    rhel)   echo "dnf install -y dnf-plugins-core && dnf config-manager --set-enabled crb && dnf install -y $pk" ;;
    arch)   echo "pacman -Syu --noconfirm --needed $pk" ;;
    suse)   echo "zypper -n in $pk" ;;
    alpine) echo "apk add $pk" ;;
    esac
}
if [ "$1" = --show ]; then   # the install command of each distribution, nothing run
    shift
    for d in "${@:-$ALL alpine-min debian13-syszlib}"; do for e in $d; do printf '%-17s %s\n' "$e" "$(install_cmd "$e")"; done; done
    exit 0
fi

distros=("$@"); [ ${#distros[@]} -eq 0 ] && read -ra distros <<< "$ALL"
for d in "${distros[@]}"; do
    [ -n "${IMAGE[$d]}" ] || { echo "unknown distribution '$d' (one of: $ALL alpine-min debian13-syszlib)"; exit 2; }
done
command -v docker >/dev/null || { echo "docker is needed"; exit 2; }

# the bundled zlib is a submodule: its files are in the tree only when it is
# checked out, and in the list only with --recurse-submodules
[ -f third_party/zlib/zlib.h ] || { echo "third_party/zlib is empty: git submodule update --init third_party/zlib"; exit 2; }
work=$(mktemp -d); trap 'rm -rf "$work"' EXIT
git ls-files -z --recurse-submodules | tar --null -T - -czf "$work/src.tgz"

build() {   # build NAME: the log in $work/NAME.log, "ok"/"FAILED" in $work/NAME.result
    local d=$1 cxx=${CXXS[$1]:-g++} zlib=
    case $d in *-syszlib) zlib=SYSTEM_ZLIB=1 ;; esac
    docker run --rm -v "$work/src.tgz:/src.tgz:ro" "${IMAGE[$d]}" sh -c "
        ( $(install_cmd "$d") ) > /tmp/install.log 2>&1 || { tail -20 /tmp/install.log; echo 'package install failed'; exit 1; }
        mkdir /build && cd /build && tar xzf /src.tgz || exit 1
        echo \"compiler: \$($cxx --version | head -1)\"
        make -j4 CXX=$cxx $zlib all > /tmp/all.log 2>&1 || { grep -E ':[0-9]+:[0-9]+: (fatal )?error' /tmp/all.log | sort -u | head -40; tail -5 /tmp/all.log; echo 'make all failed'; exit 1; }
        make $zlib lib > /tmp/lib.log 2>&1 || { grep -E 'error' /tmp/lib.log | head -20; echo 'make lib failed'; exit 1; }
        echo \"tools: \$(ls bin | tr '\n' ' ')\"
        for t in cc45 cp45 cl45 ca45 al45 af45 ln45 ar45 nm45 objdump45 basic45 disk45 asmCvt45 pack45 font45 tune45 deploy45 dbg45; do
            [ -x bin/\$t ] || { echo \"bin/\$t was not built\"; exit 1; }
        done
        grep -m1 '^  zlib' /tmp/all.log
        if [ -n '$zlib' ]; then
            ldd bin/disk45 | grep -q 'libz\.so' || { echo 'SYSTEM_ZLIB=1: disk45 is not linked with the system libz'; exit 1; }
            echo 'disk45 is linked with the system libz'
        else
            ! ldd bin/disk45 | grep -q 'libz\.so' || { echo 'disk45 is linked with a system libz, not the bundled zlib'; exit 1; }
        fi
        printf 'ab' > /tmp/p.prg
        for i in /tmp/t.d81.gz /tmp/t.d81.zip; do
            bin/disk45 create \$i -n T > /dev/null 2>&1 && bin/disk45 add \$i /tmp/p.prg P > /dev/null 2>&1 &&
                bin/disk45 list \$i | grep -q '\"P\"' || { echo \"disk45: \$i was not written and read back\"; exit 1; }
        done
        ! command -v gzip > /dev/null || gzip -t /tmp/t.d81.gz || { echo 'gzip -t: disk45 wrote a bad .gz'; exit 1; }
        echo 'disk45 wrote and read a .gz and a .zip image'
        case $d in *-min)
            bin/disk45 catalog list 2>&1 | grep -q 'built without SQLite' || { echo 'disk45 catalog: no \"built without SQLite\" message'; exit 1; }
            echo 'disk45 without catalog, as it says' ;;
        esac
        if [ '${DISTRO_EXAMPLES:-0}' = 1 ]; then
            make -C examples all > /tmp/examples.log 2>&1 || { grep -E 'rror' /tmp/examples.log | head -20; tail -5 /tmp/examples.log; echo 'make -C examples all failed'; exit 1; }
            echo 'examples built'
        fi
        if [ '${DISTRO_TEST:-0}' = 1 ]; then
            make $zlib test > /tmp/test.log 2>&1 || { grep -E 'FAIL|command not found|rror:' /tmp/test.log | head -30; tail -5 /tmp/test.log; echo 'make test failed'; exit 1; }
            grep -E 'Summary|Example tests' /tmp/test.log
        fi
    " > "$work/$d.log" 2>&1 && echo ok > "$work/$d.result" || echo FAILED > "$work/$d.result"
}

jobs=${DISTRO_JOBS:-3}
for d in "${distros[@]}"; do
    while [ "$(jobs -rp | wc -l)" -ge "$jobs" ]; do wait -n; done
    build "$d" &
done
wait

failed=0
for d in "${distros[@]}"; do
    r=$(cat "$work/$d.result" 2>/dev/null || echo FAILED)
    printf '%-17s %-22s %s  (%s)\n' "$d" "${IMAGE[$d]}" "$r" "$(grep -m1 '^compiler:' "$work/$d.log" | cut -c11-)"
    grep -q '^examples built' "$work/$d.log" 2>/dev/null && echo "    examples built"
    grep -E '^  zlib|libz' "$work/$d.log" 2>/dev/null | sed 's/^ */    /'
    if [ "$r" != ok ]; then failed=$((failed + 1)); sed 's/^/    /' "$work/$d.log" | tail -45; fi
done
echo "Distribution builds: $(( ${#distros[@]} - failed )) passed, $failed failed"
[ $failed -eq 0 ]
