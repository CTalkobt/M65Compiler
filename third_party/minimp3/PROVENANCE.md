# minimp3 in m65compiler

minimp3, a small MP3 decoder in one header, used by `pack45` to read MP3
sounds (`--suggest-rate`, `--sound`; `src/main/PackAudio.cpp`).

- Upstream: https://github.com/lieff/minimp3
- Commit: ea99364f61c14656440e8d77e9c233ccf3124633
- License: CC0 1.0 Universal, public domain (`LICENSE`)
- Taken: `minimp3.h`, `minimp3_ex.h`, `LICENSE`, `README.md`; not the tests,
  the player or the build files
- No file is changed. Compiled into `src/main/PackAudio.cpp` with
  `MINIMP3_IMPLEMENTATION` and `MINIMP3_NO_STDIO` (it decodes from memory;
  its file mapping is not built), its warnings off there.
