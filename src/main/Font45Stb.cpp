// Font45Stb.cpp — font45's readers and writer that use stb (#466):
// TrueType / OpenType fonts (stb_truetype), images as glyph sheets
// (stb_image) and the preview PNG (stb_image_write). The stb
// implementations are compiled here, and only here.
#include "Font45.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wcast-qual"
#endif
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "../../third_party/stb/stb_truetype.h"
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#define STBI_NO_STDIO
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#include "../../third_party/stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_STATIC
#define STBI_WRITE_NO_STDIO
#include "../../third_party/stb/stb_image_write.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

namespace font45 {

namespace {

Glyph blankGlyph(int w, int h) {
    Glyph g;
    g.w = w; g.h = h; g.advance = w;
    g.px.assign((size_t)w * h, 0);
    return g;
}

}  // namespace

// --- TrueType / OpenType -----------------------------------------------------

bool readTrueType(const std::vector<uint8_t>& data, int pixelSize, Font& f, std::string& err) {
    if (pixelSize < 4 || pixelSize > 255) { err = "--size is 4-255 pixels"; return false; }
    stbtt_fontinfo info;
    int offset = stbtt_GetFontOffsetForIndex(data.data(), 0);
    if (offset < 0 || !stbtt_InitFont(&info, data.data(), offset)) { err = "not a TrueType / OpenType font stb_truetype reads"; return false; }
    // the size is the line's pixel height: ascent to descent
    float scale = stbtt_ScaleForPixelHeight(&info, (float)pixelSize);
    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);
    int base = (int)std::lround(ascent * scale);
    int height = std::max(1, (int)std::lround((ascent - descent) * scale));
    f.height = height;
    f.baseline = base;
    // ASCII and Latin-1, and what PETSCII has besides (the arrows, pi)
    std::vector<int> codes;
    for (int c = 32; c < 256; c++) if (c < 127 || c >= 160) codes.push_back(c);
    for (int c : {0x2190, 0x2191, 0x3C0}) codes.push_back(c);
    for (int c : codes) {
        int index = stbtt_FindGlyphIndex(&info, c);
        if (!index && c != ' ') continue;
        int adv, lsb;
        stbtt_GetGlyphHMetrics(&info, index, &adv, &lsb);
        int x0, y0, x1, y1;
        stbtt_GetGlyphBitmapBox(&info, index, scale, scale, &x0, &y0, &x1, &y1);
        int advance = std::max(0, (int)std::lround(adv * scale));
        // the glyph from the pen's x (a left overhang is cut), on the line's cell
        int left = std::max(0, x0);
        int w = std::max({advance, x1, 1});
        Glyph g = blankGlyph(w, height);
        g.advance = advance;
        int bw = x1 - x0, bh = y1 - y0;
        if (bw > 0 && bh > 0) {
            std::vector<uint8_t> bm((size_t)bw * bh);
            stbtt_MakeGlyphBitmap(&info, bm.data(), bw, bh, bw, scale, scale, index);
            for (int y = 0; y < bh; y++)
                for (int x = 0; x < bw; x++) {
                    int gx = x0 + x, gy = base + y0 + y;
                    if (gx < left || gx >= w || gy < 0 || gy >= height) continue;
                    g.px[(size_t)gy * w + gx] = std::max(g.px[(size_t)gy * w + gx], bm[(size_t)y * bw + x]);
                }
        }
        f.glyphs[c] = g;
    }
    if (f.glyphs.empty()) { err = "no glyphs for ASCII or Latin-1"; return false; }
    // the name table's family name, when it is plain ASCII
    int len = 0;
    const char* n = stbtt_GetFontNameString(&info, &len, STBTT_PLATFORM_ID_MAC, STBTT_MAC_EID_ROMAN, STBTT_MAC_LANG_ENGLISH, 1);
    if (n && len > 0 && len < 64) f.name.assign(n, (size_t)len);
    return true;
}

// --- images --------------------------------------------------------------------

