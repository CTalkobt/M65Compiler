# Dockerfile — MEGA65 C Compiler Suite (cc45)
#
# Build:   docker build -t cc45 .
# Usage:   docker run --rm -v $(pwd):/work cc45 cc45 -c /work/main.c -o /work/main.o45
#          docker run --rm cc45 cc45 --version
#          docker run --rm -v $(pwd):/work cc45 disk45 list /work/game.d81
# Catalog: docker run --rm -v $(pwd):/work -v catalog:/catalog cc45 \
#            disk45 catalog build /work --db /catalog/disk45.db
# Test:    docker run --rm cc45 make test

FROM ubuntu:22.04 AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    g++ make zlib1g-dev libfuse3-dev libsqlite3-dev pkg-config && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY . .

RUN make clean && make -j$(nproc) all && make -C lib -j$(nproc)

# --- Runtime image ---
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y --no-install-recommends \
    libstdc++6 make zlib1g libfuse3-3 libsqlite3-0 fuse3 && \
    rm -rf /var/lib/apt/lists/*

COPY --from=builder /build/bin/ /usr/local/bin/
COPY --from=builder /build/lib/build/ /usr/local/lib/cc45/
COPY --from=builder /build/lib/include/ /usr/local/include/cc45/

# Copy source tree for running tests inside container
COPY --from=builder /build/ /opt/cc45/

WORKDIR /opt/cc45

ENV CC45_LIB=/usr/local/lib/cc45
ENV CC45_INCLUDE=/usr/local/include/cc45

# Volume for disk45 catalog database persistence
VOLUME /catalog

ENTRYPOINT []
CMD ["cc45", "--version"]
