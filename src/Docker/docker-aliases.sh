#!/bin/bash
# Docker Aliases for MEGA65 C Compiler Suite
# Source this file to create convenient command aliases:
#   source docker-aliases.sh
#
# This will create aliases for all tools so you can use them like normal commands.
# Make sure you have the mega65-cc45 Docker image built first.

# The image: mega65-cc45:latest (make docker), or the one M65_IMAGE names,
# e.g. the published one:
#   M65_IMAGE=ghcr.io/ctalkobt/m65compiler source docker-aliases.sh
IMAGE="${M65_IMAGE:-mega65-cc45:latest}"

if ! docker image inspect "$IMAGE" >/dev/null 2>&1; then
    echo "Error: Docker image '$IMAGE' not found."
    echo "Build it with 'make docker' (docker build -f src/Docker/Dockerfile -t mega65-cc45:latest .)"
    echo "or fetch the published one: docker pull ghcr.io/ctalkobt/m65compiler"
    return 1
fi

# Create aliases for all tools (as the calling user: the files they write in
# the current directory are yours, not root's)
_M65RUN="docker run --rm --user \$(id -u):\$(id -g) -v \$(pwd):/work"
_M65TOOLS="cc45 cp45 cl45 ca45 al45 ln45 ar45 nm45 objdump45 disk45 basic45 asmCvt45 pack45 font45 tune45 deploy45 dbg45"
for _t in $_M65TOOLS; do
    alias $_t="$_M65RUN --entrypoint $_t $IMAGE"
done

echo "✓ Docker aliases created for MEGA65 toolchain ($IMAGE)"
echo "  Available commands: $_M65TOOLS"
unset _t _M65TOOLS
echo ""
echo "Examples:"
echo "  cc45 -O2 hello.c -o hello.prg   # Compile and link a program"
echo "  cc45 -O2 -S hello.c -o hello.s45  # Compile C to assembly"
echo "  ln45 -basic *.o45 -l c45.lib -o output.prg  # Link object files"
echo "  disk45 create game.d81          # Create disk image"
