# Getting Started with Docker

A step-by-step guide to compiling MEGA65 C programs using Docker.

## Prerequisites

- **Docker:** Install [Docker Desktop](https://www.docker.com/products/docker-desktop) (macOS, Windows) or [Docker Engine](https://docs.docker.com/engine/install/) (Linux)
- **This repository:** Clone the m65compiler repo
- **No local C++ compiler needed** — Docker handles compilation

## Step 1: Clone the Repository

```bash
git clone https://github.com/CTalkobt/M65Compiler.git
cd M65Compiler
```

## Step 2: Build the Docker Image

Build the compiler image (one-time, takes ~2.5 minutes):

```bash
docker build -f Dockerfile.build -t m65compiler:builder .
```

**Verify the build:**
```bash
docker images m65compiler:builder
```

You should see an image ~92MB in size.

## Step 3: Create Your First Program

Create a simple C program:

```bash
cat > hello.c << 'EOF'
#include <stdio.h>

int main() {
    printf("Hello, MEGA65!\n");
    return 0;
}
EOF
```

## Step 4: Compile to Assembly

Use the `cc45-docker.sh` wrapper script:

```bash
./scripts/cc45-docker.sh hello.c
```

**Expected output:**
```
Compiling: hello.c
  Toolchain: /path/to/m65compiler
  Work dir: /current/directory
  Image: m65compiler:builder
✓ Output: /current/directory/hello.s
```

**Check the output:**
```bash
ls -lh hello.s
head -30 hello.s
```

You should see 45GS02 assembly code.

## Step 5: Compile to Object File

For relocatable compilation (needed for linking):

```bash
./scripts/cc45-docker.sh hello.c -c
```

This creates `hello.o45` (relocatable object file).

## Step 6: Link to Create a PRG File

```bash
./scripts/ln45-docker.sh -basic -o hello.prg hello.o45 /opt/m65/lib/c45.lib
```

**Verify the binary:**
```bash
ls -lh hello.prg
file hello.prg
```

You now have a `hello.prg` file ready to run on a MEGA65!

## Next: Multi-File Project

Once you're comfortable with single-file compilation, try multiple files:

```bash
# Create a simple math library
cat > math.c << 'EOF'
int add(int a, int b) {
    return a + b;
}
EOF

# Create header
cat > math.h << 'EOF'
int add(int a, int b);
EOF

# Create main program
cat > main.c << 'EOF'
#include <stdio.h>
#include "math.h"

int main() {
    int result = add(5, 3);
    printf("5 + 3 = %d\n", result);
    return 0;
}
EOF
```

**Compile and link:**
```bash
# Compile to objects
./scripts/cc45-docker.sh main.c -c -o main.o45
./scripts/cc45-docker.sh math.c -c -o math.o45

# Link
./scripts/ln45-docker.sh -basic -o app.prg main.o45 math.o45 /opt/m65/lib/c45.lib
```

## Common Tasks

### Check compiler version
```bash
docker run m65compiler:builder cc45 --version
```

### View available compiler options
```bash
docker run m65compiler:builder cc45 -?
```

### Inspect object file symbols
```bash
docker run --rm -v $(pwd):/work m65compiler:builder \
  nm45 /work/hello.o45
```

### Disassemble object file
```bash
docker run --rm -v $(pwd):/work m65compiler:builder \
  objdump45 -d /work/hello.o45
```

### Interactive compilation in container shell
```bash
# Open bash shell with tools available
docker run --rm -it \
  -v $(pwd)/lib:/opt/m65/lib:ro \
  -v $(pwd)/bin:/opt/m65/bin:ro \
  -v $(pwd):/work:rw \
  m65compiler:builder bash

# Inside container, run commands:
/work# cc45 -I/opt/m65/lib/include hello.c -o hello.s
/work# ca45 hello.s -o hello.prg
/work# nm45 hello.o45
/work# exit
```

## Using a Makefile

Create a `Makefile` for easier builds:

```makefile
CC45 := ./scripts/cc45-docker.sh
LN45 := ./scripts/ln45-docker.sh

SOURCES := main.c math.c
OBJECTS := $(SOURCES:.c=.o45)
TARGET := app.prg

all: $(TARGET)

%.o45: %.c
	$(CC45) $< -c -o $@

$(TARGET): $(OBJECTS)
	$(LN45) -basic -o $@ $(OBJECTS) /opt/m65/lib/c45.lib

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all clean
```

Now build with:
```bash
make
```

## Environment Variables

Configure the wrapper scripts with environment variables:

```bash
# Use a custom toolchain location
export CC45_ROOT=/opt/my-m65compiler
./scripts/cc45-docker.sh program.c

# Compile source from different directory
export WORK_DIR=./src
./scripts/cc45-docker.sh program.c

# Place output in build directory
export OUTPUT_DIR=./build
./scripts/cc45-docker.sh program.c
```

## Troubleshooting

### Docker not found
```
docker: command not found
```
**Solution:** Install Docker Desktop or Docker Engine from https://www.docker.com

### Image not found
```
Error: m65compiler:builder not found
```
**Solution:** Build the image first:
```bash
docker build -f Dockerfile.build -t m65compiler:builder .
```

### Source file not found
```
Failed to open input file: /work/program.c
```
**Solution:** Ensure the file exists in your current directory and use the correct path:
```bash
# Wrong:
./scripts/cc45-docker.sh /home/user/program.c

# Right:
./scripts/cc45-docker.sh program.c
```

### Permission denied
```
Permission denied: /work/output.s
```
**Solution:** The work directory may have restrictive permissions:
```bash
chmod 755 .
./scripts/cc45-docker.sh program.c
```

## What's Next?

- **Advanced usage:** See [docs/DOCKER.md](DOCKER.md) for advanced workflows
- **Quick reference:** See [docs/DOCKER-QUICKREF.md](DOCKER-QUICKREF.md) for common commands
- **Language features:** See [docs/compiler.md](compiler.md) for C language support
- **MEGA65 libraries:** See [docs/stdlib.md](stdlib.md) for available standard library functions

## Need Help?

- **Docker issues?** Check [DOCKER.md troubleshooting](DOCKER.md#troubleshooting)
- **Compiler issues?** See [compiler.md documentation](compiler.md)
- **GitHub issues?** Report at https://github.com/CTalkobt/M65Compiler/issues

---

**Happy coding on MEGA65!**
