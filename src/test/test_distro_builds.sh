#!/bin/bash
# test_distro_builds.sh [DISTRO ...]: builds the tree (make all, make lib) in
# a fresh container of each distribution, with the packages README.md lists
# for it, so a compiler newer or stricter than the one in use (a missing
# <cstdint>, musl, clang) is found before a user finds it. Needs Docker.
#   DISTROS: ubuntu2204 ubuntu2404 debian12 debian13 fedora fedora-clang
#            arch alpine opensuse rocky9 (default: all)
#   DISTRO_JOBS (default 3) builds at once, each with make -j4;
#   DISTRO_TEST=1 runs make test after the build too.
# Builds the files git tracks, as they are in the working tree.
cd "$(dirname "$0")/../.." || exit 1

APT="export DEBIAN_FRONTEND=noninteractive; apt-get update && apt-get install -y --no-install-recommends build-essential pkg-config libsqlite3-dev zlib1g-dev libfuse3-dev git python3 xxd bsdextrautils zip unzip"
FEDORA="dnf install -y gcc-c++ make pkgconf-pkg-config sqlite-devel zlib-ng-compat-devel fuse3-devel git python3 xxd util-linux diffutils zip unzip"
declare -A IMAGE INSTALL CXXS
IMAGE[ubuntu2204]=ubuntu:22.04;        INSTALL[ubuntu2204]=$APT
IMAGE[ubuntu2404]=ubuntu:24.04;        INSTALL[ubuntu2404]=$APT
IMAGE[debian12]=debian:12;             INSTALL[debian12]=$APT
IMAGE[debian13]=debian:13;             INSTALL[debian13]=$APT
IMAGE[fedora]=fedora:latest;           INSTALL[fedora]=$FEDORA
IMAGE[fedora-clang]=fedora:latest;     INSTALL[fedora-clang]="$FEDORA clang"; CXXS[fedora-clang]=clang++
IMAGE[arch]=archlinux:latest;          INSTALL[arch]="pacman -Syu --noconfirm --needed base-devel sqlite zlib fuse3 git python tinyxxd util-linux diffutils zip unzip"
IMAGE[alpine]=alpine:latest;           INSTALL[alpine]="apk add build-base pkgconf sqlite-dev zlib-dev fuse3-dev git python3 linux-headers bash xxd hexdump zip unzip"
IMAGE[opensuse]=opensuse/tumbleweed;   INSTALL[opensuse]="zypper -n in gcc-c++ make pkgconf sqlite3-devel zlib-devel fuse3-devel git python3 xxd util-linux diffutils findutils gawk zip unzip gzip tar"
IMAGE[rocky9]=rockylinux:9;            INSTALL[rocky9]="dnf install -y dnf-plugins-core && dnf config-manager --set-enabled crb && dnf install -y gcc-c++ make pkgconf-pkg-config sqlite-devel zlib-devel fuse3-devel git python3 vim-common util-linux diffutils zip unzip"
ALL="ubuntu2204 ubuntu2404 debian12 debian13 fedora fedora-clang arch alpine opensuse rocky9"

distros=("$@"); [ ${#distros[@]} -eq 0 ] && read -ra distros <<< "$ALL"
for d in "${distros[@]}"; do
    [ -n "${IMAGE[$d]}" ] || { echo "unknown distribution '$d' (one of: $ALL)"; exit 2; }
done
command -v docker >/dev/null || { echo "docker is needed"; exit 2; }

work=$(mktemp -d); trap 'rm -rf "$work"' EXIT
git ls-files -z | tar --null -T - -czf "$work/src.tgz"

build() {   # build NAME: the log in $work/NAME.log, "ok"/"FAILED" in $work/NAME.result
    local d=$1 cxx=${CXXS[$1]:-g++}
    docker run --rm -v "$work/src.tgz:/src.tgz:ro" "${IMAGE[$d]}" sh -c "
        ( ${INSTALL[$d]} ) > /tmp/install.log 2>&1 || { tail -20 /tmp/install.log; echo 'package install failed'; exit 1; }
        mkdir /build && cd /build && tar xzf /src.tgz || exit 1
        echo \"compiler: \$($cxx --version | head -1)\"
        make -j4 CXX=$cxx all > /tmp/all.log 2>&1 || { grep -E ':[0-9]+:[0-9]+: (fatal )?error' /tmp/all.log | sort -u | head -40; tail -5 /tmp/all.log; echo 'make all failed'; exit 1; }
        make lib > /tmp/lib.log 2>&1 || { grep -E 'error' /tmp/lib.log | head -20; echo 'make lib failed'; exit 1; }
        echo \"tools: \$(ls bin | tr '\n' ' ')\"
        if [ '${DISTRO_TEST:-0}' = 1 ]; then
            make test > /tmp/test.log 2>&1 || { grep -E 'FAIL|command not found|rror:' /tmp/test.log | head -30; tail -5 /tmp/test.log; echo 'make test failed'; exit 1; }
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
    printf '%-14s %-22s %s  (%s)\n' "$d" "${IMAGE[$d]}" "$r" "$(grep -m1 '^compiler:' "$work/$d.log" | cut -c11-)"
    if [ "$r" != ok ]; then failed=$((failed + 1)); sed 's/^/    /' "$work/$d.log" | tail -45; fi
done
echo "Distribution builds: $(( ${#distros[@]} - failed )) passed, $failed failed"
[ $failed -eq 0 ]
