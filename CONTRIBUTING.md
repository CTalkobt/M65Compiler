# Contributing

How the repository is worked on: branches, building, the tests, adding tests, the docs. The code's layout and
conventions are in [CLAUDE.md](CLAUDE.md) (the codebase guide); the reference pages are in [doc/](doc/README.md).

## Branches and pull requests

- Work on a branch from `main`, never on `main` itself. Name it for the kind of change and the issue:
  `feat/611-asm-style`, `fix/676-shift-bytemove`, `docs/644-new-pages`.
- Commit messages: the tool or area, what changed, the issue in parentheses:
  `cc45: (unsigned long)x << k lost the high byte (#676)`, `basic45 --compile: LOG10, LOG2 ... (#693)`.
- Open a pull request against `main` that names the issue. Before it: `make test` passes (and `make test-xemu`
  for a change to the compiler, the library or anything else that changes what programs do), and the docs say
  what the change does (below). CI (`.github/workflows/compile-check.yml`) builds the tree on several Linux
  distributions and macOS; `ide-tooling.yml` builds and tests `tools/ide`.
- Judge a run by its exit status and its `0 failed` line, not by a result line being there.

## Building

```bash
git clone --recurse-submodules https://github.com/CTalkobt/M65Compiler.git
cd M65Compiler
make check-deps          # what the build and the tests need, what is there, how to install the rest
make -j"$(nproc)"        # the tools, in bin/
make lib                 # the standard libraries, in lib/build/ (c45.lib, c45_zp.lib, c45_stack.lib)
```

