# Docker Quick Reference

Fast reference for using the MEGA65 C Compiler in Docker.

## Setup (One-time)

```bash
# Build the Docker image (in repo root)
docker build -f Dockerfile.build -t m65compiler:builder .
```

## Basic Compilation

```bash
# Compile to assembly
./scripts/cc45-docker.sh myprogram.c
# Output: myprogram.s

# Compile to object file
./scripts/cc45-docker.sh myprogram.c -c
# Output: myprogram.o45
```

## Environment Variables

| Variable | Default | Purpose |
|----------|---------|---------|
| `CC45_ROOT` | `.` | Toolchain root directory |
| `WORK_DIR` | `.` | Where source and output files are |
| `OUTPUT_DIR` | `.` | Where to write output (if different) |
| `CC45_IMAGE` | `m65compiler:builder` | Docker image to use |

## Common Workflows

### Single File
```bash
./scripts/cc45-docker.sh program.c                    # Assembly
./scripts/cc45-docker.sh program.c -c                 # Object file
./scripts/cc45-docker.sh program.c -S                 # Keep assembly
```

### Multiple Files
```bash
# Compile each file
./scripts/cc45-docker.sh src/main.c -c -o build/main.o45
./scripts/cc45-docker.sh src/lib.c -c -o build/lib.o45

# Link together
./scripts/ln45-docker.sh -basic -o build/app.prg \
  build/main.o45 build/lib.o45 /opt/m65/lib/c45.lib
```

### Create Library
```bash
# Compile source
./scripts/cc45-docker.sh src/math.c -c -o build/math.o45

# Archive into library
./scripts/ar45-docker.sh c build/math.lib build/math.o45

# Link using library
./scripts/ln45-docker.sh -basic -o build/app.prg \
  build/main.o45 build/math.lib
```

### Custom Directories
```bash
# Source in different location
CC45_ROOT=/opt/m65compiler WORK_DIR=./src ./scripts/cc45-docker.sh program.c

# Output to build directory
WORK_DIR=./build ./scripts/cc45-docker.sh ../src/program.c
```

## Inspect Object Files

```bash
# List symbols
docker run --rm -v $(pwd):/work m65compiler:builder \
  nm45 /work/myprogram.o45

# Disassemble
docker run --rm -v $(pwd):/work m65compiler:builder \
  objdump45 -d /work/myprogram.o45

# Show relocations
docker run --rm -v $(pwd):/work m65compiler:builder \
  objdump45 -r /work/myprogram.o45
```

## Interactive Shell

```bash
# Open bash with tools available
docker run --rm -it \
  -v $(pwd)/lib:/opt/m65/lib:ro \
  -v $(pwd)/bin:/opt/m65/bin:ro \
  -v $(pwd):/work:rw \
  m65compiler:builder bash

# Inside container:
/work# cc45 -I/opt/m65/lib/include /work/program.c -o /work/program.s
/work# nm45 /work/program.o45
```

## Makefile Integration

```makefile
CC45 := ./scripts/cc45-docker.sh
LN45 := ./scripts/ln45-docker.sh

SRC := src/main.c src/lib.c
OBJ := $(SRC:src/%.c=build/%.o45)
PRG := build/program.prg

all: $(PRG)

build/%.o45: src/%.c
	$(CC45) $< -c -o $@

$(PRG): $(OBJ)
	$(LN45) -basic -o $@ $(OBJ) /opt/m65/lib/c45.lib

clean:
	rm -rf build
```

## Compiler Options

```bash
# Common flags
./scripts/cc45-docker.sh program.c -c              # Compile to object
./scripts/cc45-docker.sh program.c -S              # Keep assembly
./scripts/cc45-docker.sh program.c -E              # Preprocess only
./scripts/cc45-docker.sh program.c -fzpcall        # ZP calling convention
./scripts/cc45-docker.sh program.c -O2             # Optimization level
./scripts/cc45-docker.sh program.c -Wall           # All warnings
./scripts/cc45-docker.sh program.c -I/path/include # Include path
```

## Linker Options

```bash
# Common flags
./scripts/ln45-docker.sh -basic -o app.prg file.o45   # Create PRG
./scripts/ln45-docker.sh -flat -o app.bin file.o45    # Create binary
./scripts/ln45-docker.sh -a $2000 -o app.prg file.o45 # Custom address
```

## Troubleshooting

| Issue | Solution |
|-------|----------|
| `docker: command not found` | Install Docker Desktop or Docker Engine |
| `Source file not found` | Use correct relative path: `./src/program.c` |
| `CC45_ROOT not found` | Set to m65compiler repo root: `export CC45_ROOT=/path/to/repo` |
| `Image not found` | Build it: `docker build -f Dockerfile.build -t m65compiler:builder .` |

## Image Information

```bash
# Check image size
docker images m65compiler:builder --format "table {{.Size}}"

# View image details
docker inspect m65compiler:builder

# Check available tools
docker run m65compiler:builder ls -lh /usr/local/bin/
```

## Advanced

```bash
# Run with additional Docker flags
CC45_DOCKER="-e DEBUG=1 -v /extra/lib:/opt/extra:ro" ./scripts/cc45-docker.sh program.c

# Compile with custom preprocessor defines
docker run --rm -v $(pwd):/work m65compiler:builder \
  cc45 -DDEBUG=1 -DVERSION=100 /work/program.c -o /work/program.s

# Use specific image version
CC45_IMAGE=m65compiler:v1.0 ./scripts/cc45-docker.sh program.c
```

---

**For full documentation, see:** [DOCKER.md](DOCKER.md)
