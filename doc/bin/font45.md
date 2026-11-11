# font45 — Fonts for the MEGA65

`font45` converts a font (bitmap, TrueType or an image of glyphs) into one of two forms (#466):

- an **8x8 charset** for the VIC-IV's text mode, 256 or 512 characters, in ASCII, screen-code or PETSCII order;
- a **`<font.h>` font**: 1, 2 or 4 bits a pixel, fixed or proportional, any height up to 255 rows. The library
  measures it and draws it into a bitmap or into characters ([stdlib.md](../architecture/stdlib.md#fonts-fonth));
- a **glyph set** (`--fcm`, `--ncm`): each glyph pre-drawn into the VIC-IV's full-colour or nibble-colour
  characters, coloured from a ramp of palette entries, which `font_put_cells` puts on the screen (large digits,
  titles, score displays).

`--segments 7|14|16` makes a segment display's font instead of reading one.

Both are written as C (an array and its header), ca45 assembly or a binary file.

## Usage

```bash
font45 convert <font> [options] -o <output>
font45 list <font> [options]
font45 preview <font> [options] -o <image.png>
font45 -V | --version
font45 -h | --help
```

### Inputs

The format comes from the extension, or from `--format`:

| Extension | `--format` | What it is |
|-----------|------------|------------|
| `.bdf` | `bdf` | X11 BDF. The cell comes from `FONT_ASCENT` + `FONT_DESCENT` (or the bounding box), and each glyph's `BBX` is placed by its offset from the baseline. `ENCODING` gives the codes and `DWIDTH` the advance |
| `.psf`, `.psfu` | `psf` | Linux console fonts, PSF1 (8 wide, 256 or 512 glyphs) and PSF2 (any width, its header's glyph size). Glyph *n* is code *n* (the Unicode table is not read) |
| `.bin`, `.64c`, `.chr`, `.rom` | `charset` | A C64 / C65 charset: 8x8, 8 bytes a character, 256 or 512 of them. Read with or without a two-byte load address in front. Its order is `--from`'s: screen codes by default (the ROM's charsets; 0-127, as 128-255 are their inverse) |
| `.txt`, `.glyphs` | `text` | Hand-drawn glyphs, below |
| `.ttf`, `.otf` | `truetype` | TrueType and OpenType fonts, rasterized by `stb_truetype` (not hinted) at `--size` pixels: the line's height from ascent to descent. It takes ASCII, Latin-1 and the arrows and π PETSCII has. Coverage is anti-aliased, 0–255 |
| `.png`, `.bmp`, `.gif`, `.jpg`, `.tga` | `image` | A sheet of glyphs (`stb_image`). With `--grid WxH`, cells W x H pixels, rows first (`--order columns`: down the columns first), codes from `--first` (32). With `--marker`, one row of glyphs whose top row marks where each glyph starts (proportional sheets). The background is the image's commonest brightness (transparent pixels are background; `--invert` when the commonest is the ink), and a pixel's coverage is its difference from it |
| a 128 KB `.rom` | `rom` | A MEGA65 ROM image: its lower / upper case set at `$29800` (`--upper`: the upper case / graphics set at `$29000`), in screen-code order |
| `.bin`, `.prg` with `F4`; `.c`, `.h`, `.s45`, `.asm` | `font45` | font45's own fonts: a binary (with or without a load address), or the C or assembly it wrote. They keep their codes (no `--encoding`), so a font can be converted again, listed or previewed |

A **hand-drawn** font has one glyph to a line: the code, a colon, then the rows separated by spaces. Each row is
`0`/`1`, or `.`/`#` (`x`, `X`, `*` are set too; `-` is clear). The code can be a character (`A`, `' '`), a
number (`65`, `0x41`) or hex (`$41`). `baseline: N` gives the baseline (the default is the tallest glyph's height).
Lines starting with `#` are comments. A glyph's width is its longest row.

```
baseline: 5
A: 0110 1001 1111 1001 1001
I: 1 1 1 1 1
' ': 00 00 00 00 00
```

### Options

| Option | Description |
|--------|-------------|
| `-o FILE` | The output. A `.c` (with a `.h` beside it) or `.h` name gives C; `.s45`, `.asm` or `.s` gives assembly; anything else (`.bin`, `.prg`) gives binary |
| `--c`, `--asm`, `--bin` | The output form, whatever the file is called |
| `--name NAME` | The array's or label's name. The default is the output's base name |
| `--charset ORDER` | Write an 8x8 charset in this order instead of a font: `ascii`, `screencode` or `petscii` (below) |
| `--count N` | With `--charset`: 256 (the default) or 512 characters |
| `--encoding E` | A `<font.h>` font's codes: `petscii` (the default: cc45's plain literals, `'A'` is 193, `'a'` 65), `ascii` (a program under `#pragma encoding ascii`, or `a"…"` strings) or `screencode` |
| `--bpp 1\|2\|4` | Write a `<font.h>` font with this many bits a pixel (the default is 1) |
| `--proportional` | Store a width for each glyph. The default is fixed: every glyph is the widest glyph's width |
| `--fixed W` | A cell W pixels wide (wider glyphs are cut) |
| `--range A-B` | Keep only codes A to B (in the font's encoding: `--range 193-218` is PETSCII's A-Z). They also become the font's first and last code (codes with no glyph are blank) |
| `--map FROM=TO` | Give code TO the glyph of code FROM (repeatable; in the font's encoding, applied before `--range`) |
| `--bold` | Bold: each pixel also drawn one to the right (one wider) |
| `--italic` | Italic: rows slant right, one pixel for each four rows above the baseline |
| `--outline` | Outline: the pixels around the glyph, the glyph itself cleared (two wider) |
| `--load-at ADDR` | Binary output: a two-byte load address in front (for `cbm_load` or [`load_far`](../architecture/stdlib.md#loading-files-anywhere-loadh)) |
| `--format F` | The input's format: `bdf`, `psf`, `charset`, `text`, `truetype`, `image`, `rom`, `font45` |
| `--size N` | TrueType: the line's height in pixels, 4–255 (default 16) |
| `--grid WxH`, `--first N`, `--order rows\|columns` | An image: the cells, the first cell's code, the cells' order |
| `--marker` | An image: one row of glyphs, each starting at a mark in the top row |
| `--invert` | An image: the commonest colour is the ink, not the background |
| `--baseline N` | An image: the baseline's row (default: the cell's bottom) |
| `--upper` | A ROM image: the upper case / graphics set |
| `--fcm`, `--ncm` | A glyph set: full-colour characters (8x8, a byte a pixel) or nibble-colour ones (16x8, 4 bits a pixel, the left pixel the low nibble) |
| `--palette B,L` | A glyph set: coverage levels 1 to L as palette entries B to B+L−1 (default 1,1; 0 is transparent). With `--ncm` the entries are a character's 16 (0–15) |
| `--gradient N` | A glyph set: N ramps down the glyph, ramp r's entries B + r·L … (a vertical gradient) |
| `--shadow C`, `--edge C` | A glyph set: a drop shadow (one pixel below and right) / an outline in palette entry C |
| `--segments 7\|14\|16` | No input: a segment display's font, `--size` pixels high (7–255): the digits, the letters the display can show (lower case as upper case) and some signs. Unlit segments are drawn dim, at a third of the coverage (`--bpp 2`'s level 1, the lit ones 3), unless `--no-ghost` |
| `--pack ADDR` | The output is a pack45 stream that unpacks to ADDR, for `font_load` |
| `--scale N`, `--text "..."` | `preview`: pixels a font pixel (1–16, default 4), the sample line (default "The quick brown fox") |
| `--from ORDER` | A charset input's order: `screencode` (the default), `petscii` or `ascii` (as font45 `--charset ascii` writes) |
| `-v` … `-vvvv` | Log on stderr as `[font45:LEVEL] …` |

The glyphs of BDF, PSF and hand-drawn fonts have character codes (ASCII, or Unicode in BDF). A `<font.h>` font moves them to its encoding's codes. Characters the encoding does not have (`{`, `|`, `}`, `~` and the backtick in PETSCII) are dropped. Codes between the glyphs are stored as blank glyphs, so a PETSCII font of the space to `Z` holds 187 codes. Use `--range` to keep fewer.

`list` prints the font's height, baseline, glyph count, codes and widths in their character codes.

`preview` writes a PNG: every glyph on a grid (each cell's advance a lighter shade) and the sample line below
it. With `--bpp` the coverage is shown in that many levels, as the MEGA65 would draw it. A font45 font in PETSCII
shows the sample text in PETSCII's letters.

### Charset orders

| Order | Character *i* is the glyph of |
|-------|-------------------------------|
| `ascii` | code *i* |
| `screencode` | the C64's lower/upper-case set: screen code *i* (1–26 `a`–`z`, 65–90 `A`–`Z`, 32–63 as ASCII). 128–255 are 0–127 inverted, as in the ROM's charsets |
| `petscii` | PETSCII code *i*: 65–90 `a`–`z`, 193–218 `A`–`Z` (the shifted set) |

With `--count 512` the second 256 characters repeat the first. Glyphs taller than 8 rows lose their lower rows
(a warning); glyphs narrower than 8 are placed at the left.

## The font format (format 1)

All values are bytes.

| Offset | Value |
|--------|-------|
| 0–1 | `F4` (`$46 $34`) |
| 2 | 1, the version |
| 3 | height: rows per glyph |
| 4 | baseline: the row below the glyphs' feet, counted from the top |
| 5, 6 | first, last: the codes the font holds |
| 7 | bpp: 1, 2 or 4 |
| 8 | flags: bit 0 = proportional |
| 9 | cellw: the cell's width in pixels |
| 10 | rowbytes: bytes per glyph row, `(cellw * bpp + 7) / 8` |
| 11 | 0 (reserved) |
| 12 | proportional only: `last - first + 1` widths (each glyph's advance) |
| … | the glyphs, first to last, each `height * rowbytes` bytes, row by row |

The leftmost pixel of a row is in the top bits of its first byte. A pixel's value is its coverage: 0 is blank and
1, 3 or 15 is full. Pixels in between come from a source with grey levels.

## Glyph sets (`--fcm`, `--ncm`)

A glyph set keeps format 1's header, with flags bit 1 (FCM, bpp 8) or bit 2 (NCM, bpp 4):

| Offset | Value |
|--------|-------|
| 3 | the rows of pixels: rows × 8 |
| 7 | 8 (FCM) or 4 (NCM) |
| 9 | the cell's width in pixels |
| 10, 11 | cols, rows: a glyph's characters across and down |
| 12 | proportional: the widths, as format 1 |
| the next multiple of 64 | each glyph's cols × rows characters, 64 bytes each, numbered down each column, then the next column |

The characters start 64-byte aligned from the font's start, and the font must be 64-byte aligned (font45's C array
is `_Alignas(64)`, its assembly `.align 64`), so character *n* of the set is at the font's address + 64·*n*. That is
what the VIC-IV needs to show it, and what `font_put_cells` writes to the screen. A glyph is drawn from its cell's
top-left, coverage level *n* as palette entry B + *n* − 1 (0 transparent), the edge and the shadow round it.

## Example

```bash
font45 convert title.bdf --bpp 1 --proportional -o title.c
cc45 -O2 game.c title.c -o game.prg
```

```c
#include <font.h>
#include "title.h"

font_t t;
font_from(&t, (unsigned long)(unsigned int)title);
font_draw_bitmap(&t, 0x40000UL, 320, 10, 20, "Hello");    /* palette entry 1 */
```

An 8x8 charset for the text screen:

```bash
font45 convert term.psf --charset screencode -o term.bin --load-at 0x3000
```

```c
font_install(0x3000UL, 2048, 0x40000UL);       /* (after loading it to $3000) */
font_select(0x40000UL);
```

## Third-party code

`stb_truetype.h`, `stb_image.h` and `stb_image_write.h` (`third_party/stb`, public domain or MIT; the commit is in its
`PROVENANCE.md`), compiled into `src/main/Font45Stb.cpp` only.

## Known problems

- TrueType is not hinted: small sizes are softer than a hinted rasterizer makes them. 2 bpp keeps them readable.
- Drawing is C, a pixel at a time; assembly routines are a later phase of #466.
- PSF's Unicode table is ignored: glyphs keep their position as their code.