C++17, GNU make, nothing else required. zlib is the git submodule `third_party/zlib`: in a clone made without
`--recurse-submodules`, `git submodule update --init third_party/zlib`. A change to `lib/` needs `make lib`
again; `make clean` removes `obj/`, `bin/` and all of `build/` (including `build/tmp`). The packages for each
distribution are in [README.md](README.md#dependencies) (from `src/deps/deps_table.sh`).

## Tests

| Command | What | Time |
|---------|------|------|
| `make test` | The steps of `src/test/test_steps.txt` in parallel (`TEST_JOBS`, default one per core): the unit and validation tests, the shell test scripts, the docs check, the tutorials; then the examples (`EXAMPLE_JOBS`). Steps that run programs use xemu when it is there and are skipped without it | about 6 minutes |
| `make test-xemu` | `src/test/test_xemu.sh`: the runtime tests in xemu, each in both calling conventions (`XEMU_JOBS`) | about 1.5 minutes |
| `make test-validation` | `bin/test_validation`, the C++ validation tests; `bin/test_validation parser linker` runs some, `-l` lists them, `TV_VERBOSE=1` shows the passes | seconds |
| `bash src/test/run_gte_all.sh` | The GCC torture tests in xemu (`-j N`; `GTE_CFLAGS="-fsigned-default -fimplicit-function-declaration -fno-zpcall"` for the stack convention) | minutes |
| `bash src/test/test_cc_interop.sh` | Calling-convention interop | seconds |
| `make lint-lib` | cl45 and al45 over the library: prints nothing when it is clean (keep it so) | seconds |

The runtime tests need `xemu-xmega65` on the `PATH` and the MEGA65 ROM
([troubleshooting](doc/guide/troubleshooting.md#xemu-and-the-mega65-rom)); the Docker image at the root
(`Dockerfile`) has xemu and the tree for both. **Runtime tests run on xemu, never on mmemu.**

Every test script and the library build pass an explicit **`-O2`** to cc45 (`CC="./bin/cc45 -O2"`), so a
personal `~/.config/m65/cc45.conf` (`-O1`, say) does not change what is tested. Do the same in a new script
and in commands you try by hand; a test's own later `-O` still wins.

The test outputs are under `build/test/`.

### A test step for `make test`

A step is one line of `src/test/test_steps.txt`, `title|command`:

```
Running signedness tests...|bash src/test/test_signedness.sh
```

The command passes by exiting 0. Steps run at the same time, so a step must leave other steps' files alone:
write into a directory of its own (`build/test/<name>/`) and never glob files another step may write
(`cc45 --save-temps` writes `temp_<pid>.s45` to the current directory). A step that runs xemu gets its own
preferences directory (`XEMU_PREF_DIR`, set by `run_parallel.sh`).

### A runtime test for `make test-xemu`

1. Write `src/test-resources/test_<feature>_xemu.c`. It stores its results from `$C0C0` and a `$AA` sentinel
   after them; by convention `01` is a pass for each check:

   ```c
   /* test_feature_xemu.c — what it checks
    *
    * Expected at $C0C0: 01 01 AA
    */
   #define R(i) (*(volatile unsigned char *)(0xC0C0 + (i)))
   int main(void) {
       R(0) = (2 + 2 == 4);
       R(1) = (sizeof(int) == 2);
       R(2) = 0xAA;
       return 0;
   }
   ```

2. Register it in `src/test/test_xemu.sh`, in both calling conventions, with `xemu_case`:

   ```bash
   for fmode in "zp|" "stack|-fno-zpcall"; do
       IFS='|' read -r fname fflags <<< "$fmode"
       xemu_case "FAIL: test_feature_xemu.c $fname (compilation/linking failed)" \
           "src/test-resources/test_feature_xemu.c" "build/test/test_feature_${fname}_xemu.prg" "$fflags" -- \
           "test_feature_$fname" "build/test/test_feature_${fname}_xemu.prg" "C0C0" 3 "01 01 AA"
   done
   ```

   Before `--`: `compile_link_test`'s arguments (the source, the `.prg`, cc45's flags, more sources to link).
   After it: `run_xemu_test`'s (a name, the `.prg`, the address, the number of bytes, the bytes expected, then
   xemu's options, such as `-8 disk.d81`). The program is compiled, linked and run as a background job.
   `XEMU_SYNC=1 run_xemu_test ...` instead, for a test that reads what the program wrote to its disk afterwards.

To run one program by hand:

```bash
./bin/cc45 -O2 src/test-resources/test_feature_xemu.c -o /tmp/t.prg
timeout 15 xemu-xmega65 -headless -besure -sleepless -prgmode 65 -prgexit -dumpmem /tmp/t.mem -prg /tmp/t.prg
xxd -s 0xC0C0 -l 3 /tmp/t.mem       # the results
xxd -s 0xFD -l 2 /tmp/t.mem         # the exit status: 0000 = 0; FFFF = __exit never reached; FEFF = abort()
```

### A GCC torture test that fails

Fix it; a failure that could be fixed but is not yet goes in `src/test/known_failures/gte.txt` (empty now). A
test that is not reasonable on the platform (a 32-bit `int`, too large, `mmap`) goes in
`src/test/known_ignore/gte.txt` with its reason.

## A change and its parts

- A language feature: lexer and parser (`Parser.cpp`, `AST.hpp`), the IR (`IRBuilder.cpp`, `IR.hpp`), code
  generation (`IRCodeGen.cpp`), a runtime test `test_<feature>_xemu.c` in both conventions, `doc/bin/cc45.md`.
- A library function: `lib/stdlib/<f>.c` or `lib/stdlib_asm/<f>.s45`, its declaration in `lib/include/<h>.h`,
  `make lib`, a runtime test, `doc/architecture/stdlib.md`; `make lint-lib` stays clean.
- A tool option: the tool's page in `doc/bin/`, and its `--help`.
- Diagnostics go through `include/Logger.hpp` (`LOG_INFO("cc45", ...)`), never `std::cout`; `-v` must not change
  a tool's output (`src/test/test_logging.sh`).
- `CHANGELOG.md` gets a line for a change a user would notice.

## Docs

- The reference pages are `doc/bin/<tool>.md` (one per tool; `check_docs.py` allows only those names) and
  `doc/architecture/`. The source wins over a page: when they disagree, fix the page.
- `python3 src/test/check_docs.py` checks the layout of `doc/` and every relative link (a `make test` step).
- The tutorials in `doc/guide/` are run as written by `src/test/test_guides.py` (also a step): HTML comments
  (`<!-- guide: run -->`, `<!-- guide: file NAME -->`...) mark the commands, their output and the screens to
  check ([doc/guide/README.md](doc/guide/README.md#kept-working)). A change to a tool's output can fail a
  tutorial: `python3 src/test/test_guides.py doc/guide/<page>.md` runs one.
- `make man` makes the man pages from `doc/bin/*.md` (pandoc).
