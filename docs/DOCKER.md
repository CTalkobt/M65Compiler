# Docker Build & Compilation Environment

The MEGA65 C Compiler toolchain is available as a Docker container for reproducible builds across macOS, Linux, and Windows (WSL2).

**Current status:** v1.1 (implementation in progress)

## Quick Start

### 1. Build the Docker Image

```bash
# Build the compiler toolchain image
docker build -f Dockerfile.build -t m65compiler:builder .
```

This creates a ~120MB image with:
- cc45 (C compiler)
- ca45 (assembler)
- ln45 (linker)
- ar45 (archiver)
- nm45, objdump45, cp45 (utilities)
- stdlib libraries and headers

**Build time:** ~2.5 minutes (first build), ~30 seconds (cached)

### 2. Compile C Code

#### Option A: Using wrapper script (recommended)

```bash
# Simple compilation
./scripts/cc45-docker.sh myprogram.c

# With custom toolchain root
CC45_ROOT=/opt/m65compiler ./scripts/cc45-docker.sh myprogram.c

# With custom working directory
WORK_DIR=./src ./scripts/cc45-docker.sh myprogram.c

# Custom output directory
OUTPUT_DIR=./build ./scripts/cc45-docker.sh myprogram.c

# Compile to object file (-c flag)
./scripts/cc45-docker.sh myprogram.c -c
```

#### Option B: Direct docker run

```bash
# Compile with custom include path
docker run --rm \
  -v /path/to/m65compiler/lib:/opt/m65/lib:ro \
  -v $(pwd):/work:rw \
  m65compiler:builder \
  cc45 -I/opt/m65/lib/include /work/myprogram.c -o /work/myprogram.s
```

### 3. Link and Create PRG File

```bash
# Using wrapper script
./scripts/ln45-docker.sh -basic -o /work/myprogram.prg \
  /work/myprogram.o45 /opt/m65/lib/c45.lib

# Or manually with docker run
docker run --rm \
  -v /path/to/m65compiler/lib:/opt/m65/lib:ro \
  -v $(pwd):/work:rw \
  m65compiler:builder \
  ln45 -basic -o /work/myprogram.prg /work/myprogram.o45 /opt/m65/lib/c45.lib
```

---

## Environment Variables

### Script Configuration

All wrapper scripts support these environment variables:

| Variable | Default | Purpose |
|----------|---------|---------|
| `CC45_ROOT` | `.` (PWD) | Path to m65compiler repository |
| `CC45_IMAGE` | `m65compiler:builder` | Docker image name/tag |
| `WORK_DIR` | `.` (PWD) | Working directory for compilation |
| `OUTPUT_DIR` | `.` (PWD) | Where to write output files |
| `CC45_DOCKER` | (empty) | Additional Docker flags (e.g., `-e VAR=val`) |

### Example: Custom configuration

```bash
# Set all options at once
export CC45_ROOT=/opt/m65compiler
export WORK_DIR=$(pwd)/src
export OUTPUT_DIR=$(pwd)/build
export CC45_IMAGE=m65compiler:v1.1

./scripts/cc45-docker.sh myprogram.c
```

---

## Volume Mounting Strategy

The wrapper scripts use **read-only toolchain mounts** for safety and efficiency:

```
Docker Container
├── /opt/m65/lib (read-only, from CC45_ROOT/lib)
├── /opt/m65/bin (read-only, from CC45_ROOT/bin)
└── /work (read-write, from WORK_DIR)
```

**Benefits:**
- Toolchain is immutable during compilation
- Multiple containers can share the same toolchain
- Output files are immediately accessible on host
- No need to copy files in/out of container

---

## Image Details

### Multi-stage Build Process

1. **Stage 1 (builder):** Compiles from source
   - Includes: g++, make, build tools
   - Runs: `make clean && make all -j4`
   - Runs validation tests

2. **Stage 2 (final):** Minimal runtime image
   - Stripped binaries
   - Headers and library archives
   - Excludes: build tools, source code, tests
   - **Size:** ~120MB

### What's Included

**Binaries** (~8MB):
- `/usr/local/bin/cc45` — C compiler
- `/usr/local/bin/ca45` — 45GS02 assembler
- `/usr/local/bin/ln45` — Linker
- `/usr/local/bin/ar45` — Archiver
- `/usr/local/bin/nm45` — Symbol inspector
- `/usr/local/bin/objdump45` — Disassembler
- `/usr/local/bin/cp45` — C preprocessor

**Libraries** (~30MB):
- `/usr/local/lib/c45.lib` — Stack convention stdlib
- `/usr/local/lib/c45_zp.lib` — ZP convention stdlib
- All variants for different calling conventions

**Headers** (~5MB):
- `/usr/local/include/stdio.h`
- `/usr/local/include/stdlib.h`
- `/usr/local/include/string.h`
- `/usr/local/include/ctype.h`
- `/usr/local/include/math.h`
- And all other standard headers

