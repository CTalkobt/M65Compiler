# MEGA65 C Compiler Suite — Docker Setup

The repository has two Dockerfiles:

| File | Base | Used by | Notes |
|------|------|---------|-------|
| `Dockerfile` (project root) | Ubuntu 22.04, two-stage | `docker build .` | Recommended. Tools on `PATH`, libraries and headers in the install layout. |
| `src/Docker/Dockerfile` | Alpine 3.20, two-stage | `make docker` | Currently broken — see [Alpine image](#alpine-image-make-docker). |

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

`make test` can be run inside the container (`docker run --rm cc45 make test`), but the image has no `xemu-xmega65`, so it stops at the xemu runtime tests.

## Alpine Image (`make docker`)

`make docker` runs `docker build -f src/Docker/Dockerfile -t mega65-cc45:latest .` and then `docker save` to `bin/mega65-cc45.tar`, which can be loaded elsewhere with `docker load -i mega65-cc45.tar`.

As written, this Dockerfile does not build: it copies `lib/crt*.s`, but the startup files are `lib/crt*.s45`, so the `COPY` matches nothing and fails. Even with that fixed, the layout does not match what the tools look for:

- headers go to `/app/lib/cc45/include/` (and `lib/include/*` flattens `sys/types.h` to `types.h`), but `cc45` in `/app/bin` searches `/app/lib/include`, and the image sets no `CC45_INCLUDE`;
- it sets `CC45_LIB_PATH`, which no tool reads (`ln45` reads `CC45_LIB`);
- `basic45` and `cvt_asm` are not copied.

The libraries are copied to `/app/lib/cc45/`, which is where a one-step `cc45 ... -o x.prg` looks, and the entrypoint is `/app/bin/cc45` (other tools via `--entrypoint`). `src/Docker/docker-aliases.sh` defines shell aliases (`cc45`, `ca45`, `cp45`, `nm45`, `ln45`, `ar45`, `objdump45`, `disk45`) that run this `mega65-cc45:latest` image with the current directory mounted at `/work`.

Use the root Dockerfile until the Alpine one is fixed.

## References

- [../bin/cc45.md](../bin/cc45.md), [../bin/ln45.md](../bin/ln45.md), [../bin/disk45.md](../bin/disk45.md) — tool options
- [stdlib.md](stdlib.md) — which library goes with which calling convention
