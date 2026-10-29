# Dockerfile — MEGA65 C Compiler Suite (cc45)
#
# Build:   docker build -t cc45 .
# Usage:   docker run --rm -v $(pwd):/work cc45 cc45 -c /work/main.c -o /work/main.o45
#          docker run --rm cc45 cc45 --version
#          docker run --rm -v $(pwd):/work cc45 disk45 list /work/game.d81
# Catalog: docker run --rm -v $(pwd):/work -v catalog:/catalog cc45 \
#            disk45 catalog build /work --db /catalog/disk45.db
# Test:    docker run --rm cc45 make test
# xemu:    the MEGA65 ROM is not included: mount yours to run programs and the
#          runtime tests (#99; xemu does not run as root: the user is m65)
#          docker run --rm -v /path/to/MEGA65.ROM:/rom/MEGA65.ROM:ro cc45 make test-xemu
#          docker run --rm -v /path/to/MEGA65.ROM:/rom/MEGA65.ROM:ro -v $(pwd):/work cc45 \
#              m65run -d 0xC0C0:8 /work/hello.prg

FROM ubuntu:22.04 AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    g++ make zlib1g-dev libfuse3-dev libsqlite3-dev pkg-config && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY . .

RUN make clean && make -j$(nproc) all && make -C lib -j$(nproc)

# --- xemu: CTalkobt/xemu cc45-dev (upstream dev with XEMU_PREF_DIR, the
# monitor's breakpoint list for dbg45, the hardware divider as the core
# computes it), pinned ---
FROM ubuntu:22.04 AS xemu

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential libsdl2-dev git python3 ca-certificates && \
    rm -rf /var/lib/apt/lists/*

ARG XEMU_REPO=https://github.com/CTalkobt/xemu.git
ARG XEMU_REF=06408fca8b16f3896b542836814fddb6ab4a0f02
RUN git clone --filter=blob:none "$XEMU_REPO" /xemu && cd /xemu && git checkout -q "$XEMU_REF" && \
    make -C targets/mega65 -j$(nproc) && test -x build/bin/xmega65.native

# --- Runtime image ---
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y --no-install-recommends \
    libstdc++6 make zlib1g libfuse3-3 libsqlite3-0 fuse3 \
    libsdl2-2.0-0 python3 xxd file && \
    rm -rf /var/lib/apt/lists/* && \
    useradd -m -u 1000 m65

COPY --from=builder /build/bin/ /usr/local/bin/
COPY --from=builder /build/lib/build/ /usr/local/lib/cc45/
COPY --from=builder /build/lib/include/ /usr/local/include/cc45/

# Copy source tree for running tests inside container (the tests write into
# it: the user's)
COPY --from=builder --chown=m65:m65 /build/ /opt/cc45/

# xemu (the wrapper sets up its preferences from the mounted ROM) and m65run
COPY --from=xemu /xemu/build/bin/xmega65.native /usr/local/libexec/xmega65
COPY src/Docker/xemu-wrapper.sh /usr/local/bin/xemu-xmega65
COPY src/Docker/m65run src/Docker/m65-entry.sh /usr/local/bin/

WORKDIR /opt/cc45

ENV CC45_LIB=/usr/local/lib/cc45
ENV CC45_INCLUDE=/usr/local/include/cc45
ENV XEMU_PREF_DIR=/tmp/xemu-pref

# Volume for disk45 catalog database persistence
VOLUME /catalog

USER m65

ENTRYPOINT ["m65-entry.sh"]
CMD ["cc45", "--version"]
