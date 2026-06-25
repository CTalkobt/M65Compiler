# MEGA65 C Compiler Suite — Docker Setup

This Dockerfile encapsulates the complete MEGA65 C Compiler Suite toolchain, allowing you to use the compiler and related tools via Docker with volume mounts for local source files.

## Building the Docker Image

### Ubuntu Version (Default - Recommended)
Larger but more feature-complete and stable:

```bash
docker build -t mega65-cc45 .
docker build -t mega65-cc45:ubuntu -f Dockerfile .
```

**Size:** 97.1 MB | **Features:** Full shell, package manager, debugging tools

### Alpine Version (Lightweight)
Smaller footprint, good for production deployments:

```bash
docker build -t mega65-cc45:alpine -f Dockerfile.alpine .
```

**Size:** 29.4 MB (70% smaller) | **Trade-off:** Minimal shell, fewer tools

### Both versions are fully functional with all 8 tools included.

## Quick Start with Aliases

For the easiest experience, source the provided alias script:

```bash
source docker-aliases.sh
```

This creates convenient command aliases so you can use tools naturally:

```bash
cc45 hello.c -o hello.s       # Compile C to assembly
ca45 hello.s -o hello.prg     # Assemble to binary
ln45 *.o45 -o output.prg      # Link object files
disk45 create game.d81        # Create disk image
nm45 object.o45               # Inspect symbols
```

## Usage Without Aliases

### Default Tool: cc45 (C Compiler)

The default entry point is `cc45`, so you can compile C code directly:

```bash
docker run -v $(pwd):/work mega65-cc45:latest hello.c -o hello.prg
docker run -v $(pwd):/work mega65-cc45:latest -S hello.c -o hello.s    # Assembly output
docker run -v $(pwd):/work mega65-cc45:latest -c hello.c -o hello.o45  # Object file
```

### Using Other Tools

To invoke other tools, use the `--entrypoint` flag:

#### Assembler (ca45)
```bash
docker run -v $(pwd):/work --entrypoint ca45 mega65-cc45:latest program.s -o program.prg
```

#### Preprocessor (cp45)
```bash
docker run -v $(pwd):/work --entrypoint cp45 mega65-cc45:latest -E input.c -o output.i
```

#### Linker (ln45)
```bash
docker run -v $(pwd):/work --entrypoint ln45 mega65-cc45:latest object1.o45 object2.o45 -o output.prg
```

#### Symbol Inspector (nm45)
```bash
docker run -v $(pwd):/work --entrypoint nm45 mega65-cc45:latest object.o45
```

#### Object Disassembler (objdump45)
```bash
docker run -v $(pwd):/work --entrypoint objdump45 mega65-cc45:latest -d object.o45
docker run -v $(pwd):/work --entrypoint objdump45 mega65-cc45:latest -r object.o45  # Show relocations
```

#### Archiver (ar45)
```bash
docker run -v $(pwd):/work --entrypoint ar45 mega65-cc45:latest c mylib.lib object1.o45 object2.o45
docker run -v $(pwd):/work --entrypoint ar45 mega65-cc45:latest t mylib.lib          # List archive contents
docker run -v $(pwd):/work --entrypoint ar45 mega65-cc45:latest s mylib.lib          # Show symbol table
```

#### Disk Image Utility (disk45)
```bash
docker run -v $(pwd):/work --entrypoint disk45 mega65-cc45:latest create game.d81 -n "MY GAME"
docker run -v $(pwd):/work --entrypoint disk45 mega65-cc45:latest add game.d81 hello.prg "HELLO"
docker run -v $(pwd):/work --entrypoint disk45 mega65-cc45:latest list game.d81
docker run -v $(pwd):/work --entrypoint disk45 mega65-cc45:latest info game.d81
```

## Complete Compilation Workflow Example

```bash
# Compile C files to object files
docker run -v $(pwd):/work mega65-cc45:latest main.c -c -o main.o45
docker run -v $(pwd):/work mega65-cc45:latest utils.c -c -o utils.o45

# Link objects into executable (if using aliases)
ln45 main.o45 utils.o45 -o final.prg

# Or without aliases:
docker run -v $(pwd):/work --entrypoint ln45 mega65-cc45:latest main.o45 utils.o45 -o final.prg

# Create a disk image
disk45 create output.d81 -n "MYGAME"
disk45 add output.d81 final.prg "FINAL"

# Inspect the executable
nm45 main.o45
objdump45 -d main.o45
```

## Container Environment

### Volume Mount
- Local directory is mounted to `/work` inside the container
- The container's working directory is `/work`

### Library Path
- Headers are located at `/app/lib/cc45/include/`
- Standard library archives: `/app/lib/cc45/c45.lib` (stack convention) and `/app/lib/cc45/c45_zp.lib` (ZP convention)
- Startup files: `/app/lib/cc45/crt*.s`

### Available Tools (in `/app/bin/`)
- `cc45` — C compiler (default)
- `ca45` — 45GS02 assembler
- `cp45` — C preprocessor
- `nm45` — Symbol inspector
- `ln45` — Linker
- `ar45` — Archiver
- `objdump45` — Object disassembler
- `disk45` — Disk image utility

## Environment Variables

Inside the container:
- `CC45_LIB_PATH=/app/lib/cc45` — Path to compiler libraries
- `PATH` includes `/app/bin` with all tools

## Tips

### Using an Interactive Shell

For debugging or running multiple commands:

```bash
docker run -it -v $(pwd):/work mega65-cc45:latest /bin/bash
# Now inside the container:
cd /work
cc45 hello.c -c -o hello.o45
nm45 hello.o45
objdump45 -d hello.o45
```

### File Permissions

If compiled files are owned by root, adjust permissions after:

```bash
docker run -v $(pwd):/work --user $(id -u):$(id -g) mega65-cc45:latest cc45 hello.c -o hello.prg
```

Or use the `--rm` flag (already in alias script) to clean up containers.

### Setting Include Paths

Pass include paths to the compiler:

```bash
cc45 -I/path/to/headers hello.c -o hello.s
```

### Linking with Standard Library

To link successfully, you'll need to link with the standard library:

```bash
ln45 main.o45 -l /app/lib/cc45/c45.lib -o main.prg
```

## Troubleshooting

### Cannot find stdio.h

The compiler needs the `-I` flag to find headers:

```bash
docker run -v $(pwd):/work mega65-cc45:latest -I/app/lib/cc45/include hello.c -c -o hello.o45
```

Or create your own headers without dependencies.

### Linker: undefined symbol errors

Link with the standard library:

```bash
ln45 your_object.o45 /app/lib/cc45/c45.lib -o output.prg
```

### Docker permission issues

If you encounter permission errors, you may need to use:

```bash
docker run --user $(id -u):$(id -g) -v $(pwd):/work mega65-cc45:latest cc45 hello.c
```

### Image size is large

The Docker image is ~250-300 MB, which includes the full compilation toolchain. To reduce size, you can:
- Remove documentation and example files from `.dockerignore`
- Use a smaller base image (though this may require additional compatibility work)
- Use multi-stage builds to extract only binaries

## References

- **Project Documentation**: See `CLAUDE.md` for architecture and feature details
- **Compiler Flags**: Run `cc45 -?` inside the container for all options
- **Toolchain Documentation**: See `doc/` directory in the source repository
