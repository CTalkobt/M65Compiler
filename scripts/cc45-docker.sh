#!/bin/bash
# cc45-docker.sh - Compile C code using Docker
# Mounts toolchain (read-only) and work directory (read-write)
#
# Usage: cc45-docker.sh <source.c> [additional cc45 args...]
# Environment variables:
#   CC45_ROOT      - Path to m65compiler repo (default: PWD)
#   CC45_IMAGE     - Docker image name (default: m65compiler:builder)
#   WORK_DIR       - Working directory (default: PWD)
#   CC45_DOCKER    - Additional docker flags (default: empty)

set -euo pipefail

# Configuration
SOURCE_FILE="${1:?Usage: $0 <source.c> [cc45 args...]}"
shift || true
ADDITIONAL_ARGS=("$@")

# Environment defaults
CC45_ROOT="${CC45_ROOT:-.}"
CC45_IMAGE="${CC45_IMAGE:-m65compiler:builder}"
WORK_DIR="${WORK_DIR:-.}"
CC45_DOCKER="${CC45_DOCKER:-}"

# Resolve absolute paths
if [[ ! -f "$SOURCE_FILE" ]]; then
    echo "Error: Source file not found: $SOURCE_FILE" >&2
    exit 1
fi

SOURCE_ABS="$(cd "$(dirname "$SOURCE_FILE")" && pwd)/$(basename "$SOURCE_FILE")"
SOURCE_BASE="$(basename "$SOURCE_FILE")"
WORK_ABS="$(cd "$WORK_DIR" && pwd)"
CC45_ABS="$(cd "$CC45_ROOT" && pwd)"

# Validate that CC45_ROOT contains expected structure
if [[ ! -d "$CC45_ABS/lib" ]]; then
    echo "Error: CC45_ROOT does not contain lib/ directory: $CC45_ABS" >&2
    exit 1
fi

# Build output name (source.c -> source.s or source.prg depending on flags)
SOURCE_NAME="${SOURCE_BASE%.*}"
OUTPUT_NAME="${SOURCE_NAME}.s"

# Check for -c flag (object compilation)
for arg in "${ADDITIONAL_ARGS[@]}"; do
    if [[ "$arg" == "-c" ]]; then
        OUTPUT_NAME="${SOURCE_NAME}.o45"
        break
    fi
done

# Create work directory if needed
mkdir -p "$WORK_ABS"

# Run Docker
echo "Compiling: $SOURCE_BASE"
echo "  Toolchain: $CC45_ABS"
echo "  Work dir: $WORK_ABS"
echo "  Image: $CC45_IMAGE"

docker run --rm \
    -v "$CC45_ABS/lib:/opt/m65/lib:ro" \
    -v "$CC45_ABS/bin:/opt/m65/bin:ro" \
    -v "$WORK_ABS:/work:rw" \
    $CC45_DOCKER \
    "$CC45_IMAGE" \
    /usr/local/bin/cc45 \
        -I/opt/m65/lib/include \
        "/work/$SOURCE_BASE" \
        -o "/work/$OUTPUT_NAME" \
        "${ADDITIONAL_ARGS[@]}"

echo "✓ Output: $WORK_ABS/$OUTPUT_NAME"
