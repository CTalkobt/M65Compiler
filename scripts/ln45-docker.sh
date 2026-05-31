#!/bin/bash
# ln45-docker.sh - Link object files using Docker
# Mounts toolchain (read-only) and work directory (read-write)
#
# Usage: ln45-docker.sh [ln45 args...]
# Environment variables:
#   CC45_ROOT      - Path to m65compiler repo (default: PWD)
#   CC45_IMAGE     - Docker image name (default: m65compiler:builder)
#   WORK_DIR       - Working directory (default: PWD)
#   CC45_DOCKER    - Additional docker flags (default: empty)

set -euo pipefail

# Environment defaults
CC45_ROOT="${CC45_ROOT:-.}"
CC45_IMAGE="${CC45_IMAGE:-m65compiler:builder}"
WORK_DIR="${WORK_DIR:-.}"
CC45_DOCKER="${CC45_DOCKER:-}"

# Resolve absolute paths
WORK_ABS="$(cd "$WORK_DIR" && pwd)"
CC45_ABS="$(cd "$CC45_ROOT" && pwd)"

# Validate that CC45_ROOT contains expected structure
if [[ ! -d "$CC45_ABS/lib" ]]; then
    echo "Error: CC45_ROOT does not contain lib/ directory: $CC45_ABS" >&2
    exit 1
fi

# Create work directory if needed
mkdir -p "$WORK_ABS"

# Run Docker
echo "Linking with ln45"
echo "  Toolchain: $CC45_ABS"
echo "  Work dir: $WORK_ABS"
echo "  Image: $CC45_IMAGE"

docker run --rm \
    -v "$CC45_ABS/lib:/opt/m65/lib:ro" \
    -v "$CC45_ABS/bin:/opt/m65/bin:ro" \
    -v "$WORK_ABS:/work:rw" \
    $CC45_DOCKER \
    "$CC45_IMAGE" \
    /usr/local/bin/ln45 \
        -L/opt/m65/lib \
        "$@"

echo "✓ Linking complete"
