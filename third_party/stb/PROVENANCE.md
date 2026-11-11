# stb in m65compiler

Three of Sean Barrett's single-header libraries, used by `font45` (#466,
`src/main/Font45Stb.cpp`):

- `stb_truetype.h` (v1.26): TrueType / OpenType fonts rasterized at a pixel
  size (`font45 convert font.ttf --size 12`)
- `stb_image.h` (v2.30): PNG, BMP, GIF and other images read as glyph sheets
- `stb_image_write.h`: `font45 preview`'s PNG

- Upstream: https://github.com/nothings/stb
- Commit: 4e9b368bfc071e97206ea5c6adba68dd0cbf8897
- License: public domain (Unlicense) or MIT, the reader's choice (`LICENSE`)
- Taken: those three headers and `LICENSE`; nothing else
- No file is changed. Their implementations are compiled in one file,
  `src/main/Font45Stb.cpp`, with their warnings off there.
