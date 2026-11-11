// Font45.cpp — font45's readers, transforms and writers (#466)
#include "Font45.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <sstream>

namespace font45 {

namespace {

std::string lower(std::string s) {
    for (auto& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}

// a glyph h rows high, w wide, all clear
Glyph blank(int w, int h) {
    Glyph g;
    g.w = w; g.h = h; g.advance = w;
    g.px.assign((size_t)w * h, 0);
    return g;
}

int hexval(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    c = (char)std::tolower((unsigned char)c);
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

}  // namespace

std::string formatOf(const std::string& path) {
    auto dot = path.find_last_of('.');
    std::string ext = dot == std::string::npos ? "" : lower(path.substr(dot + 1));
    if (ext == "bdf") return "bdf";
    if (ext == "psf" || ext == "psfu") return "psf";
    if (ext == "bin" || ext == "64c" || ext == "chr" || ext == "rom" || ext == "prg") return "charset";
    if (ext == "txt" || ext == "glyphs") return "text";
    if (ext == "ttf" || ext == "otf") return "truetype";
    if (ext == "png" || ext == "bmp" || ext == "gif" || ext == "jpg" || ext == "jpeg" || ext == "tga") return "image";
    if (ext == "c" || ext == "h" || ext == "s45" || ext == "asm") return "font45";
    return "";
}

// --- BDF ---------------------------------------------------------------

bool readBDF(const std::vector<uint8_t>& data, Font& f, std::string& err) {
    std::istringstream in(std::string(data.begin(), data.end()));
    std::string line;
    int ascent = -1, descent = -1, bbW = 0, bbH = 0, bbX = 0, bbY = 0;
    bool any = false;
    struct Raw { int code = -1, dw = -1, w = 0, h = 0, xo = 0, yo = 0; std::vector<std::string> rows; };
    std::vector<Raw> raws;
    Raw cur;
    bool inChar = false, inBitmap = false;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::istringstream ls(line);
        std::string kw;
        ls >> kw;
        if (inBitmap) {
            if (kw == "ENDCHAR") { inBitmap = false; inChar = false; raws.push_back(cur); continue; }
            cur.rows.push_back(kw);
            continue;
        }
        if (kw == "STARTFONT") any = true;
        else if (kw == "FONT") { std::getline(ls, f.name); if (!f.name.empty() && f.name[0] == ' ') f.name.erase(0, 1); }
        else if (kw == "FONTBOUNDINGBOX") ls >> bbW >> bbH >> bbX >> bbY;
        else if (kw == "FONT_ASCENT") ls >> ascent;
        else if (kw == "FONT_DESCENT") ls >> descent;
        else if (kw == "STARTCHAR") { cur = Raw(); inChar = true; }
        else if (inChar && kw == "ENCODING") ls >> cur.code;
        else if (inChar && kw == "DWIDTH") ls >> cur.dw;
        else if (inChar && kw == "BBX") ls >> cur.w >> cur.h >> cur.xo >> cur.yo;
        else if (inChar && kw == "BITMAP") inBitmap = true;
    }
    if (!any) { err = "not a BDF font (no STARTFONT)"; return false; }
    if (ascent < 0 || descent < 0) { ascent = bbH + bbY; descent = -bbY; }
    f.height = ascent + descent;
    f.baseline = ascent;
    if (f.height <= 0) { err = "BDF: no height (FONT_ASCENT / FONT_DESCENT or FONTBOUNDINGBOX)"; return false; }
    for (const auto& r : raws) {
        if (r.code < 0) continue;
        int adv = r.dw >= 0 ? r.dw : r.xo + r.w;
        int w = std::max(adv, r.xo + r.w);
        if (w <= 0) w = 1;
        Glyph g = blank(w, f.height);
        g.advance = adv;
        // the BBX's top row: its bottom yo above the baseline
        int top = f.baseline - (r.yo + r.h);
        for (int y = 0; y < r.h && y < (int)r.rows.size(); y++) {
            const std::string& hex = r.rows[(size_t)y];
            for (int x = 0; x < r.w; x++) {
                int nib = (size_t)(x / 4) < hex.size() ? hexval(hex[(size_t)(x / 4)]) : 0;
                if (nib < 0) nib = 0;
                if (nib & (8 >> (x % 4))) {
                    int gx = r.xo + x, gy = top + y;
                    if (gx >= 0 && gx < w && gy >= 0 && gy < f.height) g.px[(size_t)gy * w + gx] = 255;
                }
            }
        }
        f.glyphs[r.code] = g;
    }
    if (f.glyphs.empty()) { err = "BDF: no glyphs"; return false; }
    return true;
}

// --- PSF1 / PSF2 ------------------------------------------------------

bool readPSF(const std::vector<uint8_t>& d, Font& f, std::string& err) {
    size_t hdr, count, charsize;
    int w, h;
    if (d.size() >= 4 && d[0] == 0x36 && d[1] == 0x04) {
        int mode = d[2];
        charsize = d[3];
        count = (mode & 1) ? 512 : 256;
        hdr = 4; w = 8; h = (int)charsize;
    } else if (d.size() >= 32 && d[0] == 0x72 && d[1] == 0xB5 && d[2] == 0x4A && d[3] == 0x86) {
        auto u32 = [&](size_t o) { return (size_t)d[o] | (size_t)d[o + 1] << 8 | (size_t)d[o + 2] << 16 | (size_t)d[o + 3] << 24; };
        hdr = u32(8); count = u32(16); charsize = u32(20); h = (int)u32(24); w = (int)u32(28);
    } else {
        err = "not a PSF font (no PSF1 / PSF2 magic)";
        return false;
    }
    int rowBytes = (w + 7) / 8;
    if (h <= 0 || w <= 0 || charsize < (size_t)(rowBytes * h) || hdr + count * charsize > d.size()) {
        err = "PSF: the glyphs do not fit the file";
        return false;
    }
    f.height = h;
    f.baseline = h - h / 8;          // (PSF has no baseline: the usual one)
    for (size_t c = 0; c < count; c++) {
        Glyph g = blank(w, h);
        const uint8_t* p = d.data() + hdr + c * charsize;
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
                if (p[y * rowBytes + x / 8] & (0x80 >> (x % 8))) g.px[(size_t)y * w + x] = 255;
        f.glyphs[(int)c] = g;
    }
    return true;
}

// --- C64 / C65 charsets ------------------------------------------------

bool readCharset(const std::vector<uint8_t>& d, Font& f, std::string& err) {
    size_t off = 0, n = d.size();
    if (n == 2048 + 2 || n == 4096 + 2) { off = 2; n -= 2; }
    if (n != 2048 && n != 4096) {
        err = "a charset is 2048 or 4096 bytes (with or without a load address), not " + std::to_string(d.size());
        return false;
    }
    f.height = 8;
    f.baseline = 7;
    for (size_t c = 0; c < n / 8; c++) {
        Glyph g = blank(8, 8);
        for (int y = 0; y < 8; y++)
            for (int x = 0; x < 8; x++)
                if (d[off + c * 8 + y] & (0x80 >> x)) g.px[(size_t)y * 8 + x] = 255;
        f.glyphs[(int)c] = g;
    }
    return true;
}

bool readRom(const std::vector<uint8_t>& d, bool upperSet, Font& f, std::string& err) {
    if (d.size() != 131072) { err = "a MEGA65 ROM image is 128 KB, not " + std::to_string(d.size()) + " bytes"; return false; }
    // the ROM is $20000-$3FFFF: its character sets at $29000 (upper case / graphics) and $29800 (lower / upper)
    size_t at = upperSet ? 0x9000 : 0x9800;
    std::vector<uint8_t> set(d.begin() + (long)at, d.begin() + (long)at + 2048);
    return readCharset(set, f, err);
}

bool readFont45(const std::vector<uint8_t>& d, Font& f, std::string& err) {
    size_t o = 0;
    if (d.size() > 4 && !(d[0] == 'F' && d[1] == '4') && d[2] == 'F' && d[3] == '4') o = 2;   // a load address
    if (d.size() < o + 12 || d[o] != 'F' || d[o + 1] != '4') { err = "not a font font45 wrote (no \"F4\")"; return false; }
    if (d[o + 2] != 1) { err = "font45's format " + std::to_string(d[o + 2]) + ": this font45 reads format 1"; return false; }
    int height = d[o + 3], baseline = d[o + 4], first = d[o + 5], last = d[o + 6], bpp = d[o + 7], flags = d[o + 8];
    int cellw = d[o + 9], rowbytes = d[o + 10];
    if (flags & 6) { err = "a glyph set (FCM / NCM characters): not read back"; return false; }
    if (last < first || !height || (bpp != 1 && bpp != 2 && bpp != 4)) { err = "a font45 header that makes no sense"; return false; }
    bool prop = flags & 1;
    size_t widths = o + 12, glyphs = widths + (prop ? (size_t)(last - first + 1) : 0);
    size_t need = glyphs + (size_t)(last - first + 1) * height * rowbytes;
    if (d.size() < need) { err = "the font is cut short (" + std::to_string(d.size()) + " of " + std::to_string(need) + " bytes)"; return false; }
    f.height = height;
    f.baseline = baseline;
    int levels = (1 << bpp) - 1;
    for (int c = first; c <= last; c++) {
        int adv = prop ? d[widths + (size_t)(c - first)] : cellw;
        Glyph g = blank(cellw, height);
        g.advance = adv;
        bool any = false;
        const uint8_t* src = &d[glyphs + (size_t)(c - first) * height * rowbytes];
        for (int y = 0; y < height; y++)
            for (int x = 0; x < cellw; x++) {
                int bit = x * bpp;
                int v = (src[(size_t)y * rowbytes + (size_t)(bit / 8)] >> (8 - bpp - bit % 8)) & levels;
                g.px[(size_t)y * cellw + x] = (uint8_t)(v * 255 / levels);
                if (v) any = true;
            }
        // (a proportional font's code without a glyph has width 0)
        if (prop && !adv && !any) continue;
        f.glyphs[c] = g;
    }
    return true;
}

bool readFont45Source(const std::string& t, Font& f, std::string& err) {
    // the numbers of the array's initializer (C: 0x46, ...; assembly: .byte $46, ...)
    std::vector<uint8_t> bytes;
    size_t open = t.find('{');
    bool c = open != std::string::npos;
    size_t i = c ? open + 1 : 0, end = c ? t.find('}', open) : t.size();
    if (end == std::string::npos) end = t.size();
    while (i < end) {
        if (!c) {
            // assembly: the operands of .byte lines
            size_t b = t.find(".byte", i);
            if (b == std::string::npos || b >= end) break;
            size_t e = t.find('\n', b);
            if (e == std::string::npos) e = end;
            std::string line = t.substr(b + 5, e - b - 5);
            for (size_t k = 0; k < line.size();) {
                if (line[k] == '$' || std::isdigit((unsigned char)line[k])) {
                    size_t used = 0;
                    long v = line[k] == '$' ? std::stol(line.substr(k + 1), &used, 16) + 0 : std::stol(line.substr(k), &used, 0);
                    bytes.push_back((uint8_t)v);
                    k += used + (line[k] == '$' ? 1 : 0);
                } else if (line[k] == ';') {
                    break;
                } else {
                    k++;
                }
            }
            i = e + 1;
            continue;
        }
        if (std::isdigit((unsigned char)t[i])) {
            size_t used = 0;
            long v = std::stol(t.substr(i, 16), &used, 0);
            bytes.push_back((uint8_t)v);
            i += used;
        } else if (t.compare(i, 2, "/*") == 0) {
            size_t e = t.find("*/", i + 2);
            i = e == std::string::npos ? end : e + 2;
        } else {
            i++;
        }
    }
    if (bytes.empty()) { err = "no bytes found (font45's C array or .byte lines)"; return false; }
    return readFont45(bytes, f, err);
}

// --- hand-drawn glyphs ---------------------------------------------------

bool readGlyphText(const std::string& text, Font& f, std::string& err) {
    std::istringstream in(text);
    std::string line;
    int lineNo = 0, maxH = 0, baseline = -1;
    while (std::getline(in, line)) {
        lineNo++;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        auto hash = line.find('#');
        // (a comment: # at the line's start or after a space, not a pixel)
        if (hash != std::string::npos && (hash == 0 || line[hash - 1] == ' ') &&
            line.find_first_not_of(" \t") == hash) continue;
        if (line.find_first_not_of(" \t") == std::string::npos) continue;
        auto colon = line.find(':');
        if (colon == std::string::npos) { err = "line " + std::to_string(lineNo) + ": no ':' (code: rows)"; return false; }
        std::string key = line.substr(0, colon), rest = line.substr(colon + 1);
        while (!key.empty() && key.back() == ' ') key.pop_back();
        while (!key.empty() && key.front() == ' ') key.erase(0, 1);
        if (key == "baseline") { baseline = std::atoi(rest.c_str()); continue; }
        int code;
        if (key.size() == 3 && key.front() == '\'' && key.back() == '\'') code = (unsigned char)key[1];
        else if (key.size() == 1) code = (unsigned char)key[0];
        else if (key[0] == '$') code = (int)std::strtol(key.c_str() + 1, nullptr, 16);
        else code = (int)std::strtol(key.c_str(), nullptr, 0);
        std::istringstream rs(rest);
        std::vector<std::string> rows;
        std::string r;
        while (rs >> r) rows.push_back(r);
        if (rows.empty()) { err = "line " + std::to_string(lineNo) + ": no rows"; return false; }
        int w = 0;
        for (const auto& x : rows) w = std::max(w, (int)x.size());
        Glyph g = blank(w, (int)rows.size());
        for (size_t y = 0; y < rows.size(); y++)
            for (size_t x = 0; x < rows[y].size(); x++) {
                char c = rows[y][x];
                if (c == '1' || c == '#' || c == 'X' || c == 'x' || c == '*') g.px[y * w + x] = 255;
                else if (c != '0' && c != '.' && c != ' ' && c != '-') {
                    err = "line " + std::to_string(lineNo) + ": '" + std::string(1, c) + "' in a row (0/1 or ./#)";
                    return false;
                }
            }
        maxH = std::max(maxH, g.h);
        f.glyphs[code] = g;
    }
    if (f.glyphs.empty()) { err = "no glyphs"; return false; }
    f.height = maxH;
    f.baseline = baseline >= 0 ? baseline : maxH;
    // every glyph on the font's cell (shorter ones at the top)
    for (auto& [c, g] : f.glyphs)
        if (g.h < maxH) { g.px.resize((size_t)g.w * maxH, 0); g.h = maxH; }
    return true;
}

// --- transforms --------------------------------------------------------

void keepRange(Font& f, int lo, int hi) {
    for (auto it = f.glyphs.begin(); it != f.glyphs.end();)
        it = (it->first < lo || it->first > hi) ? f.glyphs.erase(it) : std::next(it);
}

void remap(Font& f, int from, int to) {
    auto it = f.glyphs.find(from);
    if (it == f.glyphs.end()) return;
    f.glyphs[to] = it->second;
}

void makeBold(Font& f) {
    for (auto& [c, g] : f.glyphs) {
        Glyph b = blank(g.w + 1, g.h);
        b.advance = g.advance + 1;
        for (int y = 0; y < g.h; y++)
            for (int x = 0; x < b.w; x++)
                b.px[(size_t)y * b.w + x] = std::max(g.at(x, y), g.at(x - 1, y));
        g = b;
    }
}

void makeItalic(Font& f) {
    for (auto& [c, g] : f.glyphs) {
        int extra = (f.baseline + 3) / 4;
        Glyph b = blank(g.w + extra, g.h);
        b.advance = g.advance;
        for (int y = 0; y < g.h; y++) {
            int sh = (f.baseline - y) / 4;
            if (sh < 0) sh = 0;
            for (int x = 0; x < g.w; x++)
                if (x + sh < b.w) b.px[(size_t)y * b.w + x + sh] = g.at(x, y);
        }
        g = b;
    }
}

void makeOutline(Font& f) {
    for (auto& [c, g] : f.glyphs) {
        Glyph b = blank(g.w + 2, g.h);
        b.advance = g.advance + 2;
        for (int y = 0; y < g.h; y++)
            for (int x = 0; x < b.w; x++) {
                if (g.at(x - 1, y) >= 128) continue;     // the glyph itself: clear
                bool near = false;
                for (int dy = -1; dy <= 1 && !near; dy++)
                    for (int dx = -1; dx <= 1 && !near; dx++)
                        if (g.at(x - 1 + dx, y + dy) >= 128) near = true;
                if (near) b.px[(size_t)y * b.w + x] = 255;
            }
        g = b;
    }
}

// --- writers -----------------------------------------------------------

namespace {

// the source code(s) for a charset index, by order (the first one present)
std::vector<int> sourceCodes(int i, Order order) {
    if (order == Order::Ascii) return {i};
    if (order == Order::ScreenCode) {
        int s = i & 0x7F;
        if (s == 0) return {'@'};
        if (s >= 1 && s <= 26) return {'a' + s - 1};
        if (s == 27) return {'['};
        if (s == 28) return {0xA3, '#'};          // £
        if (s == 29) return {']'};
        if (s == 30) return {0x2191, '^'};        // ↑
        if (s == 31) return {0x2190, '_'};        // ←
        if (s >= 32 && s <= 63) return {s};
        if (s >= 65 && s <= 90) return {'A' + s - 65};
        return {};
    }
    // PETSCII, the lower/upper case set as printed
    if (i >= 32 && i <= 63) return {i};
    if (i == 64) return {'@'};
    if (i >= 65 && i <= 90) return {'a' + i - 65};
    if (i == 91) return {'['};
    if (i == 92) return {0xA3, '#'};
    if (i == 93) return {']'};
    if (i == 94) return {0x2191, '^'};
    if (i == 95) return {0x2190, '_'};
    if (i >= 193 && i <= 218) return {'A' + i - 193};
    return {};
}

const Glyph* findGlyph(const Font& f, const std::vector<int>& codes) {
    for (int c : codes) {
        auto it = f.glyphs.find(c);
        if (it != f.glyphs.end()) return &it->second;
    }
    return nullptr;
}

}  // namespace

void reencode(Font& f, Order order) {
    if (order == Order::Ascii) return;
    std::map<int, Glyph> out;
    // (screen codes 128-255 are the inverse of 0-127: a font has no inverse glyphs)
    int n = order == Order::ScreenCode ? 128 : 256;
    for (int i = 0; i < n; i++)
        if (const Glyph* g = findGlyph(f, sourceCodes(i, order))) out[i] = *g;
    f.glyphs = std::move(out);
}

void decodeCharset(Font& f, Order order) {
    if (order == Order::Ascii) return;
    std::map<int, Glyph> out;
    int n = order == Order::ScreenCode ? 128 : 256;
    for (auto& [i, g] : f.glyphs) {
        if (i >= n) continue;
        auto codes = sourceCodes(i, order);
        if (!codes.empty() && !out.count(codes[0])) out[codes[0]] = g;
    }
    f.glyphs = std::move(out);
}

std::vector<uint8_t> charset8x8(const Font& f, Order order, int count, bool center) {
    std::vector<uint8_t> out((size_t)count * 8, 0);
    for (int i = 0; i < count; i++) {
        int idx = i;
        bool inverse = false;
        // (screen codes 128-255: the inverse of 0-127, as the ROM's charsets;
        // a second 256: the same order again)
        if (order == Order::ScreenCode) { inverse = (i & 0x80) != 0; idx = i & 0x7F; }
        const Glyph* g = findGlyph(f, sourceCodes(order == Order::ScreenCode ? idx : (i & 0xFF), order));
        // (centred: by the glyph's inked columns)
        int shift = 0;
        if (g && center) {
            int lo = 8, hi = -1;
            for (int y = 0; y < 8; y++)
                for (int x = 0; x < g->w && x < 8; x++)
                    if (g->at(x, y) >= 128) { lo = std::min(lo, x); hi = std::max(hi, x); }
            if (hi >= lo) shift = (8 - (hi - lo + 1)) / 2 - lo;
        }
        for (int y = 0; y < 8; y++) {
            uint8_t b = 0;
            if (g)
                for (int x = 0; x < 8; x++)
                    if (g->at(x - shift, y) >= 128) b |= (uint8_t)(0x80 >> x);
            out[(size_t)i * 8 + y] = inverse ? (uint8_t)~b : b;
        }
    }
    return out;
}

std::vector<uint8_t> fontData(const Font& f, int bpp, bool proportional, int first, int last, int fixedWidth) {
    int cellW = fixedWidth;
    if (!cellW)
        for (int c = first; c <= last; c++) {
            auto it = f.glyphs.find(c);
            if (it != f.glyphs.end()) cellW = std::max(cellW, it->second.w);
        }
    if (cellW <= 0) cellW = 1;
    int rowBytes = (cellW * bpp + 7) / 8;
    std::vector<uint8_t> out = {'F', '4', 1, (uint8_t)f.height, (uint8_t)f.baseline, (uint8_t)first, (uint8_t)last,
                                (uint8_t)bpp, (uint8_t)(proportional ? 1 : 0), (uint8_t)cellW, (uint8_t)rowBytes, 0};
    if (proportional)
        for (int c = first; c <= last; c++) {
            auto it = f.glyphs.find(c);
            out.push_back(it == f.glyphs.end() ? 0 : (uint8_t)std::min(255, std::max(0, it->second.advance)));
        }
    int levels = (1 << bpp) - 1;
    for (int c = first; c <= last; c++) {
        auto it = f.glyphs.find(c);
        const Glyph* g = it == f.glyphs.end() ? nullptr : &it->second;
        for (int y = 0; y < f.height; y++) {
            std::vector<uint8_t> row((size_t)rowBytes, 0);
            for (int x = 0; x < cellW && g; x++) {
                int cov = g->at(x, y);
                int v = bpp == 1 ? (cov >= 128) : (cov * levels + 127) / 255;
                int bit = x * bpp;                       // from the row's top bit
                row[(size_t)(bit / 8)] |= (uint8_t)(v << (8 - bpp - bit % 8));
            }
            out.insert(out.end(), row.begin(), row.end());
        }
    }
    return out;
}

namespace {
std::string hexBytes(const std::vector<uint8_t>& d, const char* pre, const char* sep, const char* lineStart) {
    std::string s;
    char b[8];
    for (size_t i = 0; i < d.size(); i++) {
        if (i % 16 == 0) s += lineStart;
        std::snprintf(b, sizeof b, "%s%02X", pre, d[i]);
        s += b;
        if (i + 1 < d.size()) s += (i % 16 == 15) ? std::string(sep).substr(0, 1) + "\n" : sep;
    }
    return s + "\n";
}
std::string upper(std::string s) {
    for (auto& c : s) c = (char)std::toupper((unsigned char)c);
    return s;
}
}  // namespace

std::string asC(const std::vector<uint8_t>& data, const std::string& name, const std::string& what) {
    // (a glyph set's characters must be 64-byte aligned: the array is)
    bool cells = data.size() >= 12 && data[0] == 'F' && data[1] == '4' && (data[8] & 6);
    return "/* " + name + ": " + what + " (font45) */\n#include \"" + name + ".h\"\n\n" +
           std::string(cells ? "_Alignas(64) " : "") + "const unsigned char " + name +
           "[" + std::to_string(data.size()) + "] = {\n" + hexBytes(data, "0x", ", ", "    ") + "};\n";
}

std::string asHeader(const std::vector<uint8_t>& data, const std::string& name, const std::string& what,
                     bool isFont) {
    std::string N = upper(name);
    std::string s = "/* " + name + ": " + what + " (font45) */\n#ifndef " + N + "_H\n#define " + N + "_H\n\n";
    s += "#define " + N + "_SIZE " + std::to_string(data.size()) + "\n";
    if (isFont && data.size() >= 12) {
        s += "#define " + N + "_HEIGHT " + std::to_string(data[3]) + "\n";
        s += "#define " + N + "_BASELINE " + std::to_string(data[4]) + "\n";
        s += "#define " + N + "_FIRST " + std::to_string(data[5]) + "\n";
        s += "#define " + N + "_LAST " + std::to_string(data[6]) + "\n";
        s += "#define " + N + "_BPP " + std::to_string(data[7]) + "\n";
        if (data[8] & 6) {
            s += "#define " + N + "_COLS " + std::to_string(data[10]) + "\n";
            s += "#define " + N + "_ROWS " + std::to_string(data[11]) + "\n";
        }
        s += "/* font_from(&f, (unsigned long)(unsigned int)" + name + ") (<font.h>) */\n";
    }
    s += "extern const unsigned char " + name + "[" + std::to_string(data.size()) + "];\n\n#endif\n";
    return s;
}

std::string asAsm(const std::vector<uint8_t>& data, const std::string& name, const std::string& what) {
    bool cells = data.size() >= 12 && data[0] == 'F' && data[1] == '4' && (data[8] & 6);
    return "; " + name + ": " + what + " (font45)\n    .global " + name + "\n" + (cells ? "    .align 64\n" : "") + name + ":\n" +
           hexBytes(data, "$", ", ", "    .byte ");
}

std::vector<uint8_t> asBinary(const std::vector<uint8_t>& data, int loadAt) {
    std::vector<uint8_t> out;
    if (loadAt >= 0) { out.push_back((uint8_t)loadAt); out.push_back((uint8_t)(loadAt >> 8)); }
    out.insert(out.end(), data.begin(), data.end());
    return out;
}

std::string describe(const Font& f) {
    std::ostringstream o;
    int lo = f.glyphs.empty() ? 0 : f.glyphs.begin()->first, hi = f.glyphs.empty() ? 0 : f.glyphs.rbegin()->first;
    int maxW = 0;
    for (const auto& [c, g] : f.glyphs) maxW = std::max(maxW, g.w);
    if (!f.name.empty()) o << "name:     " << f.name << "\n";
    o << "glyphs:   " << f.glyphs.size() << " (codes " << lo << "-" << hi << ")\n";
    o << "height:   " << f.height << " (baseline " << f.baseline << ")\n";
    o << "widest:   " << maxW << "\n";
    o << "widths:  ";
    int n = 0;
    for (const auto& [c, g] : f.glyphs) {
        if (n++ % 12 == 0) o << "\n ";
        char b[32];
        std::snprintf(b, sizeof b, " %d:%d", c, g.advance);
        o << b;
    }
    o << "\n";
    return o.str();
}

}  // namespace font45
