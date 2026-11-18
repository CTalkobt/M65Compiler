# Alpine image of the MEGA65 C Compiler Suite (make docker)
#
# Build:  docker build -f src/Docker/Dockerfile -t mega65-cc45 .
# Use:    docker run --rm -v "$PWD":/work mega65-cc45 hello.c -o hello.prg
#         docker run --rm -v "$PWD":/work --entrypoint disk45 mega65-cc45 list game.d81
#
# The suite is installed with `make install PREFIX=/app`: tools in /app/bin,
# libraries in /app/lib/cc45, headers in /app/include/cc45 — the installed
# layout the tools find relative to their own location (no environment
# variables needed).

# --- Build ---
FROM alpine:3.20 AS builder

RUN apk add --no-cache g++ make fuse3-dev sqlite-dev pkgconf

WORKDIR /build
COPY . .

# zlib is the bundled one, the submodule third_party/zlib (#620): the image
# is built from a checkout that has it
RUN test -f third_party/zlib/zlib.h || { \
    echo "third_party/zlib is empty: run 'git submodule update --init third_party/zlib' in the checkout, then build the image again"; exit 1; }

RUN make clean && make -j"$(nproc)" all && make -C lib -j"$(nproc)" && \
    make install PREFIX=/app

# --- Runtime ---
FROM alpine:3.20

RUN apk add --no-cache libstdc++ libgcc fuse3 sqlite-libs

COPY --from=builder /app /app
ENV PATH="/app/bin:$PATH"

VOLUME /work
WORKDIR /work

ENTRYPOINT ["cc45"]
CMD ["--help"]
