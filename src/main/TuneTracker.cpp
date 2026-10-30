// TuneTracker.cpp — tune45 import of an XM, S3M or IT into a MOD's
// structures (TuneTracker.hpp)
#include "TuneTracker.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>

namespace {

using Bytes = std::vector<uint8_t>;

const double kMidRate = 3546894.6 / 428;   // the rate period 428 plays a sample at (8287.14 Hz)

int u8(const Bytes& f, size_t at) { return at < f.size() ? f[at] : 0; }
int le16(const Bytes& f, size_t at) { return at + 1 < f.size() ? f[at] | f[at + 1] << 8 : 0; }
uint32_t le32(const Bytes& f, size_t at) {
    return at + 3 < f.size() ? (uint32_t)f[at] | (uint32_t)f[at + 1] << 8 | (uint32_t)f[at + 2] << 16 | (uint32_t)f[at + 3] << 24 : 0;
}
bool has(const Bytes& f, size_t at, size_t n) { return at <= f.size() && n <= f.size() - at; }

std::string text(const Bytes& f, size_t at, size_t n) {
    std::string o;
    for (size_t k = 0; k < n && at + k < f.size() && f[at + k]; k++)
        o += std::isprint(f[at + k]) ? (char)f[at + k] : ' ';
    while (!o.empty() && o.back() == ' ') o.pop_back();
    while (!o.empty() && o[0] == ' ') o.erase(0, 1);
    return o;
}

// a note semis semitones above the middle (ProTracker's C-2) as its period
uint16_t periodOf(double semis) {
    long p = std::lround(428.0 / std::pow(2.0, semis / 12));
    return (uint16_t)std::max(1L, std::min(4095L, p));
}

// --- the samples ----------------------------------------------------------

using Pcm = std::vector<int32_t>;   // a channel, at a 16-bit scale

// n samples of plain PCM at at (as many as the file has)
Pcm readPcm(const Bytes& f, size_t at, size_t n, bool is16, bool isSigned, bool delta, bool bigEndian = false) {
    Pcm o;
    size_t w = is16 ? 2 : 1;
    n = std::min(n, at < f.size() ? (f.size() - at) / w : 0);
    o.reserve(n);
    int32_t acc = 0;
    for (size_t k = 0; k < n; k++) {
        size_t p = at + k * w;
        int32_t v;
        if (is16) {
            int raw = bigEndian ? f[p] << 8 | f[p + 1] : f[p] | f[p + 1] << 8;
            v = isSigned ? (int16_t)raw : raw - 32768;
            if (delta) v = acc = (int16_t)(acc + v);
        } else {
            v = isSigned ? (int8_t)f[p] : f[p] - 128;
            if (delta) v = acc = (int8_t)(acc + v);
            v *= 256;
        }
        o.push_back(v);
    }
    return o;
}

// to 8-bit signed (rounded), the mean of two channels
std::vector<int8_t> to8(const Pcm& l, const Pcm* r) {
    size_t n = r ? std::min(l.size(), r->size()) : l.size();
    std::vector<int8_t> o(n);
    for (size_t k = 0; k < n; k++) {
        double v = r ? std::floor((l[k] + (*r)[k] + 256) / 512.0) : std::floor((l[k] + 128) / 256.0);
        o[k] = (int8_t)std::max(-128.0, std::min(127.0, v));
    }
    return o;
}

// the loop [start, end): the data cut at its end (never heard after); a
// ping-pong loop unrolled, forward then back, so a forward loop plays it
void setLoop(ModSample& s, size_t start, size_t end, bool pingpong) {
    end = std::min(end, s.data.size());
    if (start + 2 > end) return;   // (none, or too short)
    s.data.resize(end);
    if (pingpong && end - start > 2)
        for (size_t k = end - 2; k > start; k--) s.data.push_back(s.data[k]);
    s.loopStart = (uint32_t)start;
    s.loopLen = (uint32_t)(s.data.size() - start);
}

// IT 2.14 / 2.15 compressed samples: blocks (a 16-bit length, then a bit
// stream, LSB first) of 0x8000 samples (8-bit) or 0x4000 (16-bit); a
// value's width changes in the stream (a short value of one pattern, a
// medium one in a range at the top, or the top bit at the widest); the
// values are deltas, summed once (2.14) or twice (2.15)
struct Bits {
    const uint8_t* p;
    size_t n, at = 0;
    int bit = 0;
    bool over = false;
    uint32_t get(int w) {
        uint32_t v = 0;
        for (int k = 0; k < w; k++) {
            if (at >= n) { over = true; return v; }
            v |= (uint32_t)(p[at] >> bit & 1) << k;
            if (++bit == 8) { bit = 0; at++; }
        }
        return v;
    }
};

Pcm itUnpack(const Bytes& f, size_t& at, size_t count, bool is16, bool it215) {
    Pcm o;
    const int top = is16 ? 16 : 8, widest = top + 1;
    while (count > 0 && has(f, at, 2)) {
        size_t blen = (size_t)le16(f, at);
        at += 2;
        Bits b{f.data() + at, std::min(blen, f.size() - at)};
        size_t todo = std::min(count, (size_t)(is16 ? 0x4000 : 0x8000)), done = 0;
        int width = widest;
        int32_t d1 = 0, d2 = 0;
        while (done < todo) {
            if (width < 1 || width > widest) break;   // (a broken stream)
            uint32_t v = b.get(width);
            if (b.over) break;
            if (width < 7) {   // a short value: 100..0 changes the width (the next 3 or 4 bits)
                if (v == 1u << (width - 1)) {
                    int nw = (int)b.get(is16 ? 4 : 3) + 1;
                    width = nw < width ? nw : nw + 1;
                    continue;
                }
            } else if (width < widest) {   // a medium one: the top few values change it
                uint32_t border = ((is16 ? 0xFFFFu : 0xFFu) >> (widest - width)) - (is16 ? 8 : 4);
                if (v > border && v <= border + (is16 ? 16 : 8)) {
                    int nw = (int)(v - border);
                    width = nw < width ? nw : nw + 1;
                    continue;
                }
            } else if (v & (1u << top)) {   // the widest: its top bit
                width = (int)((v + 1) & 0xFF);
                continue;
            }
            int32_t s;
            if (width < top) {
                int sh = 32 - width;
                s = (int32_t)(v << sh) >> sh;
            } else s = is16 ? (int16_t)v : (int8_t)v;
            d1 = is16 ? (int16_t)(d1 + s) : (int8_t)(d1 + s);
            d2 = is16 ? (int16_t)(d2 + d1) : (int8_t)(d2 + d1);
            int32_t x = it215 ? d2 : d1;
            o.push_back(is16 ? x : x * 256);
            done++;
        }
        at += blen;
        if (done < todo || !blen) {   // (a block cut short or broken: silence to its end, and no more)
            o.resize(o.size() + (todo - done), 0);
            break;
        }
        count -= todo;
    }
    return o;
}

// --- the cells ------------------------------------------------------------

// a cell as the format has it
struct Raw {
    int note = -1;                  // -1: none; -2: off (a cut, an off, a fade); else the format's
    int ins = 0, vol = -1, cmd = 0, param = 0;
};

const int kOff = -2;

// an effect a note cut must not displace (the song's flow)
bool flow(const ModCell& c) {
    return c.effect == 0xB || c.effect == 0xD || c.effect == 0xF || (c.effect == 0xE && ((c.param >> 4) == 6 || (c.param >> 4) == 0xE));
}
bool empty(const ModCell& c) { return c.effect == 0 && c.param == 0; }
void cut(ModCell& c) {
    c.period = 0;
    if (!flow(c)) { c.effect = 0xE; c.param = 0xC0; }
}

// the order list: (S3M, IT) "+++" (254) skipped, ended at "---" (255);
// patterns that are not dropped; at[k]: where raw position k went (for B)
struct Orders {
    std::vector<uint8_t> list;
    std::vector<int> at;
    uint8_t to(int raw) const { return (uint8_t)std::min(255, raw < (int)at.size() ? at[(size_t)raw] : (int)list.size()); }
};
Orders orders(const Bytes& f, size_t start, size_t n, size_t patterns, bool marks = true) {
    Orders o;
    bool ended = false;
    for (size_t k = 0; k < n; k++) {
        o.at.push_back((int)o.list.size());
        int p = u8(f, start + k);
        if (marks && p == 255) ended = true;
        if (ended || (marks && p == 254) || (size_t)p >= patterns) continue;
        o.list.push_back((uint8_t)p);
    }
    return o;
}

// the patterns in the order the song first plays them, then the rest (an
// effect's memory carried that way)
std::vector<size_t> playOrder(const std::vector<uint8_t>& order, size_t np) {
    std::vector<size_t> o;
    std::vector<bool> seen(np, false);
    for (uint8_t p : order) if (p < np && !seen[p]) { seen[p] = true; o.push_back(p); }
    for (size_t p = 0; p < np; p++) if (!seen[p]) o.push_back(p);
    return o;
}

// a channel's memory, as the song goes
struct Chan {
    int ins = 0, note = -1;         // the last instrument, and note
    int d = 0, ef = 0, g = 0, h = 0, j = 0, q = 0, s3m = 0;   // effect memory
};

// S3M's and IT's letter effects (A = 1) as ProTracker's
void letterEffect(const Raw& r, Chan& c, bool it, bool linkG, const Orders& ord, ModCell& o) {
    int p = r.param;
    // the memories: S3M's one for most; IT's D/K/L, E/F (G too, linked), J, Q
    int& mD = it ? c.d : c.s3m;
    int& mEF = it ? c.ef : c.s3m;
    int& mG = it && linkG ? c.ef : c.g;
    int& mJ = it ? c.j : c.s3m;
    int& mQ = it ? c.q : c.s3m;
    auto recall = [&](int& mem) { if (p) mem = p; else p = mem; };
    auto set = [&](int e, int v) { o.effect = (uint8_t)e; o.param = (uint8_t)v; };
    auto volSlide = [&](bool fineOk) {   // D's param: A, or a fine slide (EA, EB)
        recall(mD);
        int hi = p >> 4, lo = p & 15;
        if (lo == 0xF && hi) { if (fineOk) set(0xE, 0xA0 | hi); return 0; }
        if (hi == 0xF && lo) { if (fineOk) set(0xE, 0xB0 | lo); return 0; }
        return hi ? hi << 4 : lo;
    };
    auto porta = [&](int e) {            // E, F: 2, 1, or fine (E2, E1)
        recall(mEF);
        int hi = p >> 4, lo = p & 15;
        if (hi == 0xF) { if (lo) set(0xE, e << 4 | lo); }
        else if (hi == 0xE) { if (lo >> 2) set(0xE, e << 4 | lo >> 2); }   // (extra fine: a quarter)
        else set(e, p);
    };
    switch (r.cmd) {
    case 1: if (p) set(p < 32 ? 0xF : 0x10, p); break;                          // A: speed (over 31: TuneMod's 0x10)
    case 2: set(0xB, ord.to(p)); break;                                       // B: position
    case 3: { int row = std::min(p, 159); set(0xD, it ? (row / 10) << 4 | row % 10 : p); break; }   // C: break
    case 4: { int v = volSlide(true); if (o.effect == 0 && v) set(0xA, v); break; }   // D
    case 5: porta(2); break;                                                  // E: down
    case 6: porta(1); break;                                                  // F: up
    case 7: recall(mG); set(0x3, p); break;                                   // G
    case 8: {                                                                 // H (each half remembered)
        int sp = p >> 4 ? p >> 4 : c.h >> 4, dp = p & 15 ? p & 15 : c.h & 15;
        c.h = sp << 4 | dp;
        set(0x4, c.h);
        break;
    }
    case 10: recall(mJ); if (p) set(0x0, p); break;                           // J: arpeggio
    case 11: set(0x6, volSlide(false)); break;                                // K
    case 12: set(0x5, volSlide(false)); break;                                // L
    case 15: set(0x9, p); break;                                              // O: offset
    case 17: recall(mQ); if (p & 15) set(0xE, 0x90 | (p & 15)); break;        // Q: retrigger
    case 18: set(0x7, p); break;                                              // R: tremolo
    case 19:                                                                  // S
        switch (p >> 4) {
        case 0xB: set(0xE, 0x60 | (p & 15)); break;
        case 0xC: set(0xE, 0xC0 | (p & 15)); break;
        case 0xD: set(0xE, 0xD0 | (p & 15)); break;
        case 0xE: set(0xE, 0xE0 | (p & 15)); break;
        default: break;
        }
        break;
    case 20: if (p >= 32) set(0xF, p); break;                                 // T: tempo (not a slide)
    default: break;
    }
}

// the used channels (from 0), at most 64
bool compact(const std::vector<int>& used, ModFile& m, std::string& error) {
    if (used.size() > 64) { error = std::to_string(used.size()) + " channels: more than 64"; return false; }
    if (used.empty()) { error = "no channels used"; return false; }
    m.channels = (int)used.size();
    return true;
}

// the patterns translated: raw cells (nch a row) of the used channels;
// resolve(cell, chan, out) sets the sample and period
template <class Resolve, class Effect>
void translate(ModFile& m, const std::vector<std::vector<Raw>>& raw, const std::vector<int>& rows, int nch,
               const std::vector<int>& used, Resolve resolve, Effect effect) {
    m.patterns.assign(raw.size(), {});
    m.rows = rows;
    std::vector<Chan> chans(used.size());
    for (size_t p : playOrder(m.order, raw.size())) {
        auto& P = m.patterns[p];
        P.assign((size_t)rows[p] * used.size(), ModCell());
        for (int row = 0; row < rows[p]; row++)
            for (size_t k = 0; k < used.size(); k++) {
                const Raw& r = raw[p][(size_t)row * nch + used[k]];
                ModCell& o = P[(size_t)row * used.size() + k];
                effect(r, chans[k], o);
                resolve(r, chans[k], o);
                if (r.note == kOff) cut(o);
            }
    }
}

// --- XM -------------------------------------------------------------------
//
// A 60-byte head ("Extended Module: ", the name, the tracker's, the
// version), then from 60 the header (its size first): the song's length,
// restart, channels, patterns, instruments, flags, speed, BPM, 256 orders;
// the patterns (a header: its size, packing, rows, the packed size; the
// cells packed: a byte with bit 7 set says which of note, instrument,
// volume, effect, param follow); the instruments (a header: its size, the
// name, the samples; with samples, the note→sample map and envelopes, then
// the samples' headers, then their data, delta-coded).

bool readXm(const Bytes& f, ModFile& m, std::string& error) {
    if (f.size() < 336) { error = "an XM, truncated (its header)"; return false; }
    int ver = le16(f, 58);
    if (ver < 0x0104) {
        char b[96];
        snprintf(b, sizeof b, "an XM of version %d.%02x (before 1.04): not read", ver >> 8, ver & 0xFF);
        error = b;
        return false;
    }
    m.tag = "XM";
    m.title = text(f, 17, 20);
    size_t hsize = le32(f, 60);
    int len = std::min(256, le16(f, 64)), nch = le16(f, 68), np = le16(f, 70), ni = le16(f, 72);
    m.speed = le16(f, 76) ? std::min(255, le16(f, 76)) : 6;
    m.bpm = le16(f, 78) >= 32 ? std::min(255, le16(f, 78)) : 125;
    if (nch < 1 || nch > 64) { error = "an XM of " + std::to_string(nch) + " channels (1-64 read)"; return false; }
    if (np > 256 || ni > 128) { error = "an XM of " + std::to_string(np) + " patterns, " + std::to_string(ni) + " instruments: not an XM?"; return false; }
    Orders ord = orders(f, 80, (size_t)len, (size_t)np, false);
    m.order = ord.list;
    m.songLen = (int)m.order.size();

    // the patterns
    std::vector<std::vector<Raw>> raw;
    std::vector<int> rows;
    size_t at = 60 + hsize;
    for (int p = 0; p < np; p++) {
        if (!has(f, at, 9)) { error = "an XM, truncated (pattern " + std::to_string(p) + ")"; return false; }
        size_t hl = le32(f, at);
        int nr = le16(f, at + 5);
        size_t ps = (size_t)le16(f, at + 7);
        if (nr == 0) nr = 64;
        if (nr > 1024) { error = "an XM's pattern " + std::to_string(p) + " of " + std::to_string(nr) + " rows"; return false; }
        if (!has(f, at, hl) || !has(f, at + hl, ps)) { error = "an XM, truncated (pattern " + std::to_string(p) + ")"; return false; }
        std::vector<Raw> cells((size_t)nr * nch);
        size_t q = at + hl, end = at + hl + ps;
        auto rd = [&]() { return q < end ? f[q++] : 0; };
        for (size_t c = 0; c < cells.size() && q < end; c++) {
            int b = rd(), note = 0, ins = 0, vol = 0, fx = 0, prm = 0;
            if (b & 0x80) {
                if (b & 1) note = rd();
                if (b & 2) ins = rd();
                if (b & 4) vol = rd();
                if (b & 8) fx = rd();
                if (b & 16) prm = rd();
            } else {
                note = b; ins = rd(); vol = rd(); fx = rd(); prm = rd();
            }
            Raw& r = cells[c];
            r.note = note >= 1 && note <= 96 ? note : note == 97 ? kOff : -1;
            r.ins = ins;
            r.vol = vol;
            r.cmd = fx;
            r.param = prm;
        }
        raw.push_back(cells);
        rows.push_back(nr);
        at = end;
    }

    // the instruments: their samples one list; each note→sample map to it
    std::vector<std::vector<uint8_t>> maps;   // (an instrument's, a note's flat sample: 0 none)
    for (int i = 0; i < ni; i++) {
        std::vector<uint8_t> map(96, 0);
        if (!has(f, at, 29)) { maps.push_back(map); at = f.size(); continue; }   // (a file cut short: silent)
        size_t isz = le32(f, at);
        std::string iname = text(f, at + 4, 22);
        int ns = le16(f, at + 27);
        if (ns == 0) {
            maps.push_back(map);
            at += isz ? isz : 29;
            continue;
        }
        if (ns > 255 || m.samples.size() + (size_t)ns > 255) { error = "an XM of more than 255 samples"; return false; }
        size_t shs = le32(f, at + 29);
        if (shs == 0) shs = 40;
        size_t base = m.samples.size() + 1;
        for (int n = 0; n < 96; n++) {
            int k = u8(f, at + 33 + (size_t)n);
            map[(size_t)n] = k < ns ? (uint8_t)(base + (size_t)k) : 0;
        }
        maps.push_back(map);
        at += isz ? isz : 263;
        struct Head { uint32_t len, ls, ll; int type, pack; };
        std::vector<Head> heads;
        for (int k = 0; k < ns; k++, at += shs) {
            ModSample s;
            Head h{le32(f, at), le32(f, at + 4), le32(f, at + 8), u8(f, at + 14), u8(f, at + 17)};
            s.volume = std::min(64, u8(f, at + 12));
            int ft = (int8_t)u8(f, at + 13), rel = (int8_t)u8(f, at + 16);
            s.transpose = rel + ft / 128.0 + 12 * std::log2(8363 / kMidRate);
            s.name = text(f, at + 18, 22);
            if (s.name.empty()) s.name = iname;
            heads.push_back(h);
            m.samples.push_back(s);
        }
        for (int k = 0; k < ns; k++) {
            const Head& h = heads[(size_t)k];
            ModSample& s = m.samples[base - 1 + (size_t)k];
            bool is16 = h.type & 0x10, stereo = h.type & 0x20;
            if (h.pack == 0xAD) { at += 16 + (h.len + 1) / 2; continue; }   // (ModPlug's ADPCM: not read)
            size_t w = (is16 ? 2 : 1) * (stereo ? 2 : 1), n = h.len / w;
            Pcm l = readPcm(f, at, n, is16, true, true);
            if (stereo) {
                Pcm r = readPcm(f, at + n * (is16 ? 2 : 1), n, is16, true, true);
                s.data = to8(l, &r);
            } else s.data = to8(l, nullptr);
            at += h.len;
            if (h.type & 3) setLoop(s, h.ls / w, (h.ls + h.ll) / w, h.type & 2);
        }
    }

    // the channels: all
    std::vector<int> used;
    for (int c = 0; c < nch; c++) used.push_back(c);
    if (!compact(used, m, error)) return false;
    translate(m, raw, rows, nch, used,
        [&](const Raw& r, Chan& c, ModCell& o) {
            auto sampleOf = [&](int ins, int note) {
                return ins >= 1 && ins <= (int)maps.size() && note >= 1 && note <= 96 ? maps[(size_t)ins - 1][(size_t)note - 1] : 0;
            };
            if (r.ins) c.ins = r.ins;
            if (r.note > 0) {
                c.note = r.note;
                int s = sampleOf(c.ins, r.note);
                if (s) { o.sample = (uint8_t)s; o.period = periodOf(r.note - 49); }   // (none: FT2 plays nothing)
            } else if (r.ins) o.sample = (uint8_t)sampleOf(r.ins, c.note);
        },
        [&](const Raw& r, Chan&, ModCell& o) {
            if (r.cmd <= 15) { o.effect = (uint8_t)r.cmd; o.param = (uint8_t)r.param; }
            else if (r.cmd == 20) { o.effect = 0xE; o.param = (uint8_t)(0xC0 | std::min(r.param, 15)); }   // K: key off
            // the volume column, where the effect's slot is free
            int v = r.vol;
            if (!empty(o)) return;
            if (v >= 0x10 && v <= 0x50) { o.effect = 0xC; o.param = (uint8_t)(v - 0x10); }
            else if (v >= 0x60 && v <= 0x6F) { o.effect = 0xA; o.param = (uint8_t)(v & 15); }
            else if (v >= 0x70 && v <= 0x7F) { o.effect = 0xA; o.param = (uint8_t)((v & 15) << 4); }
            else if (v >= 0x80 && v <= 0x8F) { o.effect = 0xE; o.param = (uint8_t)(0xB0 | (v & 15)); }
            else if (v >= 0x90 && v <= 0x9F) { o.effect = 0xE; o.param = (uint8_t)(0xA0 | (v & 15)); }
            else if (v >= 0xF0) { o.effect = 0x3; o.param = (uint8_t)((v & 15) << 4); }   // (a slide to the note)
        });
    for (auto& P : m.patterns)   // (B: the orders dropped)
        for (auto& c : P) if (c.effect == 0xB) c.param = ord.to(c.param);
    return true;
}

// --- S3M ------------------------------------------------------------------
//
// The name (28), 0x1A, the type (16); orders, instruments, patterns, flags,
// the tracker, the samples' format (1 signed, 2 unsigned); "SCRM"; the
// globals, speed, tempo; 32 channels' settings (bit 7: off); then the
// orders, the instruments' and patterns' paragraphs (16 bytes). A sample: 1,
// a file name, its data's paragraph (3 bytes), length, loop, volume, packing,
// flags (loop, stereo, 16-bit), C2 speed, name. A pattern: its packed length,
// then a row's cells (a byte: the channel, and which of note+instrument,
// volume, effect follow), 0 ending the row.

bool readS3m(const Bytes& f, ModFile& m, std::string& error) {
    if (f.size() < 0x60) { error = "an S3M, truncated (its header)"; return false; }
    m.tag = "S3M";
    m.title = text(f, 0, 28);
    int no = le16(f, 0x20), ni = le16(f, 0x22), np = le16(f, 0x24);
    bool isSigned = le16(f, 0x2A) == 1;
    m.speed = u8(f, 0x31) && u8(f, 0x31) < 255 ? u8(f, 0x31) : 6;
    m.bpm = u8(f, 0x32) >= 33 ? u8(f, 0x32) : 125;
    if (ni > 255) { error = "an S3M of more than 255 samples"; return false; }
    np = std::min(np, 256);
    size_t ip = 0x60 + (size_t)no, pp = ip + 2 * (size_t)ni;
    if (!has(f, 0x60, (size_t)no + 2 * ((size_t)ni + (size_t)np))) { error = "an S3M, truncated (its lists)"; return false; }
    Orders ord = orders(f, 0x60, (size_t)no, (size_t)np);
    m.order = ord.list;
    m.songLen = (int)m.order.size();

    for (int i = 0; i < ni; i++) {
        size_t at = (size_t)le16(f, ip + 2 * (size_t)i) * 16;
        ModSample s;
        if (!has(f, at, 0x50)) { m.samples.push_back(s); continue; }
        s.name = text(f, at + 0x30, 28);
        if (s.name.empty()) s.name = text(f, at + 1, 12);
        s.volume = std::min(64, u8(f, at + 0x1C));
        uint32_t c2 = le32(f, at + 0x20);
        s.transpose = 12 * std::log2((c2 ? c2 : 8363) / kMidRate);
        if (u8(f, at) == 1 && u8(f, at + 0x1E) == 0) {   // (a sample, not packed; Adlib's and ADPCM's empty)
            size_t data = ((size_t)u8(f, at + 0x0D) << 16 | (size_t)le16(f, at + 0x0E)) * 16;
            size_t n = le32(f, at + 0x10);
            int flags = u8(f, at + 0x1F);
            bool is16 = flags & 4, stereo = flags & 2;
            Pcm l = readPcm(f, data, n, is16, isSigned, false);
            if (stereo) {
                Pcm r = readPcm(f, data + n * (is16 ? 2 : 1), n, is16, isSigned, false);
                s.data = to8(l, &r);
            } else s.data = to8(l, nullptr);
            if (flags & 1) setLoop(s, le32(f, at + 0x14), le32(f, at + 0x18), false);
        }
        m.samples.push_back(s);
    }

    std::vector<std::vector<Raw>> raw;
    std::vector<int> rows;
    for (int p = 0; p < np; p++) {
        std::vector<Raw> cells(64 * 32);
        size_t at = (size_t)le16(f, pp + 2 * (size_t)p) * 16;
        if (at && !has(f, at, 2)) { error = "an S3M, truncated (pattern " + std::to_string(p) + ")"; return false; }
        if (at) {   // (its data cut short: what there is)
            size_t q = at + 2, end = std::min(f.size(), at + (size_t)le16(f, at));
            auto rd = [&]() { return q < end ? f[q++] : 0; };
            for (int row = 0; row < 64 && q < end;) {
                int b = rd();
                if (!b) { row++; continue; }
                Raw& r = cells[(size_t)row * 32 + (b & 31)];
                if (b & 32) {
                    int n = rd();
                    r.ins = rd();
                    if (n == 254 || n == 255) r.note = n == 254 ? kOff : -1;
                    else if ((n & 15) < 12) r.note = (n >> 4) * 12 + (n & 15);
                }
                if (b & 64) r.vol = rd();
                if (b & 128) { r.cmd = rd(); r.param = rd(); }
            }
        }
        raw.push_back(cells);
        rows.push_back(64);
    }

    std::vector<int> used;   // (the enabled channels: PCM and Adlib)
    for (int c = 0; c < 32; c++) if (u8(f, 0x40 + (size_t)c) < 32) used.push_back(c);
    if (!compact(used, m, error)) return false;
    int ns = (int)m.samples.size();
    translate(m, raw, rows, 32, used,
        [&](const Raw& r, Chan& c, ModCell& o) {
            if (r.ins >= 1 && r.ins <= ns) c.ins = r.ins;
            if (r.ins >= 1 && r.ins <= ns) o.sample = (uint8_t)r.ins;
            if (r.note >= 0) {
                if (!o.sample && c.ins) o.sample = (uint8_t)c.ins;
                o.period = periodOf(r.note - 48);   // (ST3's C-4: the middle, at C2 speed)
            }
        },
        [&](const Raw& r, Chan& c, ModCell& o) {
            letterEffect(r, c, false, false, ord, o);
            if (r.vol >= 0 && r.vol <= 64 && empty(o)) { o.effect = 0xC; o.param = (uint8_t)r.vol; }
        });
    return true;
}

// --- IT -------------------------------------------------------------------
//
// "IMPM", the name (26), orders, instruments, samples, patterns, the
// tracker, the format it needs (cmwt), flags (bit 2: instruments), speed
// and tempo at 0x32; 64 channels' pan and volume; the orders, then the
// offsets (32-bit) of the instruments, samples, patterns. An instrument
// ("IMPI"): a note→(note, sample) table at 0x40 (in the old format too). A
// sample ("IMPS"): flags (exists, 16-bit, stereo, compressed, loop,
// sustain loop, ping-pong each), volume, name, convert flags (signed,
// big-endian, delta: with compression, IT 2.15's), length, loops, C5
// speed, the data's offset. A pattern: its packed length and rows, then a
// row's cells (a byte: the channel, a new mask; the mask: which of note,
// instrument, volume, effect follow, or the channel's last of each).

bool readIt(const Bytes& f, ModFile& m, std::string& error) {
    if (f.size() < 0xC0) { error = "an IT, truncated (its header)"; return false; }
    m.tag = "IT";
    m.title = text(f, 4, 26);
    int no = le16(f, 0x20), ni = le16(f, 0x22), ns = le16(f, 0x24), np = le16(f, 0x26), flags = le16(f, 0x2C);
    bool instMode = flags & 4, linkG = flags & 0x20;
    m.speed = u8(f, 0x32) ? u8(f, 0x32) : 6;
    m.bpm = u8(f, 0x33) >= 32 ? u8(f, 0x33) : 125;
    if (ns > 255) { error = "an IT of " + std::to_string(ns) + " samples (255 read)"; return false; }
    size_t io = 0xC0 + (size_t)no, so = io + 4 * (size_t)ni, po = so + 4 * (size_t)ns;
    if (!has(f, 0xC0, (size_t)no + 4 * ((size_t)ni + ns + np))) { error = "an IT, truncated (its lists)"; return false; }
    np = std::min(np, 256);
    Orders ord = orders(f, 0xC0, (size_t)no, (size_t)np);
    m.order = ord.list;
    m.songLen = (int)m.order.size();

    // the samples
    for (int k = 0; k < ns; k++) {
        size_t at = le32(f, so + 4 * (size_t)k);
        ModSample s;
        if (!has(f, at, 0x50) || std::memcmp(&f[at], "IMPS", 4) != 0) { m.samples.push_back(s); continue; }
        int fl = u8(f, at + 0x12), cvt = u8(f, at + 0x2E);
        s.name = text(f, at + 0x14, 26);
        if (s.name.empty()) s.name = text(f, at + 4, 12);
        s.volume = std::min(64, u8(f, at + 0x13));
        uint32_t c5 = le32(f, at + 0x3C);
        s.transpose = 12 * std::log2((c5 ? c5 : 8363) / kMidRate);
        size_t n = le32(f, at + 0x30), data = le32(f, at + 0x48);
        if (fl & 1 && n) {
            bool is16 = fl & 2, stereo = fl & 4;
            Pcm l, r;
            if (fl & 8) {   // compressed (each channel's blocks in turn)
                size_t q = data;
                l = itUnpack(f, q, n, is16, cvt & 4);
                if (stereo) r = itUnpack(f, q, n, is16, cvt & 4);
            } else {
                l = readPcm(f, data, n, is16, cvt & 1, cvt & 4, cvt & 2);
                if (stereo) r = readPcm(f, data + n * (is16 ? 2 : 1), n, is16, cvt & 1, cvt & 4, cvt & 2);
            }
            s.data = to8(l, stereo ? &r : nullptr);
            // the loop (else the sustain loop: a note held until the next)
            if (fl & 0x10) setLoop(s, le32(f, at + 0x34), le32(f, at + 0x38), fl & 0x40);
            else if (fl & 0x20) setLoop(s, le32(f, at + 0x40), le32(f, at + 0x44), fl & 0x80);
        }
        m.samples.push_back(s);
    }

    // the instruments: a note's (note, sample)
    struct Keys { uint8_t note[120], sample[120]; std::string name; };
    std::vector<Keys> keys;
    if (instMode)
        for (int i = 0; i < ni; i++) {
            size_t at = le32(f, io + 4 * (size_t)i);
            Keys K{};
            if (has(f, at, 0x40 + 240) && std::memcmp(&f[at], "IMPI", 4) == 0) {
                K.name = text(f, at + 0x20, 26);
                for (int n = 0; n < 120; n++) {
                    K.note[n] = (uint8_t)u8(f, at + 0x40 + 2 * (size_t)n);
                    K.sample[n] = (uint8_t)u8(f, at + 0x41 + 2 * (size_t)n);
                }
            }
            keys.push_back(K);
        }
    for (const auto& K : keys)   // (a nameless sample: its instrument's name)
        for (int n = 0; n < 120; n++)
            if (K.sample[n] && K.sample[n] <= ns && m.samples[K.sample[n] - 1u].name.empty())
                m.samples[K.sample[n] - 1u].name = K.name;

    // the patterns (64 channels)
    std::vector<std::vector<Raw>> raw;
    std::vector<int> rows;
    std::vector<bool> any(64, false);
    for (int p = 0; p < np; p++) {
        size_t at = le32(f, po + 4 * (size_t)p);
        if (at && !has(f, at, 8)) { error = "an IT, truncated (pattern " + std::to_string(p) + ")"; return false; }
        if (!at) {
            raw.push_back(std::vector<Raw>(64 * 64));
            rows.push_back(64);
            continue;
        }
        int nr = le16(f, at + 2);
        if (nr < 1 || nr > 1024) { error = "an IT's pattern " + std::to_string(p) + " of " + std::to_string(nr) + " rows"; return false; }
        std::vector<Raw> cells((size_t)nr * 64);
        size_t q = at + 8, end = std::min(f.size(), q + (size_t)le16(f, at));   // (cut short: what there is)
        auto rd = [&]() { return q < end ? f[q++] : 0; };
        int mask[64] = {}, lnote[64] = {}, lins[64] = {}, lvol[64] = {}, lcmd[64] = {}, lprm[64] = {};
        auto noteOf = [](int n) { return n <= 119 ? n : n >= 120 ? kOff : -1; };
        for (int row = 0; row < nr && q < end;) {
            int b = rd();
            if (!b) { row++; continue; }
            int ch = (b - 1) & 63;
            if (b & 128) mask[ch] = rd();
            int mk = mask[ch];
            Raw& r = cells[(size_t)row * 64 + ch];
            if (mk & 1) r.note = noteOf(lnote[ch] = rd());
            if (mk & 2) r.ins = lins[ch] = rd();
            if (mk & 4) r.vol = lvol[ch] = rd();
            if (mk & 8) { r.cmd = lcmd[ch] = rd(); r.param = lprm[ch] = rd(); }
            if (mk & 16) r.note = noteOf(lnote[ch]);
            if (mk & 32) r.ins = lins[ch];
            if (mk & 64) r.vol = lvol[ch];
            if (mk & 128) { r.cmd = lcmd[ch]; r.param = lprm[ch]; }
            if (mk) any[(size_t)ch] = true;
        }
        raw.push_back(cells);
        rows.push_back(nr);
    }

    std::vector<int> used;
    for (int c = 0; c < 64; c++) if (any[(size_t)c]) used.push_back(c);
    if (!compact(used, m, error)) return false;
    translate(m, raw, rows, 64, used,
        [&](const Raw& r, Chan& c, ModCell& o) {
            // (note, sample) for a note: the instrument's table, or the sample's own
            auto play = [&](int ins, int note, int& outNote) -> int {
                outNote = note;
                if (!instMode) return ins >= 1 && ins <= ns ? ins : 0;
                if (ins < 1 || ins > (int)keys.size() || note < 0 || note > 119) return 0;
                const Keys& K = keys[(size_t)ins - 1];
                outNote = K.note[note] <= 119 ? K.note[note] : note;
                return K.sample[note] <= ns ? K.sample[note] : 0;
            };
            if (r.ins) c.ins = r.ins;
            int pn;
            if (r.note >= 0) {
                c.note = r.note;
                int s = play(c.ins, r.note, pn);
                if (s) { o.sample = (uint8_t)s; o.period = periodOf(pn - 60); }
            } else if (r.ins && c.note >= 0) o.sample = (uint8_t)play(r.ins, c.note, pn);
            else if (r.ins && !instMode && r.ins <= ns) o.sample = (uint8_t)r.ins;
        },
        [&](const Raw& r, Chan& c, ModCell& o) {
            letterEffect(r, c, true, linkG, ord, o);
            if (!empty(o) || r.vol < 0) return;
            static const uint8_t gSpeed[10] = {0, 1, 4, 8, 16, 32, 64, 96, 128, 255};
            if (r.vol <= 64) { o.effect = 0xC; o.param = (uint8_t)r.vol; }
            else if (r.vol >= 193 && r.vol <= 202) {   // (a slide to the note, G's memory)
                int& mG = linkG ? c.ef : c.g;
                int g = gSpeed[r.vol - 193];
                if (g) mG = g; else g = mG;
                o.effect = 0x3;
                o.param = (uint8_t)g;
            }
        });
    return true;
}

}  // namespace

bool isTrackerModule(const std::vector<uint8_t>& f) {
    if (f.size() >= 17 && std::memcmp(f.data(), "Extended Module: ", 17) == 0) return true;
    if (f.size() >= 48 && std::memcmp(f.data() + 44, "SCRM", 4) == 0) return true;
    return f.size() >= 4 && std::memcmp(f.data(), "IMPM", 4) == 0;
}

bool readTracker(const std::vector<uint8_t>& f, ModFile& m, std::string& error) {
    m = ModFile();
    bool ok;
    if (f.size() >= 17 && std::memcmp(f.data(), "Extended Module: ", 17) == 0) ok = readXm(f, m, error);
    else if (f.size() >= 48 && std::memcmp(f.data() + 44, "SCRM", 4) == 0) ok = readS3m(f, m, error);
    else if (f.size() >= 4 && std::memcmp(f.data(), "IMPM", 4) == 0) ok = readIt(f, m, error);
    else { error = "not an XM, S3M or IT"; return false; }
    if (ok && m.songLen == 0) { error = "an " + m.tag + " of no positions"; return false; }
    return ok;
}
