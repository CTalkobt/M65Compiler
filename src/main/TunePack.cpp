// TunePack.cpp — a tune's data coded for the player to read as it plays
// (TunePack.hpp): the plain data's streams flattened to events, their
// repeats found as patterns (a suffix array of the voices one after
// another, the run that saves the most bits first, nested two deep,
// transposed by the notes' intervals), then each field Huffman-coded in
// its own context.
#include "TunePack.hpp"
#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <queue>
#include <set>

namespace tunepack {

namespace {

// an event's argument bytes after its op (a SID voice's, a digi voice's)
int sidArgs(uint8_t c) {
    if (c < 0x80) return 1;
    switch (c) {
    case 0x80: case 0x81: case 0x82: return 1;
    case 0x83: case 0x84: case 0x85: case 0x88: return 2;
    case 0x86: case 0x89: case 0x8A: return 3;
    case 0x8B: return 4;
    default: return 0;
    }
}
int digiArgs(uint8_t c) {
    if (c < 0x80) return 1;
    switch (c) {
    case 0x80: case 0x81: case 0x82: case 0x83: return 1;
    case 0x84: return 2;
    case 0x86: return 3;
    default: return 0;
    }
}

// the contexts of an event's bytes after its op
std::vector<int> argCtx(bool digi, uint8_t c) {
    if (digi) {
        if (c < 0x80 || c == 0x80 || c == 0x81 || c == 0x82) return {DUR};
        if (c == 0x83) return {DVOL};
        if (c == 0x84) return {DHIT, DUR};
        return {};
    }
    if (c < 0x80 || c == 0x80 || c == 0x82) return {DUR};
    switch (c) {
    case 0x81: return {INST};
    case 0x83: return {ARPA, ARPB};
    case 0x84: return {GLS, GLK};
    case 0x85: return {TLO, THI};
    case 0x88: return {LNOTE, DUR};
    case 0x89: return {PWLO, PWHI, PWS};
    case 0x8A: return {CLO, CHI, CS};
    default: return {};
    }
}

struct Event {
    bool digi = false;
    std::vector<uint8_t> b;          // op and arguments
    bool operator<(const Event& o) const { return digi != o.digi ? digi < o.digi : b < o.b; }
};

// an element of a sequence: an event (by its number) or a call
struct El {
    int ev = -1;
    int call = -1, tr = 0, rep = 1;
};

// canonical Huffman code lengths of counts (at most kMaxLen bits; at most
// 255 codes a length)
std::map<int, int> lengths(std::map<int, long> cnt) {
    std::map<int, int> len;
    if (cnt.size() <= 1) {
        for (auto& k : cnt) len[k.first] = 0;
        return len;
    }
    for (;;) {
        std::priority_queue<std::pair<long, int>, std::vector<std::pair<long, int>>, std::greater<>> q;
        std::vector<int> parent, sym;
        for (auto& [s, n] : cnt) { q.push({n, (int)parent.size()}); parent.push_back(-1); sym.push_back(s); }
        size_t leaves = parent.size();
        while (q.size() > 1) {
            auto a = q.top(); q.pop();
            auto b = q.top(); q.pop();
            int id = (int)parent.size();
            parent.push_back(-1);
            parent[(size_t)a.second] = id;
            parent[(size_t)b.second] = id;
            q.push({a.first + b.first, id});
        }
        len.clear();
        int most = 0;
        std::map<int, int> per;
        for (size_t i = 0; i < leaves; i++) {
            int d = 0;
            for (int j = (int)i; parent[(size_t)j] >= 0; j = parent[(size_t)j]) d++;
            len[sym[i]] = d;
            most = std::max(most, d);
            per[d]++;
        }
        bool full = false;
        for (auto& [l, n] : per) full |= n > 255;
        if (most <= kMaxLen && !full) return len;
        if (full) cnt.begin()->second = cnt.begin()->second * 4 + 1;   // (256 alike: one made shorter)
        else for (auto& k : cnt) k.second = (k.second + 1) / 2;       // (too long: flatter)
    }
}

struct Code {
    std::map<int, int> len;          // symbol: its length
    std::map<int, uint32_t> bits;    // symbol: its code
    int most = 0;
    std::vector<uint8_t> table() const {   // L, then the counts, then the symbols
        if (most == 0) return {0, (uint8_t)(len.empty() ? 0 : len.begin()->first)};
        std::vector<uint8_t> t = {(uint8_t)most};
        std::vector<std::pair<int, int>> order;   // (length, symbol)
        for (auto& [s, l] : len) order.push_back({l, s});
        std::sort(order.begin(), order.end());
        for (int l = 1; l <= most; l++) {
            int n = 0;
            for (auto& o : order) n += o.first == l;
            t.push_back((uint8_t)n);
        }
        for (auto& o : order) t.push_back((uint8_t)o.second);
        return t;
    }
};

Code makeCode(const std::map<int, long>& cnt) {
    Code c;
    c.len = lengths(cnt);
    for (auto& [s, l] : c.len) c.most = std::max(c.most, l);
    std::vector<std::pair<int, int>> order;
    for (auto& [s, l] : c.len) order.push_back({l, s});
    std::sort(order.begin(), order.end());
    uint32_t code = 0;
    int at = order.empty() ? 0 : order[0].first;
    for (auto& [l, s] : order) {
        while (at < l) { code <<= 1; at++; }
        c.bits[s] = code++;
    }
    return c;
}

struct Bits {
    std::vector<uint8_t> out;
    int used = 8;                    // bits of out.back() taken
    void put(uint32_t v, int n) {
        for (int k = n - 1; k >= 0; k--) {
            if (used == 8) { out.push_back(0); used = 0; }
            if ((v >> k) & 1) out.back() |= (uint8_t)(0x80 >> used);
            used++;
        }
    }
    void align() { used = 8; }
};

}  // namespace

std::vector<uint8_t> pack(const std::vector<uint8_t>& B, std::string* stats) {
    auto byte = [&](size_t at) -> uint8_t { return at < B.size() ? B[at] : 0xFF; };
    if (B.size() < 8 || (B[0] & 0x20)) return {};
    int nv = B[1], nd = (B[0] >> 1) & 7;
    int ninst = B[4], isize = B[5] ? B[5] : 10;
    size_t insAt = byte(6) | byte(7) << 8;
    std::vector<size_t> start;
    for (int v = 0; v < nv + nd; v++) start.push_back(byte(8 + 2 * v) | byte(9 + 2 * v) << 8);
    size_t stab = nd ? (byte(8 + 2 * (nv + nd)) | byte(9 + 2 * (nv + nd)) << 8) : 0;
    // the raw region after the instruments (wave tables, the sample table) up to the first stream
    size_t rawFrom = insAt + (size_t)ninst * isize, rawTo = B.size();
    for (size_t s : start) rawTo = std::min(rawTo, s);
    if (rawTo < rawFrom) return {};

    // the voices' events, flattened (pattern calls expanded, transposed)
    std::vector<Event> evs;
    std::map<Event, int> evId;
    auto intern = [&](const Event& e) {
        auto it = evId.find(e);
        if (it != evId.end()) return it->second;
        evId[e] = (int)evs.size();
        evs.push_back(e);
        return (int)evs.size() - 1;
    };
    std::vector<std::vector<El>> seq;
    std::vector<bool> seqDigi;
    bool bad = false;
    std::function<void(size_t, int, bool, std::vector<El>&, int)> flat = [&](size_t at, int tr, bool digi, std::vector<El>& out, int depth) {
        if (depth > 2) { bad = true; return; }
        for (size_t guard = 0; guard < B.size() && !bad; guard++) {
            uint8_t c = byte(at);
            int n = digi ? digiArgs(c) : sidArgs(c);
            Event e;
            e.digi = digi;
            for (int k = 0; k <= n; k++) e.b.push_back(byte(at + (size_t)k));
            at += 1 + (size_t)n;
            if (c == 0x86 || (c == 0x8B && !digi)) {
                size_t p = e.b[1] | e.b[2] << 8;
                int t = c == 0x8B ? (int8_t)e.b[4] : 0;
                for (int r = 0; r < e.b[3]; r++) flat(p, tr + t, digi, out, depth + 1);
                continue;
            }
            if (c == 0x87) return;
            if (!digi && c < 0x80) e.b[0] = (uint8_t)((c + tr) & 0x7F);
            if (!digi && c == 0x88) e.b[1] = (uint8_t)((e.b[1] + tr) & 0x7F);
            out.push_back({intern(e)});
            if (c == 0xFE || c == 0xFF) return;
        }
        bad = true;
    };
    for (int v = 0; v < nv + nd; v++) {
        seq.emplace_back();
        seqDigi.push_back(v >= nv);
        flat(start[(size_t)v], 0, v >= nv, seq.back(), 0);
    }
    if (bad) return {};

    // an event's cost in bits (its fields' -log2 p, from the flat counts)
    std::vector<std::map<int, long>> cnt(NCTX);
    std::vector<long> tot(NCTX, 0);
    auto count = [&](const Event& e) {
        int oc = e.digi ? DOP : OP;
        cnt[oc][e.b[0]]++;
        tot[oc]++;
        auto ac = argCtx(e.digi, e.b[0]);
        for (size_t k = 0; k < ac.size() && k + 1 < e.b.size(); k++) { cnt[(size_t)ac[k]][e.b[k + 1]]++; tot[(size_t)ac[k]]++; }
    };
    for (auto& s : seq) for (auto& e : s) count(evs[(size_t)e.ev]);
    auto cost = [&](int c, int sym) {
        auto it = cnt[(size_t)c].find(sym);
        double p = it == cnt[(size_t)c].end() ? 0.5 / std::max(1L, tot[(size_t)c]) : (double)it->second / tot[(size_t)c];
        return std::max(1.0, -std::log2(p));
    };
    std::vector<double> evBits(evs.size());
    for (size_t i = 0; i < evs.size(); i++) {
        const Event& e = evs[i];
        double b = cost(e.digi ? DOP : OP, e.b[0]);
        auto ac = argCtx(e.digi, e.b[0]);
        for (size_t k = 0; k < ac.size() && k + 1 < e.b.size(); k++) b += cost(ac[k], e.b[k + 1]);
        evBits[i] = b;
    }
    const double kCall = 12, kRule = 26;   // a call's bits (op, number, times); a pattern's own (its end, its offset, alignment)

    // the patterns: each time the run that saves the most, its copies called
    // (depth: a pattern's calls below it and itself; inner: called in a
    // pattern. A copy inside a pattern's body makes it deeper: only in one
    // called from the voices alone, and not past kDepth)
    std::vector<std::vector<El>> rules;
    std::vector<int> depth;
    std::vector<bool> ruleDigi, inner;
    auto noteOf = [&](const El& e) {
        if (e.call >= 0) return -1;
        const Event& x = evs[(size_t)e.ev];
        if (x.digi) return -1;
        return x.b[0] < 0x80 ? (int)x.b[0] : x.b[0] == 0x88 ? (int)x.b[1] : -1;
    };
    for (int round = 0; round < 255; round++) {
        int ns = (int)seq.size() + (int)rules.size();
        auto S = [&](int s) -> std::vector<El>& { return s < (int)seq.size() ? seq[(size_t)s] : rules[(size_t)s - seq.size()]; };
        // the sequences one after another: an element's id and its note's interval
        struct Sym { long long id; int delta; bool note; };
        std::vector<Sym> all;
        std::vector<std::pair<int, size_t>> from;
        long long unique = -1;
        for (int s = 0; s < ns; s++) {
            int prev = -1;
            auto& x = S(s);
            for (size_t i = 0; i < x.size(); i++) {
                const El& e = x[i];
                if (e.call >= 0) all.push_back({(1LL << 40) + e.call * 65536LL + (e.tr & 255) * 256 + e.rep, 0, false});
                else {
                    const Event& ev = evs[(size_t)e.ev];
                    uint8_t c = ev.b[0];
                    if (c == 0x8C || c == 0xFE || c == 0xFF) all.push_back({unique--, 0, false});   // (never in a pattern)
                    else if (!ev.digi && (c < 0x80 || c == 0x88)) {   // a note: its kind and duration, its interval
                        int n = noteOf(e);
                        all.push_back({(1LL << 32) + (c < 0x80 ? 0 : 0x10000) + ev.b[c < 0x80 ? 1 : 2], prev < 0 ? 1000 : n - prev, true});
                        prev = n;
                    } else all.push_back({(long long)e.ev, 0, false});
                }
                from.push_back({s, i});
            }
            all.push_back({unique--, 0, false});
            from.push_back({s, x.size()});
        }
        auto common = [&](size_t a, size_t b, size_t limit) {
            size_t n = 0;
            bool first = true;
            while (a + n < all.size() && b + n < all.size() && n < limit) {
                const Sym &x = all[a + n], &y = all[b + n];
                if (x.id < 0 || x.id != y.id) break;
                if (!x.note && x.delta != y.delta) break;
                if (x.note) {
                    if (!first && x.delta != y.delta) break;
                    first = false;
                }
                n++;
            }
            return n;
        };
        std::vector<size_t> sa(all.size());
        for (size_t i = 0; i < sa.size(); i++) sa[i] = i;
        std::sort(sa.begin(), sa.end(), [&](size_t a, size_t b) {
            size_t n = common(a, b, all.size());
            if (a + n >= all.size() || b + n >= all.size()) return a + n >= all.size() && b + n < all.size();
            const Sym &x = all[a + n], &y = all[b + n];
            if (x.id != y.id) return x.id < y.id;
            return x.delta < y.delta;
        });
        std::vector<size_t> lcp(sa.size(), 0);
        for (size_t k = 1; k < sa.size(); k++) lcp[k] = common(sa[k - 1], sa[k], 512);
        // a candidate's copies (place, transposition), not overlapping; a
        // body that calls is copied untransposed only (its calls' own)
        auto copies = [&](size_t a, size_t len, const std::vector<size_t>& at, int dn) {
            std::vector<std::pair<size_t, int>> out;
            int s0 = from[a].first, n0 = -1;
            bool calls = false;
            for (size_t q = 0; q < len; q++) {
                const El& e = S(s0)[from[a + q].second];
                calls |= e.call >= 0;
                if (n0 < 0) n0 = noteOf(e);
            }
            size_t end = 0;
            for (size_t p : at) {
                if (!out.empty() && p < end) continue;
                int s = from[p].first, tr = 0;
                if (s >= (int)seq.size() && (dn + 1 > kDepth || inner[(size_t)s - seq.size()])) continue;
                if (n0 >= 0)
                    for (size_t q = 0; q < len; q++) {
                        int n = noteOf(S(s)[from[p + q].second]);
                        if (n >= 0) { tr = n - n0; break; }
                    }
                if (tr < -96 || tr > 96 || (tr && calls)) continue;
                out.push_back({p, tr});
                end = p + len;
            }
            return out;
        };
        double bestGain = 0;
        size_t bestAt = 0, bestLen = 0;
        std::vector<size_t> bestPos;
        std::set<std::pair<size_t, size_t>> tried;
        for (size_t k = 1; k < sa.size(); k++) {
            size_t len = lcp[k];
            if (len < 2) continue;
            size_t lo = k - 1, hi = k;
            while (lo > 0 && lcp[lo] >= len) lo--;
            while (hi + 1 < sa.size() && lcp[hi + 1] >= len) hi++;
            if (!tried.insert({len, lo}).second) continue;
            std::vector<size_t> pos(sa.begin() + (long)lo, sa.begin() + (long)hi + 1);
            std::sort(pos.begin(), pos.end());
            int sb = from[pos[0]].first;
            double body = 0;
            int dmax = 0;
            for (size_t q = 0; q < len; q++) {
                const El& e = S(sb)[from[pos[0] + q].second];
                body += e.call >= 0 ? kCall : evBits[(size_t)e.ev];
                if (e.call >= 0) dmax = std::max(dmax, depth[(size_t)e.call]);
            }
            if (dmax + 1 > kDepth) continue;
            auto cs = copies(pos[0], len, pos, dmax + 1);
            if (cs.size() < 2) continue;
            double gain = (double)(cs.size() - 1) * body - kRule - (double)cs.size() * kCall;
            if (gain > bestGain) { bestGain = gain; bestAt = pos[0]; bestLen = len; bestPos = pos; }
        }
        if (!bestLen) break;
        int rid = (int)rules.size();
        int sb = from[bestAt].first;
        std::vector<El> body;
        int dmax = 0;
        for (size_t q = 0; q < bestLen; q++) {
            body.push_back(S(sb)[from[bestAt + q].second]);
            if (body.back().call >= 0) dmax = std::max(dmax, depth[(size_t)body.back().call]);
        }
        auto cs = copies(bestAt, bestLen, bestPos, dmax + 1);
        std::map<int, std::map<size_t, int>> callAt;   // sequence: place: transposition
        for (auto& [p, tr] : cs) callAt[from[p].first][from[p].second] = tr;
        bool digi = sb < (int)seq.size() ? seqDigi[(size_t)sb] : ruleDigi[(size_t)sb - seq.size()];
        rules.push_back(body);
        depth.push_back(dmax + 1);
        ruleDigi.push_back(digi);
        inner.push_back(false);
        for (auto& e : body) if (e.call >= 0) inner[(size_t)e.call] = true;
        for (auto& [sq, places] : callAt)
            if (sq >= (int)seq.size()) {
                inner[(size_t)rid] = true;
                depth[(size_t)sq - seq.size()] = std::max(depth[(size_t)sq - seq.size()], dmax + 2);
            }
        for (auto& [s, places] : callAt) {
            std::vector<El> y;
            auto& x = S(s);
            for (size_t i = 0; i < x.size();) {
                auto it = places.find(i);
                if (it != places.end()) {
                    if (!y.empty() && y.back().call == rid && y.back().tr == it->second && y.back().rep < 255) y.back().rep++;
                    else y.push_back({-1, rid, it->second, 1});
                    i += bestLen;
                } else y.push_back(x[i++]);
            }
            x = y;
        }
    }

    // the codes: every field counted where it is now
    std::vector<std::map<int, long>> fc(NCTX);
    auto countSeq = [&](const std::vector<El>& x, bool digi, bool rule) {
        for (auto& e : x) {
            if (e.call >= 0) {
                fc[digi ? DOP : OP][e.tr ? 0x8B : 0x86]++;
                fc[PAT][e.call]++;
                fc[REP][e.rep]++;
                if (e.tr) fc[TR][e.tr & 255]++;
                continue;
            }
            const Event& ev = evs[(size_t)e.ev];
            fc[digi ? DOP : OP][ev.b[0]]++;
            auto ac = argCtx(digi, ev.b[0]);
            for (size_t k = 0; k < ac.size() && k + 1 < ev.b.size(); k++) fc[(size_t)ac[k]][ev.b[k + 1]]++;
        }
        if (rule) fc[digi ? DOP : OP][0x87]++;
    };
    for (size_t s = 0; s < seq.size(); s++) countSeq(seq[s], seqDigi[s], false);
    for (size_t r = 0; r < rules.size(); r++) countSeq(rules[r], ruleDigi[r], true);
    // (the instruments' bytes as they will be: a wave table's offset moved
    // with the tables, after the header grown by 4)
    size_t hdr = 8 + 2 * (size_t)(nv + nd) + (nd ? 2 : 0), rawAt = hdr + 4;
    auto moved = [&](size_t off) { return off - rawFrom + rawAt; };
    std::vector<std::vector<uint8_t>> ins;
    for (int i = 0; i < ninst; i++) {
        std::vector<uint8_t> r(B.begin() + (long)(insAt + (size_t)i * isize), B.begin() + (long)(insAt + (size_t)(i + 1) * isize));
        if (isize >= 17) {
            size_t ta = r[15] | r[16] << 8;
            if (ta) { ta = moved(ta); r[15] = (uint8_t)(ta & 0xFF); r[16] = (uint8_t)(ta >> 8); }
        }
        ins.push_back(r);
    }
    // each group's entries (in the order first used), each instrument's
    int groups = isize >= 19 ? 6 : 5;
    if (isize < 17) return {};
    std::vector<std::map<std::vector<uint8_t>, int>> dict((size_t)groups);
    std::vector<std::vector<std::vector<uint8_t>>> dictList((size_t)groups);
    std::vector<std::vector<uint8_t>> rec;
    for (auto& r : ins) {
        std::vector<uint8_t> x;
        int f = 0;
        for (int g = 0; g < groups; g++) {
            std::vector<uint8_t> v;
            for (int k = 0; k < kGroupSize[g]; k++) v.push_back(r[(size_t)kGroupField[f + k]]);
            f += kGroupSize[g];
            auto it = dict[(size_t)g].find(v);
            if (it == dict[(size_t)g].end()) {
                it = dict[(size_t)g].emplace(v, (int)dictList[(size_t)g].size()).first;
                dictList[(size_t)g].push_back(v);
            }
            if (it->second > 255) return {};   // (more than 256 of a group: plain)
            x.push_back((uint8_t)it->second);
        }
        rec.push_back(x);
    }
    std::vector<Code> code(NCTX);
    for (int c = 0; c < NCTX; c++) code[(size_t)c] = makeCode(fc[(size_t)c]);

    // the data
    std::vector<uint8_t> out(B.begin(), B.begin() + (long)hdr);
    out[0] |= 0x20;
    out.resize(hdr + 4, 0);
    auto put16 = [&](size_t at, size_t v) { out[at] = (uint8_t)(v & 0xFF); out[at + 1] = (uint8_t)(v >> 8); };
    // the wave tables and the sample table, as they were (moved)
    out.insert(out.end(), B.begin() + (long)rawFrom, B.begin() + (long)rawTo);
    if (nd) put16(8 + 2 * (size_t)(nv + nd), moved(stab));
    // the code tables
    put16(hdr, out.size());
    for (auto& c : code) { auto t = c.table(); out.insert(out.end(), t.begin(), t.end()); }
    // the groups' dictionaries, then the instruments (a byte a group)
    for (int g = 0; g < groups; g++) {
        out.push_back((uint8_t)(dictList[(size_t)g].size() & 0xFF));
        for (auto& v : dictList[(size_t)g]) out.insert(out.end(), v.begin(), v.end());
    }
    put16(6, out.size());
    for (auto& x : rec) out.insert(out.end(), x.begin(), x.end());
    // the pattern table, the streams, the patterns' bodies
    put16(hdr + 2, out.size());
    size_t pt = out.size();
    out.resize(out.size() + 2 * rules.size(), 0);
    auto emit = [&](const std::vector<El>& x, bool digi, bool rule) {
        Bits bb;
        auto sym = [&](int c, int s) { const Code& k = code[(size_t)c]; bb.put(k.bits.at(s), k.len.at(s)); };
        for (auto& e : x) {
            if (e.call >= 0) {
                sym(digi ? DOP : OP, e.tr ? 0x8B : 0x86);
                sym(PAT, e.call);
                sym(REP, e.rep);
                if (e.tr) sym(TR, e.tr & 255);
                continue;
            }
            const Event& ev = evs[(size_t)e.ev];
            sym(digi ? DOP : OP, ev.b[0]);
            auto ac = argCtx(digi, ev.b[0]);
            for (size_t k = 0; k < ac.size() && k + 1 < ev.b.size(); k++) sym(ac[k], ev.b[k + 1]);
        }
        if (rule) sym(digi ? DOP : OP, 0x87);
        out.insert(out.end(), bb.out.begin(), bb.out.end());
    };
    for (size_t s = 0; s < seq.size(); s++) {
        put16(8 + 2 * s, out.size());
        emit(seq[s], seqDigi[s], false);
    }
    for (size_t r = 0; r < rules.size(); r++) {
        put16(pt + 2 * r, out.size());
        emit(rules[r], ruleDigi[r], true);
    }
    if (out.size() > 0xFFFF) return {};
    if (stats) {
        size_t flatEv = 0, calls = 0;
        for (auto& s : seq) for (auto& e : s) calls += e.call >= 0;
        for (auto& r : rules) for (auto& e : r) calls += e.call >= 0;
        for (auto& s : evs) flatEv += s.b.size();
        *stats = std::to_string(rules.size()) + " patterns, " + std::to_string(calls) + " calls";
    }
    return out;
}

void Reader::init(const std::vector<uint8_t>& data) {
    d = &data;
    int nv = byte(1), nd = (byte(0) >> 1) & 7;
    unsigned h = 8 + 2 * (unsigned)(nv + nd) + (nd ? 2 : 0);
    unsigned at = byte(h) | byte(h + 1) << 8;
    for (int c = 0; c < NCTX; c++) {
        tab[c] = (uint16_t)at;
        uint8_t L = byte(at);
        if (!L) { at += 2; continue; }
        unsigned n = 0;
        for (unsigned k = 1; k <= L; k++) n += byte(at + k);
        at += 1 + L + n;
    }
    int groups = byte(5) >= 19 ? 6 : 5;   // (the dictionaries after the tables)
    for (int g = 0; g < groups; g++) {
        unsigned n = byte(at) ? byte(at) : 256;
        dict[g] = (uint16_t)(at + 1);
        at += 1 + n * (unsigned)kGroupSize[g];
    }
}

void Reader::instrument(unsigned at, int isize, uint8_t* r) const {
    int groups = isize >= 19 ? 6 : 5, f = 0;
    for (int g = 0; g < groups; g++) {
        unsigned e = dict[g] + byte(at + (unsigned)g) * (unsigned)kGroupSize[g];
        for (int k = 0; k < kGroupSize[g]; k++) r[kGroupField[f + k]] = byte(e + (unsigned)k);
        f += kGroupSize[g];
    }
}

uint8_t Reader::symbol(int c, uint16_t& pos, uint8_t& bit) const {
    unsigned t = tab[c];
    uint8_t L = byte(t);
    if (!L) return byte(t + 1);
    unsigned code = 0, first = 0, index = 0;
    for (unsigned len = 1; len <= L; len++) {
        code = (code << 1) | ((byte(pos) >> (7 - bit)) & 1);
        if (++bit == 8) { bit = 0; pos++; }
        unsigned count = byte(t + len);
        if (code - first < count) return byte(t + 1 + L + index + (code - first));
        index += count;
        first = (first + count) << 1;
    }
    return 0xFF;   // (not a code)
}

}  // namespace tunepack
