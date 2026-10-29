#!/bin/bash
# Docker Aliases for MEGA65 C Compiler Suite
# Source this file to create convenient command aliases:
#   source docker-aliases.sh
#
# This will create aliases for all tools so you can use them like normal commands.
# Make sure you have the mega65-cc45 Docker image built first.

# Image name (adjust if you built with a different tag)
IMAGE="mega65-cc45:latest"

# Check if image exists
if ! docker images | grep -q "^mega65-cc45"; then
    echo "Error: Docker image 'mega65-cc45' not found."
    echo "Build it first with: docker build -t mega65-cc45:latest ."
    return 1
fi

# Create aliases for all tools (as the calling user: the files they write in
# the current directory are yours, not root's)
_M65RUN="docker run --rm --user \$(id -u):\$(id -g) -v \$(pwd):/work"
alias cc45="$_M65RUN $IMAGE"
for _t in ca45 cp45 nm45 ln45 ar45 objdump45 disk45 basic45 asmCvt45; do
    alias $_t="$_M65RUN --entrypoint $_t $IMAGE"
done
unset _t

echo "✓ Docker aliases created for MEGA65 toolchain"
echo "  Available commands: cc45, ca45, cp45, nm45, ln45, ar45, objdump45, disk45, basic45, asmCvt45"
echo ""
echo "Examples:"
echo "  cc45 hello.c -o hello.s       # Compile C to assembly"
echo "  ca45 hello.s -o hello.prg     # Assemble to binary"
echo "  ln45 *.o45 -o output.prg      # Link object files"
echo "  disk45 create game.d81        # Create disk image"