bool readImage(const std::vector<uint8_t>& data, const ImageOptions& o, Font& f, std::string& err) {
    int w, h, n;
    unsigned char* img = stbi_load_from_memory(data.data(), (int)data.size(), &w, &h, &n, 4);
    if (!img) { err = std::string("not an image stb_image reads (") + stbi_failure_reason() + ")"; return false; }
    // the background: the commonest brightness (transparent pixels count as
    // background); a pixel's coverage its difference from it, scaled to the
    // image's largest
    auto lum = [&](int x, int y) {
        const unsigned char* p = img + ((size_t)y * w + x) * 4;
        int l = (p[0] * 299 + p[1] * 587 + p[2] * 114) / 1000;
        return p[3] < 128 ? -1 : l;   // (transparent: background)
    };
    int count[256] = {0};
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            int l = lum(x, y);
            if (l >= 0) count[l]++;
        }
    int bg = (int)(std::max_element(count, count + 256) - count);
    auto cover = [&](int x, int y) {
        int l = lum(x, y);
        if (l < 0) return 0;
        return std::abs(l - bg);
    };
    int maxc = 1;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) maxc = std::max(maxc, cover(x, y));
    auto level = [&](int x, int y) {
        uint8_t v = (uint8_t)std::min(255, cover(x, y) * 255 / maxc);
        return o.invert ? (uint8_t)(255 - v) : v;
    };

    if (o.marker) {
        // a proportional sheet: row 0 marks where each glyph starts, the glyphs below it
        f.height = h - 1;
        std::vector<int> starts;
        for (int x = 0; x < w; x++)
            if (level(x, 0) >= 128 && (x == 0 || level(x - 1, 0) < 128)) starts.push_back(x);
        if (starts.empty()) { stbi_image_free(img); err = "--marker: no marks in the top row"; return false; }
        for (size_t i = 0; i < starts.size(); i++) {
            int x0 = starts[i], x1 = i + 1 < starts.size() ? starts[i + 1] : w;
            Glyph g = blankGlyph(x1 - x0, h - 1);
            for (int y = 1; y < h; y++)
                for (int x = x0; x < x1; x++) g.px[(size_t)(y - 1) * g.w + (x - x0)] = level(x, y);
            f.glyphs[o.first + (int)i] = g;
        }
    } else {
        if (o.cellW <= 0 || o.cellH <= 0) { stbi_image_free(img); err = "an image needs --grid WxH (or --marker)"; return false; }
        int cols = w / o.cellW, rows = h / o.cellH;
        if (!cols || !rows) { stbi_image_free(img); err = "--grid is larger than the image"; return false; }
        f.height = o.cellH;
        for (int i = 0; i < cols * rows; i++) {
            int cx = o.columns ? i / rows : i % cols, cy = o.columns ? i % rows : i / cols;
            Glyph g = blankGlyph(o.cellW, o.cellH);
            int right = 0;
            for (int y = 0; y < o.cellH; y++)
                for (int x = 0; x < o.cellW; x++) {
                    uint8_t v = level(cx * o.cellW + x, cy * o.cellH + y);
                    g.px[(size_t)y * g.w + x] = v;
                    if (v >= 128) right = std::max(right, x + 1);
                }
            // proportional: the lit columns and one more (a blank glyph: half the cell)
            if (o.proportional) g.advance = right ? right + 1 : (o.cellW + 1) / 2;
            f.glyphs[o.first + i] = g;
        }
    }
    stbi_image_free(img);
    f.baseline = o.baseline >= 0 ? o.baseline : f.height;
    return true;
}

// --- the preview ------------------------------------------------------------------

bool writePreview(const Font& f, int levels, int scale, const std::string& text, std::vector<uint8_t>& png, std::string& err) {
    if (f.glyphs.empty()) { err = "no glyphs"; return false; }
    if (scale < 1 || scale > 16) { err = "--scale is 1-16"; return false; }
    int cellW = 1;
    for (const auto& [c, g] : f.glyphs) cellW = std::max(cellW, g.w);
    const int perRow = 16, pad = 2;
    int gridRows = ((int)f.glyphs.size() + perRow - 1) / perRow;
    int textW = 0;
    for (unsigned char c : text) {
        auto it = f.glyphs.find(c);
        if (it != f.glyphs.end()) textW += std::max(it->second.advance, 1);
    }
    int W = std::max(perRow * (cellW + pad) + pad, textW + 2 * pad);
    int H = gridRows * (f.height + pad) + pad + (text.empty() ? 0 : f.height + 2 * pad);
    std::vector<uint8_t> rgb((size_t)W * H * 3);
    // a dark blue page, the glyphs' cells a shade lighter, the pixels white
    for (size_t i = 0; i < rgb.size(); i += 3) { rgb[i] = 16; rgb[i + 1] = 24; rgb[i + 2] = 64; }
    auto shade = [&](uint8_t v) { return levels > 0 ? (uint8_t)(std::lround(std::round(v * levels / 255.0) * 255.0 / levels)) : v; };
    auto put = [&](int x, int y, uint8_t r, uint8_t g, uint8_t b) {
        if (x < 0 || y < 0 || x >= W || y >= H) return;
        uint8_t* p = &rgb[((size_t)y * W + x) * 3];
        p[0] = r; p[1] = g; p[2] = b;
    };
    auto drawGlyph = [&](const Glyph& g, int ox, int oy, bool cell) {
        for (int y = 0; y < f.height; y++)
            for (int x = 0; x < (cell ? cellW : g.w); x++) {
                uint8_t v = shade(g.at(x, y));
                if (v) {
                    // the page's blue blended to white by the coverage
                    put(ox + x, oy + y, (uint8_t)(16 + (255 - 16) * v / 255), (uint8_t)(24 + (255 - 24) * v / 255),
                        (uint8_t)(64 + (255 - 64) * v / 255));
                } else if (cell) {
                    put(ox + x, oy + y, x < g.advance ? 32 : 24, x < g.advance ? 44 : 34, x < g.advance ? 96 : 80);
                }
            }
    };
    int i = 0;
    for (const auto& [c, g] : f.glyphs) {
        drawGlyph(g, pad + (i % perRow) * (cellW + pad), pad + (i / perRow) * (f.height + pad), true);
        i++;
    }
    int x = pad, y = gridRows * (f.height + pad) + 2 * pad;
    for (unsigned char c : text) {
        auto it = f.glyphs.find(c);
        if (it == f.glyphs.end()) continue;
        drawGlyph(it->second, x, y, false);
        x += std::max(it->second.advance, 1);
    }
    // scaled up, each pixel a square
    std::vector<uint8_t> big((size_t)W * scale * H * scale * 3);
    for (int yy = 0; yy < H * scale; yy++)
        for (int xx = 0; xx < W * scale; xx++)
            std::memcpy(&big[((size_t)yy * W * scale + xx) * 3], &rgb[((size_t)(yy / scale) * W + xx / scale) * 3], 3);
    png.clear();
    auto sink = [](void* ctx, void* d, int size) {
        auto* v = static_cast<std::vector<uint8_t>*>(ctx);
        v->insert(v->end(), (uint8_t*)d, (uint8_t*)d + size);
    };
    if (!stbi_write_png_to_func(sink, &png, W * scale, H * scale, 3, big.data(), W * scale * 3)) { err = "the PNG was not written"; return false; }
    return true;
}

}  // namespace font45