### Verification

```bash
# Check image size
docker images m65compiler:builder

# Verify binaries are present
docker run m65compiler:builder cc45 --version

# List available tools
docker run m65compiler:builder ls -la /usr/local/bin/
```

---

## Workflow Examples

### Example 1: Compile a simple program

```bash
# Create a test program
cat > hello.c << 'EOF'
#include <stdio.h>

int main() {
    printf("Hello, MEGA65!\n");
    return 0;
}
EOF

# Compile
./scripts/cc45-docker.sh hello.c

# Link (if separate .o45 file)
./scripts/ln45-docker.sh -basic -o hello.prg hello.o45
```

### Example 2: Multiple files with Makefile

```bash
# Traditional Makefile (runs on host)
all:
	@echo "Building with Docker..."
	@mkdir -p build
	./scripts/cc45-docker.sh src/main.c -c -o build/main.o45
	./scripts/cc45-docker.sh src/lib.c -c -o build/lib.o45
	./scripts/ln45-docker.sh -basic -o build/program.prg \
		build/main.o45 build/lib.o45 /opt/m65/lib/c45.lib
```

### Example 3: Custom output directory

```bash
# Build artifacts go to ./build/
mkdir -p build
WORK_DIR=./build OUTPUT_DIR=./build ./scripts/cc45-docker.sh ../src/program.c

# Output is in ./build/program.s or ./build/program.prg
ls -lh build/program.*
```

---

## Troubleshooting

### "docker: command not found"

Install Docker Desktop or Docker Engine:
- **macOS:** https://docs.docker.com/desktop/install/mac-install/
- **Linux:** https://docs.docker.com/engine/install/
- **Windows WSL2:** https://docs.docker.com/desktop/install/windows-install/

### "Error: Source file not found"

Ensure the file path is correct and relative to where you run the script:

```bash
# Wrong: file not in current directory
./scripts/cc45-docker.sh nonexistent.c

# Right: use correct path
./scripts/cc45-docker.sh ./src/myprogram.c
```

### "Error: CC45_ROOT does not contain lib/ directory"

The `CC45_ROOT` environment variable must point to the m65compiler repository root:

```bash
# Set it correctly
export CC45_ROOT=/path/to/m65compiler
./scripts/cc45-docker.sh myprogram.c
```

### "Image m65compiler:builder not found"

Build the image first:

```bash
docker build -f Dockerfile.build -t m65compiler:builder .
```

### "Permission denied: /work/..."

The work directory permissions might be restrictive. Ensure the directory is writable:

```bash
chmod 755 ./work_directory
./scripts/cc45-docker.sh myprogram.c
```

---

## Advanced Usage

### Using a specific image version

```bash
# Tag the image with a version
docker build -f Dockerfile.build -t m65compiler:v1.1 .

# Use that version
CC45_IMAGE=m65compiler:v1.1 ./scripts/cc45-docker.sh myprogram.c
```

### Interactive shell

```bash
# Get a bash shell in the container with tools available
docker run --rm -it \
  -v $(pwd)/lib:/opt/m65/lib:ro \
  -v $(pwd)/bin:/opt/m65/bin:ro \
  -v $(pwd):/work:rw \
  m65compiler:builder bash

# Inside container:
/work# cc45 --version
/work# ls -la /opt/m65/lib/include/
```

### Adding custom libraries

Mount additional library directories:

```bash
docker run --rm \
  -v /path/to/m65compiler/lib:/opt/m65/lib:ro \
  -v /path/to/custom/lib:/opt/custom/lib:ro \
  -v $(pwd):/work:rw \
  m65compiler:builder \
  cc45 -I/opt/m65/lib/include -I/opt/custom/lib/include \
    -L/opt/m65/lib -L/opt/custom/lib \
    /work/myprogram.c
```

---

## CI/CD Integration

### GitHub Actions Example

```yaml
name: Build with Docker

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Build Docker image
        run: docker build -f Dockerfile.build -t m65compiler:builder .
      
      - name: Compile example
        run: |
          mkdir -p build
          docker run --rm \
            -v $(pwd)/lib:/opt/m65/lib:ro \
            -v $(pwd)/bin:/opt/m65/bin:ro \
            -v $(pwd)/build:/work:rw \
            m65compiler:builder \
            cc45 -I/opt/m65/lib/include /work/example.c -o /work/example.s
```

---

## Related Issues

- **#98:** Docker compilation environment (Dockerfile.build, wrapper scripts)
- **#99:** Docker run-time environment (emulator testing)

---

## References

- [Docker Documentation](https://docs.docker.com/)
- [MEGA65 Hardware](https://github.com/MEGA65/mega65-core)
- [Compiler Documentation](./compiler.md)
