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

# Create aliases for all tools
alias cc45="docker run --rm -v \$(pwd):/work $IMAGE"
alias ca45="docker run --rm -v \$(pwd):/work --entrypoint ca45 $IMAGE"
alias cp45="docker run --rm -v \$(pwd):/work --entrypoint cp45 $IMAGE"
alias nm45="docker run --rm -v \$(pwd):/work --entrypoint nm45 $IMAGE"
alias ln45="docker run --rm -v \$(pwd):/work --entrypoint ln45 $IMAGE"
alias ar45="docker run --rm -v \$(pwd):/work --entrypoint ar45 $IMAGE"
alias objdump45="docker run --rm -v \$(pwd):/work --entrypoint objdump45 $IMAGE"
alias disk45="docker run --rm -v \$(pwd):/work --entrypoint disk45 $IMAGE"

echo "✓ Docker aliases created for MEGA65 toolchain"
echo "  Available commands: cc45, ca45, cp45, nm45, ln45, ar45, objdump45, disk45"
echo ""
echo "Examples:"
echo "  cc45 hello.c -o hello.s       # Compile C to assembly"
echo "  ca45 hello.s -o hello.prg     # Assemble to binary"
echo "  ln45 *.o45 -o output.prg      # Link object files"
echo "  disk45 create game.d81        # Create disk image"
