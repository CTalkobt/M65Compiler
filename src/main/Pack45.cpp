// Pack45.cpp — the packer of format 1 (Pack45.hpp, #345): greedy LZ with
// hash chains (a lazy look one byte ahead), runs as fills, literals in
// blocks of up to 64. Every repeat is at least as far back as it is long.
#include "Pack45.hpp"
#include <algorithm>
#include <cstring>

namespace pack45 {

namespace {

constexpr int kMaxLit = 64;     // $00-$3F: n + 1
constexpr int kMaxRun = 66;     // n + 3, n <= $3F
constexpr int kMinRun = 3;
constexpr int kChainDepth = 256;   // candidates looked at per position

struct Match {
    int len = 0;
    int off = 0;
    bool fill = false;
    int cost() const { return fill ? 2 : off <= 256 ? 2 : 3; }
    int gain() const { return len - cost(); }
};

class Packer {
public:
    Packer(const std::vector<uint8_t>& d) : d_(d), head_(65536, -1), prev_(d.size(), -1) {}

    // the best repeat or fill at i
    Match best(int i) {
        Match m;
        int n = (int)d_.size();
        if (i + kMinRun > n) return m;
        // a fill
        int run = 1;
        while (i + run < n && run < kMaxRun && d_[i + run] == d_[i]) run++;
        if (run >= kMinRun) { m.len = run; m.fill = true; }
        // repeats (no overlap: off >= len)
        int h = hash(i);
        int depth = 0;
        for (int j = head_[h]; j >= 0 && depth < kChainDepth; j = prev_[j], depth++) {
            int off = i - j;
            if (off > 65535) break;
            int maxLen = std::min({kMaxRun, off, n - i});
            if (maxLen < kMinRun) continue;
            int len = 0;
            while (len < maxLen && d_[j + len] == d_[i + len]) len++;
            if (len < kMinRun) continue;
            Match c;
            c.len = len;
            c.off = off;
            if (c.gain() > m.gain() || (c.gain() == m.gain() && c.len > m.len)) m = c;
        }
        return m;
    }

    // the longest repeat at i from a near offset (<= 256 back, 2 bytes)
    // and from a far one (3 bytes): every shorter length is there too, at
    // the same offset
    void longest(int i, int& near, int& nearOff, int& far, int& farOff) {
        near = far = nearOff = farOff = 0;
        int n = (int)d_.size();
        if (i + kMinRun > n) return;
        int depth = 0;
        for (int j = head_[hash(i)]; j >= 0 && depth < kChainDepth; j = prev_[j], depth++) {
            int off = i - j;
            if (off > 65535) break;
            int maxLen = std::min({kMaxRun, off, n - i});
            if (maxLen <= (off <= 256 ? near : far)) continue;
            int len = 0;
            while (len < maxLen && d_[j + len] == d_[i + len]) len++;
            if (len < kMinRun) continue;
            if (off <= 256) {
                if (len > near) { near = len; nearOff = off; }
            } else if (len > far) { far = len; farOff = off; }
        }
    }

