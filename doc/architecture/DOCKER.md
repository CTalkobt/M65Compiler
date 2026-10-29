# MEGA65 C Compiler Suite — Docker Setup

The repository has two Dockerfiles:

| File | Base | Used by | Notes |
|------|------|---------|-------|
| `Dockerfile` (project root) | Ubuntu 22.04, two-stage | `docker build .` | Recommended. Tools on `PATH`, libraries and headers in the install layout. |
| `src/Docker/Dockerfile` | Alpine 3.20, two-stage | `make docker` | The toolchain only, installed in `/app`; the entrypoint is `cc45`. |
| `src/Docker/Dockerfile.runtime` | Alpine 3.20, three-stage | `make docker-runtime` | The toolchain and the xemu emulator, to run programs (#99). |

The root image and the runtime image include **xemu** (`xemu-xmega65`), built from CTalkobt/xemu's `cc45-dev` branch at a pinned commit: upstream xemu's `dev` with `XEMU_PREF_DIR`, the monitor's breakpoint list (`k`, which `dbg45` needs) and the hardware divider as the core computes it. **The MEGA65 ROM is not included**; mount yours, see [Running programs in xemu](#running-programs-in-xemu).

Image sizes are not given here; check them with `docker images` after building.

## Root Dockerfile (Ubuntu)

### Build

```bash
docker build -t cc45 .
docker build -t cc45:vX.Y.Z .      # with a version tag
```

The builder stage runs `make clean && make all && make -C lib` (with FUSE3, SQLite and zlib, so `disk45 mount` and the catalog are available). The runtime image contains:

| Path | Contents |
|------|----------|
| `/usr/local/bin/` | everything in `bin/`: `cc45`, `cp45`, `ca45`, `ln45`, `ar45`, `nm45`, `objdump45`, `disk45`, `cvt_asm`, `basic45` (plus the C++ test binaries) |
| `/usr/local/lib/cc45/` | `lib/build/`: `c45.lib` (ZP convention), `c45_zp.lib` (copy), `c45_stack.lib` (stack convention) and the `.o45` members |
| `/usr/local/include/cc45/` | `lib/include/` (including `sys/`) |
| `/opt/cc45/` | the full source tree (working directory) |

Environment: `CC45_INCLUDE=/usr/local/include/cc45` (header search path for `cc45`/`ca45`) and `CC45_LIB=/usr/local/lib/cc45` (library search path for `ln45`). A one-step `cc45 file.c -o file.prg` finds the right library itself (`bin/../lib/cc45/`). `VOLUME /catalog` is declared for the `disk45` catalog database. There is no entrypoint; the default command is `cc45 --version`, so name the tool explicitly:

```bash
docker run --rm cc45 cc45 --version
docker run --rm -v $(pwd):/work cc45 cc45 /work/hello.c -o /work/hello.prg      # compile + link
docker run --rm -v $(pwd):/work cc45 cc45 -S /work/hello.c -o /work/hello.s45   # assembly only
docker run --rm -v $(pwd):/work cc45 cc45 -c /work/main.c -o /work/main.o45     # object only
docker run --rm -v $(pwd):/work cc45 ln45 -basic /work/main.o45 /work/util.o45 -l c45.lib -o /work/game.prg
docker run --rm -v $(pwd):/work cc45 disk45 create /work/game.d81
docker run --rm -v $(pwd):/work cc45 disk45 list /work/game.d81
docker run --rm -v $(pwd):/work -v catalog:/catalog cc45 \
    disk45 catalog build /work --db /catalog/disk45.db
```

Paths are inside the container, so pass `/work/...` (the working directory is `/opt/cc45`, not `/work`). Use `-l c45_stack.lib` for objects compiled with `-fno-zpcall`.

Files written into `/work` are owned by root; add `--user $(id -u):$(id -g)` to keep your own ownership.

For an interactive shell:

```bash
docker run --rm -it -v $(pwd):/work cc45 bash
```

`make test` can be run inside the container (`docker run --rm cc45 make test`); with your ROM mounted, so can the runtime tests and the examples that run in xemu:

```bash
docker run --rm -v /path/to/MEGA65.ROM:/rom/MEGA65.ROM:ro cc45 make test-xemu
docker run --rm -v /path/to/MEGA65.ROM:/rom/MEGA65.ROM:ro cc45 make test
```

The image's user is `m65` (uid 1000; xemu does not run as root), who owns the source tree in `/opt/cc45`.

## Alpine Image (`make docker`)

`make docker` runs `docker build -f src/Docker/Dockerfile -t mega65-cc45:latest .` and then `docker save` to `bin/mega65-cc45.tar`, which can be loaded elsewhere with `docker load -i mega65-cc45.tar`. The suite is installed with `make install PREFIX=/app` (tools in `/app/bin`, libraries in `/app/lib/cc45`, headers in `/app/include/cc45`: the installed layout the tools find by themselves), the entrypoint is `cc45`, and the working directory `/work`:

```bash
docker run --rm -v "$PWD":/work --user $(id -u):$(id -g) mega65-cc45 -O2 hello.c -o hello.prg
docker run --rm -v "$PWD":/work --entrypoint disk45 mega65-cc45 list game.d81
```

`src/Docker/docker-aliases.sh` defines shell aliases (`cc45`, `ca45`, `cp45`, `nm45`, `ln45`, `ar45`, `objdump45`, `disk45`, `basic45`, `cvt_asm`) that run this image with the current directory mounted at `/work`, as the calling user.

## Runtime Image with xemu (`make docker-runtime`)

`src/Docker/Dockerfile.runtime` (#99) adds xemu to the Alpine toolchain: `docker build -f src/Docker/Dockerfile.runtime -t mega65-cc45:runtime .` (or `make docker-runtime`). The tools are on `PATH`, the user is `m65`, the working directory `/work`, and any command can be given (default `sh`).

## Running Programs in xemu

The MEGA65 ROM (`MEGA65.ROM`, 128 KB) cannot be distributed: mount yours at `/rom/MEGA65.ROM` (or set `MEGA65_ROM` to where it is). On first use, `xemu-xmega65` (a wrapper, `src/Docker/xemu-wrapper.sh`; the emulator is `/usr/local/libexec/xmega65`) sets up xemu's preferences directory `$XEMU_PREF_DIR` (`/tmp/xemu-pref`): an SD card image made by `disk45 sd create --onboarded --pal` (laid out as the MEGA65's FDISK does, configured as after the first-boot onboarding, which would otherwise wait for keys), with the ROM on it. The entrypoint (`src/Docker/m65-entry.sh`) does this before the command when a ROM is mounted, so the test scripts find it ready.

`m65run` runs a program headless and reports how it ended:

```bash
R="-v /path/to/MEGA65.ROM:/rom/MEGA65.ROM:ro -v $PWD:/work --user $(id -u):$(id -g)"
docker run --rm $R mega65-cc45:runtime cc45 -O2 hello.c -o hello.prg
docker run --rm $R mega65-cc45:runtime m65run hello.prg                 # exit status 0
docker run --rm $R mega65-cc45:runtime m65run -d 0xC0C0:8 -s shot.png test.prg
```

```
m65run [-t SECONDS] [-d ADDR:LEN]... [-s FILE.png] [-8 DISK.d81] [-c64] PROG.prg
```

The program is loaded in C65 mode (`-c64`: C64 mode); xemu stops at BASIC's `READY.` after it, or after `SECONDS` (default 15). It prints the program's exit status from `$FD/$FE` (65535: `__exit` never reached; 65534: after `abort()`) and, for each `-d`, that many bytes of memory from that address (decimal, or hex as `0xC0C0`); `-s` saves a screenshot, `-8` mounts a disk image on drive 8. The exit code is 0 for status 0, 1 for another status, 2 for an error (no ROM, xemu failed, no result in time).

`xemu-xmega65` itself takes xemu's options as usual (`-headless -besure -sleepless -prgmode 65 -prgexit -dumpmem out.mem -prg x.prg`). xemu does not run as root: use the image's user or `--user`.

## References

- [../bin/cc45.md](../bin/cc45.md), [../bin/ln45.md](../bin/ln45.md), [../bin/disk45.md](../bin/disk45.md) — tool options
- [stdlib.md](stdlib.md) — which library goes with which calling convention
