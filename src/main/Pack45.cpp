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
// repeat of any length up to the longest found, or literals). a[i]: the
// least bytes for data[i..] starting at a token; b[i]: inside a literal
// block, data[i] one of its bytes (a block's byte of header counted at its
// first byte only: blocks over 64 cost one more each, a little off)
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
    std::vector<int> a(n + 1, kInf), b(n + 1, kInf), len(n + 1, 0), off(n + 1, 0);
    a[n] = 0;
    for (int i = n - 1; i >= 0; i--) {
        b[i] = 1 + std::min(b[i + 1], a[i + 1]);
        int best = 1 + b[i];
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
            // literals: on while staying in the block is the cheaper way
            int j = i + 1;
            while (j < n && b[j] <= a[j]) j++;
            out.literals(data.data() + i, j - i);
            i = j;
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

bool unpack(const std::vector<uint8_t>& s, std::vector<uint8_t>& out, uint32_t* dest, std::string* err) {
    auto fail = [&](const std::string& m) { if (err) *err = m; return false; };
    out.clear();
    if (s.size() < kHeader + 3) return fail("too short");
    if (s[0] != kFormat) return fail("format " + std::to_string(s[0]) + " (not 1)");
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

bool checkInPlace(const std::vector<uint8_t>& data, const Packed& p, uint32_t offset) {
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

}  // namespace pack45
