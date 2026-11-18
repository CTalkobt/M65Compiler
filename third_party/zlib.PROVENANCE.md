# zlib in m65compiler

zlib, the compression library, used by `disk45` for `.gz` and `.zip` disk
images: inflate, deflate and crc32 (`src/main/GzipHelper.cpp`,
`src/main/ZipArchive.cpp`, `src/main/disk45_main.cpp`; #620).

Unlike the other directories here, `third_party/zlib` is not a copy: it is a
git submodule (`.gitmodules`), so this note is beside it and not inside.

- Upstream: https://github.com/madler/zlib
- Tag: `v1.4` (commit 99623ef7097352da7993ba614161fd6a311e046e)
- License: the zlib licence (`third_party/zlib/LICENSE`)
- Built: `adler32.c compress.c crc32.c deflate.c inffast.c inflate.c
  inftrees.c trees.c zutil.c` with the C compiler, into `lib45/lib45-zlib.a`
  (the Makefile's `ZLIB_SRCS`). Not its `gz*` file functions, `infback.c`,
  `uncompr.c`, `contrib/` (minizip), the tests, the examples or its own build
  files; no `./configure` (the `zconf.h` of the repository as it is).
- No file is changed and the build writes nothing in the directory. Its
  warnings are off (`-w`).

Getting it: `git clone --recurse-submodules`, or in an existing clone
`git submodule update --init third_party/zlib` (README.md, "The bundled
zlib: what to run when", has every case). A tree without it (GitHub's
source archives never contain submodules) is built with the system's zlib;
`make SYSTEM_ZLIB=1` uses the system's even when this one is there.

Moving to a newer zlib (a security release):

```bash
git -C third_party/zlib fetch --tags
git -C third_party/zlib checkout v<new>
git add third_party/zlib          # and the tag and commit above, CHANGELOG.md
make && bash src/test/test_disk45.sh
```

A new release may add a source file: a link error names the missing symbol,
and the file goes in `ZLIB_SRCS`.
