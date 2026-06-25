# Multi-stage build for MEGA65 C Compiler Suite using Alpine Linux
# Stage 1: Build the compiler suite
FROM alpine:3.20 AS builder

# Set working directory
WORKDIR /build

# Install build dependencies
RUN apk add --no-cache \
    g++ \
    make \
    git \
    zlib-dev

# Copy source code
COPY . .

# Build the compiler suite and libraries
RUN make clean && make all lib

# Stage 2: Runtime image with just the binaries
FROM alpine:3.20 AS runtime

# Install runtime dependencies and full glibc compatibility
# Note: libc6-compat provides glibc, libstdc++ provides C++ runtime libraries
RUN apk add --no-cache zlib libc6-compat libstdc++

# Create app directory structure
WORKDIR /app
RUN mkdir -p /app/bin /app/lib/cc45 /app/lib/cc45/include /app/lib/cc45/stdlib /app/lib/cc45/stdlib_zp

# Copy compiled binaries from builder
COPY --from=builder /build/bin/cc45 /app/bin/
COPY --from=builder /build/bin/ca45 /app/bin/
COPY --from=builder /build/bin/nm45 /app/bin/
COPY --from=builder /build/bin/ln45 /app/bin/
COPY --from=builder /build/bin/ar45 /app/bin/
COPY --from=builder /build/bin/objdump45 /app/bin/
COPY --from=builder /build/bin/disk45 /app/bin/
COPY --from=builder /build/bin/cp45 /app/bin/

# Copy library headers
COPY --from=builder /build/lib/include/* /app/lib/cc45/include/

# Copy startup files and stdlib archives
COPY --from=builder /build/lib/crt*.s /app/lib/cc45/
COPY --from=builder /build/lib/build/*.lib /app/lib/cc45/
COPY --from=builder /build/lib/build/*.o45 /app/lib/cc45/

# Create convenience symlinks in /usr/local/bin
RUN ln -s /app/bin/cc45 /usr/local/bin/cc45 && \
    ln -s /app/bin/ca45 /usr/local/bin/ca45 && \
    ln -s /app/bin/cp45 /usr/local/bin/cp45 && \
    ln -s /app/bin/nm45 /usr/local/bin/nm45 && \
    ln -s /app/bin/ln45 /usr/local/bin/ln45 && \
    ln -s /app/bin/ar45 /usr/local/bin/ar45 && \
    ln -s /app/bin/objdump45 /usr/local/bin/objdump45 && \
    ln -s /app/bin/disk45 /usr/local/bin/disk45

# Set environment variable for library path
ENV CC45_LIB_PATH=/app/lib/cc45
ENV PATH="/app/bin:$PATH"

# Create /work directory for volume mounts
WORKDIR /work

# Default to cc45 compiler when no entrypoint override
ENTRYPOINT ["/app/bin/cc45"]
CMD ["-?"]
