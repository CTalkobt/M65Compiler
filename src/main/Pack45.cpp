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

Packed pack(const std::vector<uint8_t>& data, uint32_t dest) {
    Packed p;
    auto& s = p.stream;
    s.push_back(kFormat);
    for (int k = 0; k < 4; k++) s.push_back((uint8_t)(dest >> (8 * k)));
    Packer pk(data);
    // the in-place offset: the most the writing gets ahead of the reading,
    // at the end of each token (a literal block's bytes are read as they
    // are copied, forwards)
    long w = 0;
    long ahead = 0;
    auto mark = [&]() { ahead = std::max(ahead, w - (long)s.size()); };
    std::vector<uint8_t> lits;
    auto flush = [&]() {
        size_t k = 0;
        while (k < lits.size()) {
            size_t c = std::min(lits.size() - k, (size_t)kMaxLit);
            s.push_back((uint8_t)(c - 1));
            s.insert(s.end(), lits.begin() + k, lits.begin() + k + c);
            w += (long)c;
            mark();
            k += c;
        }
        lits.clear();
    };
    auto emitMatch = [&](const Match& m, int i) {
        flush();
        if (m.fill) {
            s.push_back((uint8_t)(0x40 | (m.len - 3)));
            s.push_back(data[i]);
        } else if (m.off <= 256) {
            s.push_back((uint8_t)(0x80 | (m.len - 3)));
            s.push_back((uint8_t)(m.off - 1));
        } else {
            s.push_back((uint8_t)(0xC0 | (m.len - 3)));
            s.push_back((uint8_t)m.off);
            s.push_back((uint8_t)(m.off >> 8));
        }
        w += m.len;
        mark();
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
        emitMatch(m, i);
        for (int k = 1; k < m.len; k++) pk.insert(i + k);
        i += m.len;
    }
    flush();
    s.push_back(0xC0);
    s.push_back(0);
    s.push_back(0);
    mark();
    p.inPlace = (uint32_t)ahead;
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
