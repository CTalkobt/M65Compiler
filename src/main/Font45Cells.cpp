// Font45Cells.cpp — font45's pre-rendered glyph sets and segment fonts
// (#466): glyphs drawn into the VIC-IV's full-colour (FCM) or nibble-colour
// (NCM) characters, coloured from a ramp of palette entries; and fonts
// generated from 7, 14 and 16-segment display maps.
#include "Font45.hpp"

#include <algorithm>
#include <cmath>
#include <set>

namespace font45 {

// --- FCM / NCM glyph sets ---------------------------------------------------------

std::vector<uint8_t> cellData(const Font& f, const CellOptions& o, bool proportional, int first, int last) {
    const bool ncm = o.ncm;
    const int charW = ncm ? 16 : 8;                     // a character's pixels across
    int extra = o.shadow >= 0 ? 1 : 0;                  // (the shadow's pixel below and right)
    int edge = o.edge >= 0 ? 1 : 0;                     // (the edge's pixel round the glyph)
    int maxW = 1;
    for (int c = first; c <= last; c++) {
        auto it = f.glyphs.find(c);
        if (it != f.glyphs.end()) maxW = std::max(maxW, it->second.w);
    }
    int pixW = maxW + extra + 2 * edge, pixH = f.height + extra + 2 * edge;
    int cols = (pixW + charW - 1) / charW, rows = (pixH + 7) / 8;
    int n = last - first + 1;
    std::vector<uint8_t> out = {'F', '4', 1, (uint8_t)(rows * 8), (uint8_t)(f.baseline + edge), (uint8_t)first, (uint8_t)last,
                                (uint8_t)(ncm ? 4 : 8), (uint8_t)((proportional ? 1 : 0) | (ncm ? 4 : 2)), (uint8_t)(cols * charW),
                                (uint8_t)cols, (uint8_t)rows};
    if (proportional)
        for (int c = first; c <= last; c++) {
            auto it = f.glyphs.find(c);
            int a = it == f.glyphs.end() ? 0 : it->second.advance + extra + 2 * edge;
            out.push_back((uint8_t)std::min(255, a));
        }
    // the characters start at a multiple of 64 from the font's start (put
    // where the VIC-IV can number them: the font 64-byte aligned)
    out.resize((out.size() + 63) & ~(size_t)63, 0);
    int levels = std::max(1, o.levels);
    for (int c = first; c <= last; c++) {
        auto it = f.glyphs.find(c);
        const Glyph* g = it == f.glyphs.end() ? nullptr : &it->second;
        // the glyph's pixels as palette entries (0: transparent)
        std::vector<int> pix((size_t)cols * charW * rows * 8, 0);
        auto lit = [&](int x, int y) { return g && g->at(x - edge, y - edge) >= 128; };
        for (int y = 0; y < rows * 8; y++)
            for (int x = 0; x < cols * charW; x++) {
                int v = 0;
                uint8_t cov = g ? g->at(x - edge, y - edge) : 0;
                int l = (cov * levels + 127) / 255;
                if (l) {
                    int ramp = o.gradient > 1 ? std::min(o.gradient - 1, y * o.gradient / std::max(1, pixH)) : 0;
                    v = o.base + ramp * levels + l - 1;
                } else if (o.edge >= 0 && (lit(x - 1, y) || lit(x + 1, y) || lit(x, y - 1) || lit(x, y + 1))) {
                    v = o.edge;
                } else if (o.shadow >= 0 && lit(x - 1, y - 1)) {
                    v = o.shadow;
                }
                pix[(size_t)y * cols * charW + x] = v;
            }
        // the characters down each column, then the next column
        for (int cx = 0; cx < cols; cx++)
            for (int cy = 0; cy < rows; cy++)
                for (int y = 0; y < 8; y++) {
                    const int* row = &pix[(size_t)(cy * 8 + y) * cols * charW + cx * charW];
                    if (ncm) {
                        for (int x = 0; x < 16; x += 2)   // (the left pixel the low nibble)
                            out.push_back((uint8_t)((row[x] & 15) | ((row[x + 1] & 15) << 4)));
                    } else {
                        for (int x = 0; x < 8; x++) out.push_back((uint8_t)row[x]);
                    }
                }
    }
    (void)n;
    return out;
}

// --- segment fonts -------------------------------------------------------------------

namespace {

// a segment from one point to another (in units of the glyph's width and height)
enum Seg { T1, T2, UR, LR, B1, B2, LL, UL, M1, M2, VU, VD, DUL, DUR, DLL, DLR, SEGS };

// the characters (16-segment terms: T is T1 T2, B is B1 B2, M is M1 M2)
struct Shape { char c; const char* segs; };
const Shape SIXTEEN[] = {
    {'0', "T UR LR B LL UL DUR DLL"}, {'1', "UR LR DUR"}, {'2', "T UR M LL B"}, {'3', "T UR LR B M2"},
    {'4', "UL UR M LR"}, {'5', "T UL M LR B"}, {'6', "T UL LL B LR M"}, {'7', "T UR LR"}, {'8', "T UR LR B LL UL M"},
    {'9', "T UL UR M LR B"}, {'A', "T UL UR M LL LR"}, {'B', "T UR LR B VU VD M2"}, {'C', "T UL LL B"},
    {'D', "T UR LR B VU VD"}, {'E', "T UL M1 LL B"}, {'F', "T UL M1 LL"}, {'G', "T UL LL B LR M2"},
    {'H', "UL LL UR LR M"}, {'I', "T B VU VD"}, {'J', "UR LR B LL"}, {'K', "UL LL M1 DUR DLR"}, {'L', "UL LL B"},
    {'M', "UL LL UR LR DUL DUR"}, {'N', "UL LL UR LR DUL DLR"}, {'O', "T UR LR B LL UL"}, {'P', "T UL UR M LL"},
    {'Q', "T UR LR B LL UL DLR"}, {'R', "T UL UR M LL DLR"}, {'S', "T UL M LR B"}, {'T', "T VU VD"},
    {'U', "UL LL B LR UR"}, {'V', "UL LL DLL DUR"}, {'W', "UL LL UR LR DLL DLR"}, {'X', "DUL DUR DLL DLR"},
    {'Y', "DUL DUR VD"}, {'Z', "T DUR DLL B"}, {'-', "M"}, {'+', "M VU VD"}, {'/', "DUR DLL"}, {'=', "M B"},
    {'_', "B"}, {'*', "M VU VD DUL DUR DLL DLR"}, {'(', "DUR DLR"}, {')', "DUL DLL"}, {'\'', "VU"},
    {'"', "UL VU"}, {'<', "DUR DLR"}, {'>', "DUL DLL"}, {' ', ""}};
// a 7-segment display's: a T, b UR, c LR, d B, e LL, f UL, g M (letters as they can be shown)
const Shape SEVEN[] = {
    {'0', "T UR LR B LL UL"}, {'1', "UR LR"}, {'2', "T UR M LL B"}, {'3', "T UR M LR B"}, {'4', "UL M UR LR"},
    {'5', "T UL M LR B"}, {'6', "T UL M LL LR B"}, {'7', "T UR LR"}, {'8', "T UR LR B LL UL M"},
    {'9', "T UR LR B UL M"}, {'A', "T UR LR LL UL M"}, {'B', "LR B LL UL M"}, {'C', "T B LL UL"},
    {'D', "UR LR B LL M"}, {'E', "T B LL UL M"}, {'F', "T LL UL M"}, {'G', "T UL LL B LR"}, {'H', "UR LR LL UL M"},
    {'I', "LL UL"}, {'J', "UR LR B"}, {'L', "B LL UL"}, {'N', "LR LL M"}, {'O', "LR B LL M"}, {'P', "T UR LL UL M"},
    {'R', "LL M"}, {'S', "T UL M LR B"}, {'T', "B LL UL M"}, {'U', "UR LR B LL UL"}, {'Y', "UR LR B UL M"},
    {'-', "M"}, {'_', "B"}, {'=', "M B"}, {' ', ""}};

std::set<int> parseSegs(const char* s, int kind) {
    std::set<int> r;
    std::string t;
    auto add = [&](const std::string& n) {
        if (n.empty()) return;
        static const std::pair<const char*, int> names[] = {
            {"T1", T1}, {"T2", T2}, {"UR", UR}, {"LR", LR}, {"B1", B1}, {"B2", B2}, {"LL", LL}, {"UL", UL}, {"M1", M1}, {"M2", M2},
            {"VU", VU}, {"VD", VD}, {"DUL", DUL}, {"DUR", DUR}, {"DLL", DLL}, {"DLR", DLR}};
        if (n == "T") { r.insert(T1); r.insert(T2); return; }
        if (n == "B") { r.insert(B1); r.insert(B2); return; }
        if (n == "M") { r.insert(M1); r.insert(M2); return; }
        for (auto& [nm, v] : names)
            if (n == nm) r.insert(v);
    };
    for (const char* p = s;; p++) {
        if (*p == ' ' || !*p) { add(t); t.clear(); if (!*p) break; }
        else t += *p;
    }
    (void)kind;
    return r;
}

}  // namespace

bool makeSegmentFont(int kind, int height, bool ghost, Font& f, std::string& err) {
    if (kind != 7 && kind != 14 && kind != 16) { err = "--segments is 7, 14 or 16"; return false; }
    if (height < 7 || height > 255) { err = "a segment font's --size is 7-255 pixels"; return false; }
    const int H = height, W = std::max(5, (int)std::lround(H * 0.6));
    const double t = std::max(1.0, H / 9.0);                  // a segment's thickness
    const double L = t / 2, R = W - 1 - t / 2, Top = t / 2, Bot = H - 1 - t / 2, Mid = (H - 1) / 2.0, Cx = (W - 1) / 2.0;
    struct Line { double x0, y0, x1, y1; };
    // 7 and 14 segments join the halves of the top, bottom (and, for 7, the middle)
    bool joinTB = kind != 16, joinM = kind == 7;
    Line lines[SEGS] = {
        {L, Top, joinTB ? R : Cx, Top}, {Cx, Top, R, Top}, {R, Top, R, Mid}, {R, Mid, R, Bot},
        {L, Bot, joinTB ? R : Cx, Bot}, {Cx, Bot, R, Bot}, {L, Mid, L, Bot}, {L, Top, L, Mid},
        {L, Mid, joinM ? R : Cx, Mid}, {Cx, Mid, R, Mid}, {Cx, Top, Cx, Mid}, {Cx, Mid, Cx, Bot},
        {Cx, Mid, L, Top}, {Cx, Mid, R, Top}, {Cx, Mid, L, Bot}, {Cx, Mid, R, Bot}};
    // the display's segments (joined halves are one segment, drawn once)
    std::set<int> all;
    if (kind == 7) all = {T1, UR, LR, B1, LL, UL, M1};
    else {
        all = {UR, LR, LL, UL, M1, M2, VU, VD, DUL, DUR, DLL, DLR, T1, B1};
        if (kind == 16) { all.insert(T2); all.insert(B2); }
    }
    auto canonical = [&](std::set<int> s) {
        // a joined segment's second half is its first
        std::set<int> r;
        for (int x : s) {
            if (joinTB && x == T2) x = T1;
            if (joinTB && x == B2) x = B1;
            if (joinM && x == M2) x = M1;
            if (all.count(x)) r.insert(x);
        }
        return r;
    };
    auto draw = [&](Glyph& g, int seg, uint8_t level) {
        Line ln = lines[seg];
        // shortened at both ends, so the segments stand apart
        double dx = ln.x1 - ln.x0, dy = ln.y1 - ln.y0, len = std::sqrt(dx * dx + dy * dy);
        double gap = std::min(len / 3, t * 0.75);
        double ux = len ? dx / len : 0, uy = len ? dy / len : 0;
        double ax = ln.x0 + ux * gap, ay = ln.y0 + uy * gap, bx = ln.x1 - ux * gap, by = ln.y1 - uy * gap;
        double r = t / 2;
        // 4 x 4 samples a pixel
        for (int y = 0; y < H; y++)
            for (int x = 0; x < W; x++) {
                int in = 0;
                for (int sy = 0; sy < 4; sy++)
                    for (int sx = 0; sx < 4; sx++) {
                        double px = x + (sx + 0.5) / 4 - 0.5, py = y + (sy + 0.5) / 4 - 0.5;
                        double vx = bx - ax, vy = by - ay, l2 = vx * vx + vy * vy;
                        double k = l2 ? std::clamp(((px - ax) * vx + (py - ay) * vy) / l2, 0.0, 1.0) : 0;
                        double qx = ax + k * vx - px, qy = ay + k * vy - py;
                        if (qx * qx + qy * qy <= r * r) in++;
                    }
                // (crisp: half the samples or more; no edge between a lit and a ghost level)
                if (in >= 8) {
                    uint8_t v = level;
                    uint8_t& p = g.px[(size_t)y * g.w + x];
                    p = std::max(p, v);
                }
            }
    };
    const int spacing = std::max(1, (int)std::lround(t));
    f.height = H;
    f.baseline = H;
    f.name = std::to_string(kind) + "-segment";
    const Shape* shapes = kind == 7 ? SEVEN : SIXTEEN;
    size_t count = kind == 7 ? sizeof SEVEN / sizeof SEVEN[0] : sizeof SIXTEEN / sizeof SIXTEEN[0];
    for (size_t i = 0; i < count; i++) {
        Glyph g;
        g.w = W + spacing; g.h = H; g.advance = W + spacing;
        g.px.assign((size_t)g.w * H, 0);
        std::set<int> on = canonical(parseSegs(shapes[i].segs, kind));
        // unlit segments a third as bright (two levels at 2 bpp: 1 and 3), then the lit ones
        if (ghost)
            for (int s : all)
                if (!on.count(s)) draw(g, s, 85);
        for (int s : on) draw(g, s, 255);
        f.glyphs[(unsigned char)shapes[i].c] = g;
        // (the lower case letters as the upper case)
        if (shapes[i].c >= 'A' && shapes[i].c <= 'Z') f.glyphs[shapes[i].c + 32] = g;
    }
    return true;
}

}  // namespace font45
