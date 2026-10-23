# libresidfp in m65compiler

reSIDfp, the SID (MOS 6581/8580) emulator of the libsidplayfp project, used
by `tune45 play` to play tunes on the PC (`src/main/TuneAudio.cpp`).

- Upstream: https://github.com/libsidplayfp/libresidfp
- Commit: c1791383ca843a855e5d4366da6a7277e34e801d (2026-10-13), version 1.3.0a
- License: GNU GPL version 2 or later (`COPYING`); m65compiler is GPL version 3
- Taken: `src/` (the emulator and `version.cc`, `resample/` without its test, `residfp/`), `COPYING`,
  `AUTHORS`, `README.md`; not the tests, docs or build files
- Generated here instead of by its `configure`:
  - `src/siddefs-fp.h` from `siddefs-fp.h.in`: `RESIDFP_BRANCH_HINTS` 1,
    `HAVE_BUILTIN_EXPECT` 1, `RESIDFP_INLINING` 1, `RESIDFP_INLINE` inline,
    the version string 1.3.0a
  - `src/residfp/sidversion.h` from `sidversion.h.in`: version 1, 3, 0
- No source file is changed. Built by the top-level Makefile into
  `lib45-residfp.a` (`-pthread`), linked only into tune45.