    void insert(int i) {
        if (i + kMinRun > (int)d_.size()) return;
        int h = hash(i);
        prev_[i] = head_[h];
        head_[h] = i;
    }

private:
    int hash(int i) const { return ((d_[i] << 8) ^ (d_[i + 1] << 4) ^ d_[i + 2]) & 0xFFFF; }
    const std::vector<uint8_t>& d_;
    std::vector<int> head_, prev_;
};

}  // namespace

namespace {

// the stream as it is written, with the in-place offset: the most the
// writing gets ahead of the reading, at the end of each token (a literal
// block's bytes are read as they are copied, forwards)
struct Writer {
    std::vector<uint8_t>& s;
    long w = 0, ahead = 0;
    void mark() { ahead = std::max(ahead, w - (long)s.size()); }
    void literals(const uint8_t* p, size_t n) {
        for (size_t k = 0; k < n;) {
            size_t c = std::min(n - k, (size_t)kMaxLit);
            s.push_back((uint8_t)(c - 1));
            s.insert(s.end(), p + k, p + k + c);
            w += (long)c;
            mark();
            k += c;
        }
    }
    void fill(int len, uint8_t v) {
        s.push_back((uint8_t)(0x40 | (len - kMinRun)));
        s.push_back(v);
        w += len;
        mark();
    }
    void repeat(int len, int off) {
        if (off <= 256) {
            s.push_back((uint8_t)(0x80 | (len - kMinRun)));
            s.push_back((uint8_t)(off - 1));
        } else {
            s.push_back((uint8_t)(0xC0 | (len - kMinRun)));
            s.push_back((uint8_t)off);
            s.push_back((uint8_t)(off >> 8));
        }
        w += len;
        mark();
    }
    void end() {
        s.push_back(0xC0);
        s.push_back(0);
        s.push_back(0);
        mark();
    }
};

void packGreedy(const std::vector<uint8_t>& data, Writer& out) {
    Packer pk(data);
    std::vector<uint8_t> lits;
    auto flush = [&]() {
        out.literals(lits.data(), lits.size());
        lits.clear();
    };
    int n = (int)data.size();
    int i = 0;
    while (i < n) {
        Match m = pk.best(i);
        pk.insert(i);
        // lazy: a better one a byte later wins over this one
        if (m.gain() > 0 && i + 1 < n && pk.best(i + 1).gain() > m.gain()) m = Match();
        if (m.gain() <= 0) {
            lits.push_back(data[i]);
            i++;
            continue;
        }
        flush();
        if (m.fill) out.fill(m.len, data[i]);
        else out.repeat(m.len, m.off);
        for (int k = 1; k < m.len; k++) pk.insert(i + k);
        i += m.len;
    }
    flush();
}

// the best parse: from the end, each place's cheapest way on (a fill, a
// repeat of any length up to the longest found, or a block of 1..64
// literals: its header byte and its bytes). a[i]: the least bytes for
// data[i..]. Exact for the format's costs, so never larger than the greedy
// parse, which is one of the parses it weighs (a literal block's 64-byte
// limit was counted once a run before: on literal-heavy data --best came
// out a byte or so larger)
void packBest(const std::vector<uint8_t>& data, Writer& out) {
    int n = (int)data.size();
    std::vector<int> nearLen(n), nearOff(n), farLen(n), farOff(n), run(n);
    Packer pk(data);
    for (int i = 0; i < n; i++) {
        pk.longest(i, nearLen[i], nearOff[i], farLen[i], farOff[i]);
        pk.insert(i);
        int r = 1;
        while (i + r < n && r < kMaxRun && data[i + r] == data[i]) r++;
        run[i] = r >= kMinRun ? r : 0;
    }
    constexpr int kInf = 1 << 30;
    // len[i]: the token's length (0: a literal block of lit[i] bytes);
    // off[i]: its offset (0: a fill)
    std::vector<int> a(n + 1, kInf), len(n + 1, 0), off(n + 1, 0), lit(n + 1, 0);
    a[n] = 0;
    for (int i = n - 1; i >= 0; i--) {
        int best = kInf;
        for (int l = 1; l <= kMaxLit && i + l <= n; l++)
            if (1 + l + a[i + l] < best) { best = 1 + l + a[i + l]; lit[i] = l; }
        for (int l = kMinRun; l <= run[i]; l++)
            if (2 + a[i + l] < best) { best = 2 + a[i + l]; len[i] = l; off[i] = 0; }
        for (int l = kMinRun; l <= nearLen[i]; l++)
            if (2 + a[i + l] < best) { best = 2 + a[i + l]; len[i] = l; off[i] = nearOff[i]; }
        for (int l = std::max(kMinRun, nearLen[i] + 1); l <= farLen[i]; l++)
            if (3 + a[i + l] < best) { best = 3 + a[i + l]; len[i] = l; off[i] = farOff[i]; }
        a[i] = best;
    }
    for (int i = 0; i < n;) {
        if (!len[i]) {
            out.literals(data.data() + i, lit[i]);
            i += lit[i];
            continue;
        }
        if (off[i]) out.repeat(len[i], off[i]);
        else out.fill(len[i], data[i]);
        i += len[i];
    }
}

}  // namespace

Packed pack(const std::vector<uint8_t>& data, uint32_t dest, bool best) {
    Packed p;
    p.stream.push_back(kFormat);
    for (int k = 0; k < 4; k++) p.stream.push_back((uint8_t)(dest >> (8 * k)));
    Writer out{p.stream};
    if (best) packBest(data, out);
    else packGreedy(data, out);
    out.end();
    p.inPlace = (uint32_t)out.ahead;
    return p;
}

static bool unpack1(const std::vector<uint8_t>& s, std::vector<uint8_t>& out, uint32_t* dest, std::string* err) {
    auto fail = [&](const std::string& m) { if (err) *err = m; return false; };
    out.clear();
    if (s.size() < kHeader + 3) return fail("too short");
    if (dest) *dest = s[1] | (s[2] << 8) | (s[3] << 16) | ((uint32_t)s[4] << 24);
    size_t k = kHeader;
    for (;;) {
        if (k >= s.size()) return fail("no end mark");
        uint8_t t = s[k++];
        int n = t & 0x3F;
        if (t < 0x40) {
            if (k + n + 1 > s.size()) return fail("a literal block past the end");
            out.insert(out.end(), s.begin() + k, s.begin() + k + n + 1);
            k += n + 1;
        } else if (t < 0x80) {
            if (k >= s.size()) return fail("a fill past the end");
            out.insert(out.end(), n + 3, s[k++]);
        } else {
            int o;
            if (t < 0xC0) {
                if (k >= s.size()) return fail("a repeat past the end");
                o = s[k++] + 1;
            } else {
                if (k + 2 > s.size()) return fail("a repeat past the end");
                o = s[k] | (s[k + 1] << 8);
                k += 2;
                if (!n && !o) return true;
            }
            int len = n + 3;
            if (o < len || o > (int)out.size()) return fail("a repeat from before the start or overlapping");
            size_t from = out.size() - o;
            for (int c = 0; c < len; c++) out.push_back(out[from + c]);
        }
    }
}

static bool checkInPlace1(const std::vector<uint8_t>& data, const Packed& p, uint32_t offset) {
    // memory: the unpacked area and what the stream needs past it
    size_t size = std::max(data.size(), (size_t)offset + p.stream.size()) + 1;
    std::vector<uint8_t> mem(size, 0xEE);
    std::memcpy(mem.data() + offset, p.stream.data(), p.stream.size());
    size_t r = offset + kHeader, w = 0;
    for (;;) {
        uint8_t t = mem[r++];
        int n = t & 0x3F;
        if (t < 0x40) {
            for (int c = 0; c <= n; c++) mem[w++] = mem[r++];   // (forwards, as the DMA copy)
        } else if (t < 0x80) {
            uint8_t v = mem[r++];
            for (int c = 0; c < n + 3; c++) mem[w++] = v;
        } else {
            int o;
            if (t < 0xC0) o = mem[r++] + 1;
            else {
                o = mem[r] | (mem[r + 1] << 8);
                r += 2;
                if (!n && !o) break;
            }
            for (int c = 0; c < n + 3; c++, w++) mem[w] = mem[w - o];
        }
        if (w > r) return false;   // the writing overtook the reading
    }
    return w == data.size() && std::equal(data.begin(), data.end(), mem.begin());
}

// ---------------------------------------------------------------------------
// Format 2 (#666): an image's ranks minus a prediction, Huffman-coded
// ---------------------------------------------------------------------------

namespace {

constexpr int kMaxCode = 8;     // the decoder looks a code up on the next 8 bits

// Huffman code lengths (0: a symbol that does not occur), the counts halved
// (each kept at least 1) until none is longer than kMaxCode; ties broken as
// texture_demo's tools/tcode.py does (by count, then an id: the symbol, then
// the order the pairs were made in)
std::vector<int> huffmanLengths(std::vector<uint64_t> counts) {
    for (;;) {
        std::vector<int> len(counts.size(), 0);
        struct Node { uint64_t c; size_t tie; std::vector<int> syms; };
        auto cmp = [](const Node& a, const Node& b) { return a.c != b.c ? a.c > b.c : a.tie > b.tie; };
        std::vector<Node> heap;
        for (size_t s = 0; s < counts.size(); s++)
            if (counts[s]) heap.push_back({counts[s], s, {(int)s}});
        if (heap.size() == 1) {
            len[heap[0].syms[0]] = 1;
            return len;
        }
        std::make_heap(heap.begin(), heap.end(), cmp);
        size_t tie = counts.size();
        while (heap.size() > 1) {
            std::pop_heap(heap.begin(), heap.end(), cmp);
            Node a = heap.back();
            heap.pop_back();
            std::pop_heap(heap.begin(), heap.end(), cmp);
            Node b = heap.back();
            heap.pop_back();
            for (int s : a.syms) len[s]++;
            for (int s : b.syms) len[s]++;
            Node m{a.c + b.c, tie++, a.syms};
            m.syms.insert(m.syms.end(), b.syms.begin(), b.syms.end());
            heap.push_back(m);
            std::push_heap(heap.begin(), heap.end(), cmp);
        }
        if (*std::max_element(len.begin(), len.end()) <= kMaxCode) return len;
        for (auto& c : counts)
            if (c) c = std::max<uint64_t>(1, c / 2);
    }
}

// The decoder, as unpack2.s45 has it: the 256-entry tables (every entry
// length 8, symbol 0, then each symbol of length n given the next 2^(8 - n)
// entries, by length then symbol), a byte taken while 8 bits or fewer are
// held. rd(i): the stream's byte i (avail of them); row(y, bytes, taken): row
// y decoded (its w / 2 bytes), the stream's bytes read so far (false: stop).
// false: not valid
template <class Rd, class Row>
bool decodeImage(Rd rd, size_t avail, Row row, uint32_t* dest, std::string* err) {
    auto fail = [&](const std::string& m) { if (err) *err = m; return false; };
    if (avail < (size_t)kImageHeader) return fail("too short");
    if (dest) *dest = rd(1) | (rd(2) << 8) | (rd(3) << 16) | ((uint32_t)rd(4) << 24);
    int pitch = rd(5), h = rd(6) | (rd(7) << 8), n = rd(8), left = rd(9);
    if (pitch < 1 || pitch > 128) return fail("width " + std::to_string(2 * pitch) + " (2-256)");
    if (n < 1 || n > 16) return fail(std::to_string(n) + " colours (1-16)");
    if (left > 1) return fail("prediction " + std::to_string(left) + " (0 above, 1 left)");
    if (avail < (size_t)kImageHeader + 2 * n) return fail("too short");
    uint8_t entry[16];
    for (int r = 0; r < n; r++) entry[r] = rd(kImageHeader + r) & 15;
    uint8_t sym[256], len[256];
    std::fill(sym, sym + 256, 0);
    std::fill(len, len + 256, kMaxCode);
    int at = 0;
    for (int l = 1; l <= kMaxCode; l++)
        for (int s = 0; s < n; s++)
            if (rd(kImageHeader + n + s) == l) {
                int k = 1 << (kMaxCode - l);
                if (at + k > 256) return fail("code lengths that are not a code");
                for (int i = 0; i < k; i++) { sym[at + i] = (uint8_t)s; len[at + i] = (uint8_t)l; }
                at += k;
            }
    size_t pos = kImageHeader + 2 * n;
    int w = 2 * pitch;
    std::vector<uint8_t> rank(256, 0), bytes(pitch);
    unsigned hbuf = 0, hbits = 0;
    for (int y = 0; y < h; y++) {
        int cur = rank[0];
        for (int x = 0; x < w; x++) {
            if (hbits <= 8) {
                if (pos >= avail) return fail("the codes past the end");
                hbuf |= (unsigned)rd(pos++) << (8 - hbits);
                hbits += 8;
            }
            unsigned e = (hbuf >> 8) & 255;
            hbuf = (hbuf << len[e]) & 0xFFFF;
            hbits -= len[e];
            if (!left) cur = rank[x];
            cur += sym[e];
            if (cur >= n) cur -= n;
            rank[x] = (uint8_t)cur;
        }
        for (int i = 0; i < pitch; i++) bytes[i] = (uint8_t)(entry[rank[2 * i]] | entry[rank[2 * i + 1]] << 4);
        if (!row(y, bytes, pos)) return fail("in place: a row written over what is still to be read");
    }
    return true;
}

}  // namespace

std::vector<uint8_t> imageBytes(const Image& img) {
    std::vector<uint8_t> out;
    for (int y = 0; y < img.h; y++)
        for (int x = 0; x < img.w; x += 2) {
            uint8_t a = img.px[(size_t)y * img.w + x] & 15;
            uint8_t b = x + 1 < img.w ? img.px[(size_t)y * img.w + x + 1] & 15 : 0;
            out.push_back((uint8_t)(a | b << 4));
        }
    return out;
}

bool packImage(const Image& img, uint32_t dest, int predict, Packed& out, int* chosen, std::string* err) {
    auto fail = [&](const std::string& m) { if (err) *err = m; return false; };
    if (img.w < 2 || img.w > 256 || (img.w & 1))
        return fail("the width is " + std::to_string(img.w) + ": format 2 takes an even width of 2-256");
    if (img.h < 1 || img.h > 65535) return fail("the height is " + std::to_string(img.h) + " (1-65535)");
    if (img.px.size() != (size_t)img.w * img.h) return fail("the pixels are not w x h");
    bool used[16] = {};
    for (uint8_t p : img.px) {
        if (p > 15) return fail("a pixel of colour " + std::to_string(p) + " (0-15)");
        used[p] = true;
    }
    std::vector<uint8_t> order = img.order;
    if (order.empty())
        for (int c = 0; c < 16; c++)
            if (used[c]) order.push_back((uint8_t)c);
    if (order.size() > 16) return fail("more than 16 colours");
    int rankOf[16];
    std::fill(rankOf, rankOf + 16, -1);
    for (size_t r = 0; r < order.size(); r++) {
        if (order[r] > 15) return fail("an order entry over 15");
        if (rankOf[order[r]] < 0) rankOf[order[r]] = (int)r;
    }
    for (int c = 0; c < 16; c++)
        if (used[c] && rankOf[c] < 0) return fail("colour " + std::to_string(c) + " is not in the order");
    int n = (int)order.size(), w = img.w, h = img.h;
    std::vector<int> ranks(img.px.size());
    for (size_t i = 0; i < img.px.size(); i++) ranks[i] = rankOf[img.px[i]];

    auto code = [&](int left) {
        std::vector<uint8_t> d(ranks.size());
        std::vector<uint64_t> counts(n, 0);
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++) {
                int above = y ? ranks[(size_t)(y - 1) * w + x] : 0;
                int p = left && x ? ranks[(size_t)y * w + x - 1] : above;
                int s = ((ranks[(size_t)y * w + x] - p) % n + n) % n;
                d[(size_t)y * w + x] = (uint8_t)s;
                counts[s]++;
            }
        std::vector<int> len = huffmanLengths(counts);
        // canonical codes: by length, then symbol
        std::vector<unsigned> cw(n, 0);
        unsigned c = 0;
        for (int l = 1; l <= kMaxCode; l++) {
            for (int s = 0; s < n; s++)
                if (len[s] == l) cw[s] = c++;
            c <<= 1;
        }
        std::vector<uint8_t> s;
        s.push_back(kFormatImage);
        for (int k = 0; k < 4; k++) s.push_back((uint8_t)(dest >> (8 * k)));
        s.push_back((uint8_t)(w / 2));
        s.push_back((uint8_t)h);
        s.push_back((uint8_t)(h >> 8));
        s.push_back((uint8_t)n);
        s.push_back((uint8_t)left);
        s.insert(s.end(), order.begin(), order.end());
        for (int l : len) s.push_back((uint8_t)l);
        uint32_t acc = 0;
        int nbits = 0;
        for (uint8_t v : d) {
            acc = (acc << len[v]) | cw[v];
            nbits += len[v];
            while (nbits >= 8) {
                nbits -= 8;
                s.push_back((uint8_t)(acc >> nbits));
            }
            acc &= (1u << nbits) - 1;
        }
        if (nbits) s.push_back((uint8_t)(acc << (8 - nbits)));
        // the decoder's read-ahead: zero bytes up to the last one it takes,
        // and the in-place offset (a row's end against the bytes read)
        size_t need = s.size();
        long ahead = 0;
        auto rd = [&](size_t i) -> uint8_t { return i < s.size() ? s[i] : 0; };
        decodeImage(rd, (size_t)-1, [&](int y, const std::vector<uint8_t>&, size_t taken) {
            need = std::max(need, taken);
            ahead = std::max(ahead, (long)(y + 1) * (w / 2) - (long)taken);
            return true;
        }, nullptr, nullptr);
        s.resize(need, 0);
        Packed p;
        p.stream = s;
        p.inPlace = (uint32_t)std::max(0L, ahead);
        return p;
    };
    if (predict == 0 || predict == 1) {
        out = code(predict);
        if (chosen) *chosen = predict;
    } else {
        Packed a = code(0), l = code(1);
        bool useLeft = l.stream.size() < a.stream.size();
        out = useLeft ? l : a;
        if (chosen) *chosen = useLeft ? 1 : 0;
    }
    return true;
}

