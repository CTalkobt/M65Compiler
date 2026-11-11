// Font45.hpp — font45's fonts (#466): glyphs as coverage bitmaps, read from
// bitmap font formats and written as the MEGA65's charsets and the
// library's proportional fonts (<font.h>).
#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace font45 {

// A glyph: w x h pixels, each a coverage 0 (empty) .. 255 (full); advance
// is the pen's step after it (the width, unless the source says otherwise)
struct Glyph {
    int w = 0, h = 0, advance = 0;
    std::vector<uint8_t> px;          // row by row, w * h
    uint8_t at(int x, int y) const { return x < 0 || y < 0 || x >= w || y >= h ? 0 : px[(size_t)y * w + x]; }
};

// A font: its glyphs by code (Unicode / ASCII as the source gives them),
// the line height and the baseline (rows from the top to the baseline)
struct Font {
    int height = 0, baseline = 0;
    std::map<int, Glyph> glyphs;
    std::string name;
};

// --- readers (false and err set on an error) ---
// BDF (X11 bitmap fonts): every glyph's BBX placed on the font's cell
bool readBDF(const std::vector<uint8_t>& data, Font& f, std::string& err);
// PSF1 / PSF2 (Linux console fonts); codes 0.. in order (the Unicode
// table, if any, maps them)
bool readPSF(const std::vector<uint8_t>& data, Font& f, std::string& err);
// a C64 / C65 charset: 8x8 characters, 2048 or 4096 bytes (with or without
// a two-byte load address); codes are the character numbers (screen codes)
bool readCharset(const std::vector<uint8_t>& data, Font& f, std::string& err);
// hand-drawn glyphs: lines "<code or 'c'>: 01110 10001 ..." (rows of 0/1
// or ./#, separated by spaces), as xmas/tools/font5x7.py writes them
bool readGlyphText(const std::string& text, Font& f, std::string& err);

// TrueType / OpenType (stb_truetype, not hinted): ASCII, Latin-1 and the
// arrows and pi PETSCII has, at a line height of pixelSize pixels; coverage
// 0-255 from the rasterizer
bool readTrueType(const std::vector<uint8_t>& data, int pixelSize, Font& f, std::string& err);
// an image (PNG, BMP, GIF, ... : stb_image) as a glyph sheet: cells of
// cellW x cellH on a grid, rows first (or columns), codes from first; or
// (marker) one row of glyphs under a top row whose marks start each glyph.
// A pixel's coverage is its brightness's difference from the commonest
// brightness (transparent pixels are background), scaled to the image's largest
struct ImageOptions {
    int cellW = 0, cellH = 0, first = 32, baseline = -1;
    bool columns = false, marker = false, proportional = false, invert = false;   // invert: the commonest colour is the ink
};
bool readImage(const std::vector<uint8_t>& data, const ImageOptions& o, Font& f, std::string& err);
// a MEGA65 ROM image (128 KB): its character set at $29800 (lower / upper
// case; upper: $29000, upper case / graphics), 256 characters in screen-code
// order, as readCharset reads one
bool readRom(const std::vector<uint8_t>& data, bool upperSet, Font& f, std::string& err);
// font45's own fonts (format 1): a binary file (with or without a load
// address), or the C or assembly font45 wrote (its bytes read back)
bool readFont45(const std::vector<uint8_t>& data, Font& f, std::string& err);
bool readFont45Source(const std::string& text, Font& f, std::string& err);

// the format by the name's extension (.bdf .psf .psfu .bin .64c .chr .txt
// .ttf .otf .png .bmp .gif .jpg .tga .rom .c .h .s45 .asm); "" when unknown
std::string formatOf(const std::string& path);

// the preview: every glyph on a grid and a sample text below, scale pixels
// a pixel, coverage shaded in levels steps (0: as it is), as PNG
bool writePreview(const Font& f, int levels, int scale, const std::string& text, std::vector<uint8_t>& png, std::string& err);