bool unpack(const std::vector<uint8_t>& s, std::vector<uint8_t>& out, uint32_t* dest, std::string* err) {
    out.clear();
    if (s.empty()) {
        if (err) *err = "too short";
        return false;
    }
    if (s[0] == kFormat) return unpack1(s, out, dest, err);
    if (s[0] == kFormatImage)
        return decodeImage([&](size_t i) { return s[i]; }, s.size(),
                           [&](int, const std::vector<uint8_t>& b, size_t) {
                               out.insert(out.end(), b.begin(), b.end());
                               return true;
                           }, dest, err);
    if (err) *err = "format " + std::to_string(s[0]) + " (not 1 or 2)";
    return false;
}

bool checkInPlace(const std::vector<uint8_t>& data, const Packed& p, uint32_t offset) {
    if (p.stream.empty()) return false;
    if (p.stream[0] == kFormat) return checkInPlace1(data, p, offset);
    // memory: the stream at offset, each row written as the decoder writes
    // it; a row must not reach a byte still to be read
    size_t size = std::max(data.size(), (size_t)offset + p.stream.size()) + 1;
    std::vector<uint8_t> mem(size, 0xEE);
    std::memcpy(mem.data() + offset, p.stream.data(), p.stream.size());
    size_t w = 0;
    bool ok = decodeImage([&](size_t i) { return mem[offset + i]; }, p.stream.size(),
                          [&](int, const std::vector<uint8_t>& b, size_t taken) {
                              if (w + b.size() > offset + taken) return false;
                              for (uint8_t v : b) mem[w++] = v;
                              return true;
                          }, nullptr, nullptr);
    return ok && w == data.size() && std::equal(data.begin(), data.end(), mem.begin());
}

}  // namespace pack45