// --- transforms ---
// keep only codes lo..hi
void keepRange(Font& f, int lo, int hi);
// a code's glyph under another code (--map from=to)
void remap(Font& f, int from, int to);
// derived variants: bold (one pixel wider, smeared right), italic (sheared:
// a row shifted right by its distance from the baseline / 4), outline (the
// pixels next to the glyph, the glyph itself cleared)
void makeBold(Font& f);
void makeItalic(Font& f);
void makeOutline(Font& f);

// --- writers ---
// The order of a 256-character charset: by ASCII code, by the C64's screen
// codes (the lower/upper case set: 1-26 a-z, 65-90 A-Z) or by PETSCII (the
// same set as printed: 65-90 a-z, 193-218 A-Z)
enum class Order { Ascii, ScreenCode, Petscii };
// the 8x8 1 bpp charset (count 256 or 512 characters, 8 bytes each, bit 7 the
// leftmost pixel): each glyph's top-left 8x8, set where coverage >= 128
std::vector<uint8_t> charset8x8(const Font& f, Order order, int count);
// a font's glyphs moved from character codes (ASCII, Unicode) to the codes of
// an encoding: a C program's PETSCII literals ('A' is 193), screen codes
void reencode(Font& f, Order order);
// a charset read in position order (readCharset) to character codes: its
// position i is the character `order` puts at i (screen codes: 0-127)
void decodeCharset(Font& f, Order order);

// A pre-rendered glyph set (format 1, flags bit 1 FCM / bit 2 NCM): each
// glyph drawn into the VIC-IV's characters, cols x rows of them (FCM 8 x 8
// pixels a byte each, NCM 16 x 8 a nibble each, the left pixel the low
// nibble; 64 bytes a character), numbered down each column; the glyphs'
// characters from the next multiple of 64 after the header and widths.
// Header: 3 the rows of pixels (rows * 8), 7 bpp 8 / 4, 9 the cell's width
// in pixels, 10 cols, 11 rows. A coverage level n (of levels) is palette
// entry base + n - 1 (gradient: the ramp chosen by the row, ramp r's
// entries base + r * levels ...), 0 is transparent; edge / shadow (>= 0)
// the entries of a pixel round the glyph / below and right of it
struct CellOptions {
    bool ncm = false;
    int base = 1, levels = 1, gradient = 0, shadow = -1, edge = -1;
};
std::vector<uint8_t> cellData(const Font& f, const CellOptions& o, bool proportional, int first, int last);

// a segment display's font (7, 14 or 16 segments) height pixels high: the
// digits, the letters it can show (lower case as upper) and some signs;
// lit segments coverage 255, the unlit ones (ghost) 85: levels 3 and 1 at 2 bpp
bool makeSegmentFont(int kind, int height, bool ghost, Font& f, std::string& err);

// A proportional (or fixed) font for <font.h> (format 1):
//   0..1 "F4", 2 version (1), 3 height, 4 baseline, 5 first, 6 last, 7 bpp,
//   8 flags (bit 0 proportional), 9 cell width (the widest glyph),
//   10 row bytes (ceil(cell width * bpp / 8)), 11 reserved 0;
//   then (proportional) a width per code first..last;
//   then each code's glyph: height rows of row bytes, the leftmost pixel in
//   the top bits of a row's first byte; a pixel's bits the coverage
//   (bpp 1: >= 128; 2: 0-3; 4: 0-15)
std::vector<uint8_t> fontData(const Font& f, int bpp, bool proportional, int first, int last,
                              int fixedWidth = 0);

// the data as C (an array and its macros; a header too), ca45 assembly, or
// binary with an optional load address
std::string asC(const std::vector<uint8_t>& data, const std::string& name, const std::string& what);
std::string asHeader(const std::vector<uint8_t>& data, const std::string& name, const std::string& what,
                     bool isFont);
std::string asAsm(const std::vector<uint8_t>& data, const std::string& name, const std::string& what);
std::vector<uint8_t> asBinary(const std::vector<uint8_t>& data, int loadAt);   // loadAt < 0: none

// a summary for font45 list: the codes, the cell, every glyph's width
std::string describe(const Font& f);

}  // namespace font45
