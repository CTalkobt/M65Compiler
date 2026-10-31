// TuneSid.cpp — tune45 import of a .sid (TuneSid.hpp)
#include "TuneSid.hpp"
#include "TuneCompiler.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <set>

namespace {

// a voice's registers after a call, and whether its note began in it
struct Frame {
    bool onset = false, gate = false;
    uint8_t ctrl = 0, ad = 0, sr = 0;
    uint16_t freq = 0, pw = 0;
    bool filtered = false;          // routed through its chip's filter ($17)
    uint8_t mode = 0, res = 0;      // the chip's filter ($18 bits 4-6, $17 bits 4-7)
    uint16_t cutoff = 0;
};

// a note: frames [at, at + len), its instrument and pitches
struct Note {
    size_t at = 0, len = 0;
    std::string inst;
    int base = 0;                   // its note
    int arpA = 0, arpB = 0;         // an arpeggio (0, 0: none)
    std::vector<std::pair<int, size_t>> legato;   // (note, frames) after the first part
};

std::string waveName(uint8_t w) {
    static const char* n[4] = {"tri", "saw", "pulse", "noise"};
    std::string s;
    if (w & 0x0F) {   // (ring, sync, test: as a hex control value)
        char b[8];
        snprintf(b, sizeof b, "%02x", w);
        return b;
    }
    for (int k = 0; k < 4; k++)
        if (w & (0x10 << k)) s += (s.empty() ? "" : "+") + std::string(n[k]);
    return s.empty() ? "00" : s;
}

std::string ident(std::string n) {
    for (auto& c : n) c = std::isalnum((unsigned char)c) ? (char)std::tolower((unsigned char)c) : '_';
    while (n.find("__") != std::string::npos) n.erase(n.find("__"), 1);
    while (!n.empty() && n.back() == '_') n.pop_back();
    while (!n.empty() && n[0] == '_') n.erase(0, 1);
    if (n.empty() || std::isdigit((unsigned char)n[0])) n = "sid_" + n;
    return n;
}

}  // namespace

std::string sidImport(const PsidFile& p, const SidImportOptions& o, std::string& error) {
    int song = o.song ? o.song : p.startSong;
    SidRun run;
    // (a first call to learn the rate, then as many as the seconds take)
    if (!runPsid(p, song, 1, run, error)) return "";
    int calls = std::max(1, (int)std::lround((o.findLoop ? std::max(o.seconds, o.search) : o.seconds) * run.callsPerSecond));
    if (!runPsid(p, song, calls, run, error)) return "";
    int chips = 1 + (p.sid2 ? 1 : 0) + (p.sid3 ? 1 : 0);
    int nv = chips * 3;
    const double clock = p.ntsc() ? 1022727.0 : 985248.0;

    // the registers frame by frame
    uint8_t regs[3][0x20] = {};
    for (int k = 0; k < 0x20; k++) regs[0][k] = run.initRegs[(size_t)k];
    for (const auto& w : run.init.writes)
        if (w.chip) regs[w.chip][w.reg] = w.value;   // (chip 0's: initRegs)
    std::vector<std::vector<Frame>> fr((size_t)nv);
    int volumeWrites = 0;
    for (const auto& c : run.calls) {
        bool onset[9] = {};
        for (const auto& w : c.writes) {
            if (w.chip >= chips) continue;
            uint8_t old = regs[w.chip][w.reg];
            regs[w.chip][w.reg] = w.value;
            if (w.reg % 7 == 4 && w.reg < 0x15) {   // a control register: the gate going on, or the test bit off with it on
                int v = w.chip * 3 + w.reg / 7;
                if (((w.value & 1) && !(old & 1)) || ((old & 8) && !(w.value & 8) && (w.value & 1))) onset[v] = true;
            }
            if (w.reg == 0x18 && (w.value & 15) != (old & 15)) volumeWrites++;
        }
        for (int v = 0; v < nv; v++) {
            const uint8_t* r = regs[v / 3];
            int b = (v % 3) * 7;
            Frame f;
            f.ctrl = r[b + 4];
            f.gate = f.ctrl & 1;
            f.onset = onset[v] && f.gate;
            f.freq = (uint16_t)(r[b] | r[b + 1] << 8);
            f.pw = (uint16_t)((r[b + 2] | r[b + 3] << 8) & 0xFFF);
            f.ad = r[b + 5];
            f.sr = r[b + 6];
            f.filtered = (r[0x17] >> (v % 3)) & 1;
            f.res = r[0x17] >> 4;
            f.mode = r[0x18] & 0x70;
            f.cutoff = (uint16_t)((r[0x15] & 7) | r[0x16] << 3);
            fr[(size_t)v].push_back(f);
        }
    }
    // the loop: the shortest period L the registers keep from a call on to
    // the end (twice over and 20 s more, at least: a tenth of the search's
    // time if that is less), from its earliest call;
    // silent: the tune's end. Each voice's loop point then its first note
    // in the period from that call (or the call after it: no note crosses
    // it, and its last note ends where its first begins again), its text a
    // period past it
    size_t frames = run.calls.size();
    size_t loopFrom = SIZE_MAX, period = 0;
    bool ends = false;
    std::vector<size_t> lp((size_t)nv, SIZE_MAX), endAt((size_t)nv, frames);
    if (o.findLoop) {
        // what plays: the gate, waveform, envelope, filter alike, the cutoff
        // within 64, the pitch within a semitone (or its median over 8
        // frames, or the pitch two calls either way: a free-running vibrato,
        // arpeggio or sweep); the pulse width not (a sweep running freely);
        // a few calls amiss (a glitch) let pass
        std::vector<uint64_t> key(frames, 0);
        std::vector<std::vector<double>> pt((size_t)nv, std::vector<double>(frames)), pm = pt;
        for (size_t k = 0; k < frames; k++)
            for (int v = 0; v < nv; v++) {
                const Frame& f = fr[(size_t)v][k];
                uint64_t x = (uint64_t)f.onset | (uint64_t)(f.ctrl & 0xF7) << 1 | (uint64_t)f.ad << 9 | (uint64_t)f.sr << 17 |
                             (uint64_t)f.filtered << 25 | (uint64_t)f.mode << 26 | (uint64_t)f.res << 34;
                key[k] = (key[k] ^ x) * 0x100000001B3ull + (uint64_t)v;
                double hz = f.freq * clock / 16777216.0;
                pt[(size_t)v][k] = hz < 8 ? -100 : 69 + 12 * std::log2(hz / 440.0);
            }
        size_t w = std::max<size_t>(4, (size_t)std::lround(4 * run.callsPerSecond / 50));
        for (int v = 0; v < nv; v++)
            for (size_t k = 0; k < frames; k++) {
                size_t a = k >= w ? k - w : 0, e = std::min(frames, k + w + 1);
                std::vector<double> win(pt[(size_t)v].begin() + (long)a, pt[(size_t)v].begin() + (long)e);
                std::nth_element(win.begin(), win.begin() + (long)win.size() / 2, win.end());
                pm[(size_t)v][k] = win[win.size() / 2];
            }
        auto same = [&](size_t a, size_t b) {
            if (key[a] != key[b]) return false;
            for (int c = 0; c < chips; c++)
                if (std::abs((int)fr[(size_t)c * 3][a].cutoff - (int)fr[(size_t)c * 3][b].cutoff) > 64) return false;
            for (int v = 0; v < nv; v++) {
                const auto &P = pt[(size_t)v], &M = pm[(size_t)v];
                if (std::fabs(P[a] - P[b]) <= 1 || std::fabs(M[a] - M[b]) <= 1) continue;
                bool near = false;
                for (size_t j = b >= 2 ? b - 2 : 0; j <= std::min(frames - 1, b + 2) && !near; j++) near = std::fabs(P[a] - P[j]) <= 1;
                if (!near) return false;
            }
            return true;
        };
        // (a period of a second at least, with 8 notes begun in it: a held
        // sound changing slowly is no loop)
        size_t margin = (size_t)std::lround(std::min(20.0, o.search / 10) * run.callsPerSecond);
        std::vector<size_t> onsets(frames + 1, 0);   // (begun before each call)
        for (size_t k = 0; k < frames; k++) {
            onsets[k + 1] = onsets[k];
            for (int v = 0; v < nv; v++) onsets[k + 1] += fr[(size_t)v][k].onset;
        }
        // silent from a call to the end (the margin at least): it ends there
        size_t quiet = frames;
        while (quiet > 0) {
            bool g = false;
            for (int v = 0; v < nv; v++) g |= fr[(size_t)v][quiet - 1].gate;
            if (g) break;
            quiet--;
        }
        if (frames - quiet >= margin) { loopFrom = quiet; period = 1; }
        for (size_t L = std::max<size_t>(1, (size_t)std::lround(run.callsPerSecond)); !period && 2 * L + margin <= frames; L++) {
            size_t s = frames - L, miss = 0;
            while (s > 0) {
                if (!same(s - 1, s - 1 + L) && ++miss > 4 + (frames - L - s) / 5000) break;
                s--;
            }
            while (s < frames - L && !same(s, s + L)) s++;   // (from a call that plays as before)
            if (frames - s >= 2 * L + margin && onsets[s + L] - onsets[s] >= 8) { loopFrom = s; period = L; }
        }
    }
    if (period) {
        bool sound = false;
        for (int v = 0; v < nv; v++)
            for (size_t k = loopFrom; k < loopFrom + period && !sound; k++) sound |= fr[(size_t)v][k].gate;
        if (!sound) {   // (silent from there: it ends, its last release a second long at most)
            ends = true;
            frames = std::min(frames, loopFrom + (size_t)std::lround(run.callsPerSecond));
            period = 0;
        } else {
            size_t most = 0;
            for (int v = 0; v < nv; v++) {
                size_t at = loopFrom + 1;
                for (size_t k = loopFrom; k < loopFrom + period; k++)
                    if (fr[(size_t)v][k].onset) { at = k; break; }
                lp[(size_t)v] = at;
                endAt[(size_t)v] = at + period;
                most = std::max(most, at + period);
            }
            frames = std::min(frames, most + 64);   // (a little past: the last notes' ends seen)
        }
    } else frames = std::min(frames, (size_t)std::max(1L, std::lround(o.seconds * run.callsPerSecond)));
    for (auto& f : fr) f.resize(frames);
    for (auto& e : endAt) e = std::min(e, frames);
    auto pitch = [&](uint16_t freq) {   // the note a frequency register sounds (fractional)
        double hz = freq * clock / 16777216.0;
        return hz < 8 ? -1.0 : 69 + 12 * std::log2(hz / 440.0);
    };

    const double cpf = run.callsPerSecond / 50.0;   // calls a frame (tune45's effects run a frame)
    auto perFrame = [&](double perCall) { return (int)std::max(-127L, std::min(127L, std::lround(perCall * cpf))); };

    // a value over the calls as straight lines (within tol): each line's
    // start, value and change a call
    struct Line { size_t at; double value, slope; };
    // (tol: the error allowed at a value; a pulse near 0 or $FFF is thin: there
    // a little matters)
    auto fit = [](const std::vector<double>& x, auto tol) {
        std::vector<Line> out;
        size_t i = 0;
        while (i < x.size()) {
            double slope = 0;
            size_t j = i + 1;
            for (; j < x.size(); j++) {
                double s = (x[j] - x[i]) / (double)(j - i);
                bool ok = true;
                for (size_t q = i + 1; q <= j && ok; q++) ok = std::fabs(x[q] - (x[i] + s * (q - i))) <= tol(x[q]);
                if (!ok) break;
                slope = s;
            }
            out.push_back({i, x[i], slope});
            i = j > i + 1 ? j - 1 : i + 1;
            if (i + 1 >= x.size()) break;
        }
        return out;
    };
    auto lineAt = [](const std::vector<Line>& ls, size_t k) {
        size_t a = 0;
        for (size_t q = 0; q < ls.size() && ls[q].at <= k; q++) a = q;
        return ls[a];
    };
    // each voice's pulse width, each chip's cutoff
    std::vector<std::vector<Line>> pwLines((size_t)nv), cutLines((size_t)chips);
    for (int v = 0; v < nv; v++) {
        std::vector<double> x;
        for (const auto& f : fr[(size_t)v]) x.push_back(f.pw);
        pwLines[(size_t)v] = fit(x, [](double pw) { return std::max(1.0, std::min(48.0, std::min(pw, 4095 - pw) / 8)); });
    }
    for (int c = 0; c < chips; c++) {
        std::vector<double> x;
        for (const auto& f : fr[(size_t)c * 3]) x.push_back(f.cutoff);
        cutLines[(size_t)c] = fit(x, [](double) { return 16.0; });
    }

    // a voice's ticks: a note's start (its instrument, note, arpeggio), its
    // gate, a legato pitch, pw:/cut: tokens
    struct Row { bool start = false, gate = false; int legato = -1, note = 0; std::string inst, arp, tokens; };
    std::vector<std::vector<Row>> rows((size_t)nv, std::vector<Row>(frames));
    std::map<std::string, std::string> instOf, tableOf;
    std::vector<std::string> instLines, tableLines;
    std::vector<std::vector<std::pair<size_t, size_t>>> audible((size_t)nv);   // each note's span sounding
    // a voice's pulse width (a voice playing a pulse): one cycle running on
    // across its notes (pwrun; its limits pwlo= pwhi= where it turns at
    // steady ones), or begun again at each note (the instrument's pw and pwm);
    // either simulated a frame at a time as the player runs it, a pw: where
    // it strays from the tune's (by the tolerance of the lines above): the way
    // with fewer kept. Its instruments named then.
    struct Span { size_t at, gate, end; bool tail; };
    auto pulse = [&](int v, const std::vector<Span>& spans) {
        const auto& F = fr[(size_t)v];
        auto& R = rows[(size_t)v];
        // (the error let pass: a 16th of the way to the nearer end, at most $100: finer where it is thin)
        auto tol = [](double pw) { return std::max(2.0, std::min(256.0, std::min(pw, 4095 - pw) / 16)); };
        bool usesPulse = false;
        for (const auto& sp : spans) for (size_t k = sp.at; k < sp.end; k++) usesPulse |= (F[k].ctrl & 0x40) != 0;
        std::string none;
        auto name = [&](auto pwField, auto extra) {   // (each note's: its pw field, the rest)
            for (size_t i = 0; i < spans.size(); i++) {
                const auto& sp = spans[i];
                std::string l = R[sp.at].inst;
                l.replace(l.find("@PW@"), 4, pwField(i));
                l.replace(l.find("@PX@"), 4, extra(i));
                auto it = instOf.find(l);
                if (it == instOf.end()) {
                    char nb[16];
                    snprintf(nb, sizeof nb, "i%02zu", instOf.size() + 1);
                    it = instOf.emplace(l, nb).first;
                    instLines.push_back("instrument " + it->second + " " + l);
                }
                R[sp.at].inst = it->second;
            }
        };
        if (!usesPulse || spans.empty()) { name([](size_t) { return std::string("000"); }, [](size_t) { return std::string(); }); return; }
        // the cycles it may have: a triangle (the width turning at steady
        // limits) and a sawtooth (jumping back from a steady top to a steady
        // bottom), from where the width turns and jumps
        auto median = [](std::vector<double> x) { std::sort(x.begin(), x.end()); return x[x.size() / 2]; };
        struct Cycle { int lo = 0, hi = 0; bool saw = false; int rate = 0; };   // (rate: its step a frame)
        std::vector<Cycle> cycles = {Cycle()};
        {
            std::vector<double> peaks, troughs, tops, bottoms, rts;
            int dir = 0;
            double turnAt = F[0].pw;
            for (size_t k = 1; k < frames; k++) {
                int dpw = (int)F[k].pw - (int)F[k - 1].pw;
                if (dir > 0 && dpw <= -0x40) { tops.push_back(F[k - 1].pw); bottoms.push_back(F[k].pw); continue; }   // (a jump back)
                if (!dpw || std::abs(dpw) >= 0x40) continue;
                int sd = dpw > 0 ? 1 : -1;
                if (dir && sd != dir && std::fabs(F[k - 1].pw - turnAt) >= 0x40) (dir > 0 ? peaks : troughs).push_back(F[k - 1].pw);
                if (sd != dir) turnAt = F[k - 1].pw;
                dir = sd;
                rts.push_back(std::abs(dpw) * cpf);
            }
            auto steadyAt = [&](const std::vector<double>& a, double m) {
                int n = 0;
                for (double x : a) n += std::fabs(x - m) <= 0x60;
                return n * 10 >= (int)a.size() * 8;
            };
            if (peaks.size() >= 2 && troughs.size() >= 2) {
                double pk = median(peaks), tr = median(troughs);
                if (steadyAt(peaks, pk) && steadyAt(troughs, tr) && pk - tr >= 0x40) {
                    Cycle c;
                    c.lo = std::max(0, (int)std::lround(tr) >> 4);
                    c.hi = std::min(255, std::max(c.lo + 1, (int)std::lround(pk) >> 4));
                    c.rate = (int)std::min(127L, std::lround(median(rts)));
                    cycles.push_back(c);
                }
            }
            if (tops.size() >= 3 && !rts.empty()) {
                double tp = median(tops), bt = median(bottoms), rt = median(rts);
                if (steadyAt(tops, tp) && steadyAt(bottoms, bt) && tp - bt >= 0x40) {
                    Cycle c;
                    c.saw = true;
                    c.lo = std::max(0, (int)std::lround(bt) >> 4);
                    c.hi = std::min(255, std::max(c.lo + 1, (int)std::lround(tp + rt / cpf / 2) >> 4));
                    c.rate = (int)std::min(127L, std::lround(rt));
                    cycles.push_back(c);
                }
            }
        }
        int lo = 0, hi = 0, rate = 0;
        bool saw = false;
        auto slopeAt = [&](size_t k) {   // the width's change a frame from k (near a cycle's rate: its rate)
            size_t n = std::max<size_t>(1, (size_t)std::lround(cpf)), e = std::min(frames - 1, k + n);
            if (e == k) return 0;
            double d = ((double)F[e].pw - (double)F[k].pw) * cpf / (double)(e - k);
            if (rate && std::fabs(std::fabs(d) - rate) <= rate * 0.3) return d > 0 ? rate : -rate;
            return (int)std::max(-127L, std::min(127L, std::lround(d)));
        };
        // one way simulated: its pw: tokens (tick, value, sweep); keep: the cycle runs on
        auto simulate = [&](bool keep, const std::vector<int>& ipw, const std::vector<int>& ipwm) {
            std::vector<std::tuple<size_t, int, int>> toks;
            std::vector<char> snd(frames, 0);
            for (const auto& sp : spans) for (size_t k = sp.at; k < (sp.tail ? sp.end : sp.gate); k++) snd[k] = 1;
            int p = 0, d = 0;
            bool begun = false;
            size_t si = 0;
            for (size_t k = 0; k < endAt[(size_t)v]; k++) {
                bool onset = si < spans.size() && spans[si].at == k;
                if (onset) si++;
                if (onset) begun = true;
                if (k == lp[(size_t)v]) { p = -0x10000; d = 0; }   // (the loop point: the width as the period's end left it)
                if (onset && !keep) { p = ipw[si - 1]; d = ipwm[si - 1]; }
                int steps = (int)(std::floor((double)(k + 1) / cpf) - std::floor((double)k / cpf));
                auto step = [&](int& x, int& dir) {
                    if (!dir) return;
                    x += dir;
                    if (hi) {
                        x = std::max(0, std::min(0xFFF, x));
                        if (dir > 0 ? (x >> 4) >= hi : (x >> 4) <= lo) {
                            if (saw) x = (dir > 0 ? lo : hi) << 4;
                            else dir = -dir;
                        }
                    } else if (x < 0 || x > 0xFFF) { x = std::max(0, std::min(0xFFF, x)); dir = 0; }
                };
                bool stepping = begun && (keep || snd[k]);
                int q = p, qd = d;
                if (stepping) for (int n = 0; n < steps; n++) step(q, qd);
                bool matters = begun && snd[k] && (F[k].ctrl & 0x40);   // (where a pulse is heard)
                if (matters && std::fabs(q - F[k].pw) > tol(F[k].pw)) {
                    int s = slopeAt(k), v0 = std::max(0, std::min(0xFFF, (int)F[k].pw - s * steps));
                    toks.push_back({k, v0, s});
                    p = v0;
                    d = s;
                    q = p;
                    qd = d;
                    if (stepping) for (int n = 0; n < steps; n++) step(q, qd);
                }
                p = q;
                d = qd;
            }
            return toks;
        };
        // the begun-again way: each note's instrument its start and its first sweep
        std::vector<int> ipw, ipwm;
        for (const auto& sp : spans) {
            ipw.push_back((int)F[sp.at].pw & (F[sp.at].pw < 0x100 || F[sp.at].pw > 0xEFF ? 0xFFF : 0xFF0));
            ipwm.push_back(slopeAt(sp.at));
        }
        // every way: no cycle or a cycle found; running on, or begun again at notes
        std::vector<std::tuple<size_t, int, int>> use;
        bool keep = true;
        Cycle chosen;
        size_t bestN = SIZE_MAX;
        for (const auto& c : cycles)
            for (bool kp : {true, false}) {
                lo = c.lo;
                hi = c.hi;
                saw = c.saw;
                rate = c.rate;
                ipwm.clear();
                for (const auto& sp : spans) ipwm.push_back(slopeAt(sp.at));
                auto t = simulate(kp, ipw, ipwm);
                if (t.size() < bestN) { bestN = t.size(); use = t; keep = kp; chosen = c; }
            }
        lo = chosen.lo;
        hi = chosen.hi;
        saw = chosen.saw;
        rate = chosen.rate;
        ipwm.clear();
        for (const auto& sp : spans) ipwm.push_back(slopeAt(sp.at));
        char xb[96];
        std::string limits;
        if (hi) { snprintf(xb, sizeof xb, " pwlo=%x pwhi=%x%s", lo << 4, hi << 4, saw ? " pwsaw" : ""); limits = xb; }
        for (const auto& [k, val, sw] : use) {
            snprintf(xb, sizeof xb, "pw:%x,%d ", val, sw);
            R[k].tokens += xb;
        }
        if (keep) name([](size_t) { return std::string("000"); }, [&](size_t) { return " pwrun" + limits; });
        else
            name([&](size_t i) { char w[8]; snprintf(w, sizeof w, "%03x", ipw[i]); return std::string(w); },
                 [&](size_t i) { return (ipwm[i] ? " pwm=" + std::to_string(ipwm[i]) : std::string()) + (hi ? limits : std::string(" free")); });
    };
    char b[512];
    for (int v = 0; v < nv; v++) {
        const auto& F = fr[(size_t)v];
        auto& R = rows[(size_t)v];
        for (size_t k = 0; k < frames; k++) R[k].gate = F[k].gate;
        std::vector<size_t> starts;
        for (size_t k = 0; k < frames; k++) if (F[k].onset || (k == 0 && F[k].gate)) starts.push_back(k);
        std::vector<Span> spans;
        for (size_t si = 0; si < starts.size(); si++) {
            size_t k = starts[si], end = si + 1 < starts.size() ? starts[si + 1] : frames;
            size_t g = k;
            while (g < end && F[g].gate) g++;
            // sounding until the test bit, or the envelope zeroed with the gate off (a hard restart)
            size_t a = k + 1;
            while (a < end && !(F[a].ctrl & 8) && (F[a].gate || F[a].ad || F[a].sr)) a++;
            audible[(size_t)v].push_back({k, a});
            // the first frames: a waveform of their own (a table)
            uint8_t steady = F[std::min(a - 1, k + 8)].ctrl & 0xF6;
            size_t T = 0;
            for (size_t i = 0; i < std::min<size_t>(8, a - k); i++)
                if ((F[k + i].ctrl & 0xF6) != steady) T = i + 1;
            // the pitch after them
            std::vector<double> p;
            for (size_t i = k + T; i < a; i++) {
                double pp = pitch(F[i].freq);
                p.push_back(pp < 0 ? (p.empty() ? 60 : p.back()) : pp);
            }
            if (p.empty()) p.push_back(std::max(0.0, pitch(F[k].freq)));
            std::vector<int> rd;
            for (double x : p) rd.push_back((int)std::lround(x));
            // after the gate goes off: does the pitch or the waveform still change? (tail)
            size_t gi = g > k + T ? g - k - T : 0;   // (the gate's end in p)
            bool after = false;
            for (size_t i = std::max<size_t>(gi, 1); i < rd.size(); i++) after |= rd[i] != rd[i - 1];
            for (size_t i = std::max(g, k + 1); i < a; i++) after |= (F[i].ctrl & 0xF6) != (F[i - 1].ctrl & 0xF6);
            // an arpeggio: two or three notes in turn, most frames changing (in the
            // gate's frames when there are enough of them)
            size_t n = gi >= 4 ? gi : rd.size();
            std::set<int> distinct(rd.begin(), rd.begin() + (long)n);
            int changes = 0;
            for (size_t i = 1; i < n; i++) changes += rd[i] != rd[i - 1];
            int base;
            std::string arp;
            int vib = 0, vdel = 0, vspeed = 1;
            bool narrow = !distinct.empty() && *distinct.rbegin() - *distinct.begin() <= 24;   // (tune45's arpeggios)
            if (narrow && distinct.size() >= 2 && distinct.size() <= 3 && n >= 4 && changes * 10 >= (int)(n - 1) * 6) {
                base = *distinct.begin();   // (its lowest the note)
                std::vector<int> up;
                for (int d : distinct) if (d != base) up.push_back(d - base);
                arp = up.size() == 1 ? "arp2:" + std::to_string(up[0]) : "arp:" + std::to_string(up[0]) + "," + std::to_string(up[1]);
            } else {
                // the pitch's median over a vibrato's time (8 frames): legato notes
                // where it moves on (a jump stays a jump); what is left a vibrato
                size_t w = std::max<size_t>(3, (size_t)std::lround(8 * cpf));
                std::vector<double> m(p.size());
                for (size_t i = 0; i < p.size(); i++) {
                    size_t lo = i >= w / 2 ? i - w / 2 : 0, hi = std::min(p.size(), i + w / 2 + 1);
                    std::vector<double> win(p.begin() + (long)lo, p.begin() + (long)hi);
                    std::nth_element(win.begin(), win.begin() + (long)win.size() / 2, win.end());
                    m[i] = win[win.size() / 2];
                }
                base = (int)std::lround(m[0]);
                int cur = base;
                std::vector<int> at(m.size(), base);   // the note sounding at each frame
                size_t last = 0;   // (the last change: the pitch's jump, not where the median turned)
                for (size_t i = 1; i < m.size(); i++) {
                    if (std::fabs(m[i] - cur) > 0.6) {
                        cur = (int)std::lround(m[i]);
                        size_t j = i;
                        if (std::lround(p[j]) != cur) { while (j + 1 < p.size() && std::lround(p[j]) != cur) j++; }
                        else { while (j > last + 1 && std::lround(p[j - 1]) == cur) j--; }
                        for (size_t q = j; q < i; q++) at[q] = cur;
                        for (size_t q = i; q < j; q++) at[q] = at[i - 1];
                        R[k + T + j].legato = std::max(0, std::min(127, cur));
                        last = j;
                        if (j > i) {   // (on to it: the frames between keep the old)
                            for (size_t q = i; q < j; q++) at[q] = at[i - 1];
                            i = j;
                        }
                    }
                    at[i] = cur;
                }
                // what is left: a vibrato when it swings round its mean (the
                // swings' time its speed, their size its depth)
                std::vector<double> dev(p.size());
                double mean = 0;
                for (size_t i = 0; i < p.size(); i++) mean += (dev[i] = p[i] - at[i]);
                mean /= p.size();
                for (double& d : dev) d -= mean;
                std::vector<double> sorted;
                for (double d : dev) sorted.push_back(std::fabs(d));
                std::sort(sorted.begin(), sorted.end());
                double amp = sorted[sorted.size() * 9 / 10];
                std::vector<size_t> cross;   // where it crosses its mean (with a margin)
                int side = 0;
                for (size_t i = 0; i < dev.size(); i++) {
                    int sd = dev[i] > amp / 3 ? 1 : dev[i] < -amp / 3 ? -1 : 0;
                    if (sd && sd != side) { if (side) cross.push_back(i); side = sd; }
                }
                if (amp >= 0.15 && cross.size() >= 3) {
                    double period = 2.0 * (double)(cross.back() - cross.front()) / (double)(cross.size() - 1) / cpf;   // frames
                    vib = std::max(1, std::min(15, (int)std::lround(256 * (std::pow(2.0, amp / 12) - 1))));
                    vspeed = std::max(1, std::min(16, (int)std::lround(period / 8)));
                    vdel = (int)std::min(255.0, (T + cross.front()) / cpf / 2);
                }
            }
            base = std::max(0, std::min(127, base));
            // the instrument: its registers at the start, its pulse width's and
            // cutoff's lines then, its wave table, vibrato; tail when it moves or
            // changes after the gate goes off
            // (the pulse width's part, @PW@ and @PX@, filled in below: the voice's way)
            const Frame& on = F[k];
            snprintf(b, sizeof b, "%s %02x %02x @PW@ %d 0", waveName(steady).c_str(), on.ad, on.sr, vib);
            std::string l = b;
            if (vdel) l += " vdelay=" + std::to_string(vdel);
            if (vib && vspeed > 1) l += " vspeed=" + std::to_string(vspeed);
            if (on.filtered && on.mode) {
                std::string md;
                static const char* modes[3] = {"lp", "bp", "hp"};
                for (int q = 0; q < 3; q++) if (on.mode & (0x10 << q)) md += (md.empty() ? "" : "+") + std::string(modes[q]);
                Line cl = lineAt(cutLines[(size_t)v / 3], k);
                int cut = std::max(0, std::min(0x7FF, (int)std::lround(cl.value + cl.slope * (double)(k - cl.at)))) & 0x7F0;
                snprintf(b, sizeof b, " filter=%s res=%d cutoff=%x", md.c_str(), on.res, cut);
                l += b;
                if (perFrame(cl.slope)) l += " fsweep=" + std::to_string(perFrame(cl.slope));
            }
            if (T) {
                std::string tb;
                for (size_t i = 0; i < T; i++) {
                    double pp = pitch(F[k + i].freq);
                    int st = pp < 0 ? 0 : std::max(-127, std::min(127, (int)std::lround(pp) - base));
                    tb += " " + waveName(F[k + i].ctrl & 0xF6) + (st ? ":" + std::to_string(st) : "");
                }
                tb += " " + waveName(steady);   // (the last held)
                auto it = tableOf.find(tb);
                if (it == tableOf.end()) {
                    snprintf(b, sizeof b, "t%02zu", tableOf.size() + 1);
                    it = tableOf.emplace(tb, b).first;
                    tableLines.push_back("wavetable " + it->second + tb);
                }
                l += " table=" + it->second;
            }
            // hard restart: the frames before the next note with AD and SR 0 (as the
            // player did it: else the SID's envelope may not start a short note)
            int hr = 0;
            if (end < frames)
                while (hr < 3 && end - 1 - hr > k && !F[end - 1 - hr].gate && !F[end - 1 - hr].ad && !F[end - 1 - hr].sr) hr++;
            if (hr) l += " hr=" + std::to_string(hr);
            bool tail = a > g && after;
            if (tail) l += " tail";
            R[k].start = true;
            R[k].inst = l + "@PX@";
            R[k].note = base;
            R[k].arp = arp.empty() ? "arp:off" : arp;
            spans.push_back({k, g, a, tail});
        }
        pulse(v, spans);
    }
    // cut: where a chip's cutoff line starts (inside a note: at its start the instrument has it)
    for (int c = 0; c < chips; c++) {
        bool routed = false;   // (a chip whose filter a voice uses)
        for (int v = c * 3; v < c * 3 + 3; v++) for (const auto& f : fr[(size_t)v]) routed |= f.filtered && f.mode;
        if (!routed) continue;
        bool atLoop = false;
        for (const auto& ln : cutLines[(size_t)c])
            if (ln.at > 0 && ln.at < frames) {
                snprintf(b, sizeof b, "cut:%x,%d ", std::max(0, std::min(0x7FF, (int)std::lround(ln.value))), perFrame(ln.slope));
                rows[(size_t)c * 3][ln.at].tokens += b;
                atLoop |= ln.at == lp[(size_t)c * 3];
            }
        size_t at = lp[(size_t)c * 3];
        if (period && !atLoop) {   // (its loop point: the cutoff as it was there)
            Line ln = lineAt(cutLines[(size_t)c], at);
            int x = std::max(0, std::min(0x7FF, (int)std::lround(ln.value + ln.slope * (double)(at - ln.at))));
            snprintf(b, sizeof b, "cut:%x,%d ", x, perFrame(ln.slope));
            rows[(size_t)c * 3][at].tokens = b + rows[(size_t)c * 3][at].tokens;
        }
    }
    int last = -1;
    for (int v = 0; v < nv; v++) if (!audible[(size_t)v].empty()) last = v;
    if (last < 0) { error = "the tune played no notes in " + std::to_string((int)(frames / run.callsPerSecond)) + " s"; return ""; }
    last = std::max(last, (chips - 1) * 3);   // (a chip's cut: tokens on its first voice)

    // the text
    std::string t;
    std::string name = ident(!o.name.empty() ? o.name : !p.name.empty() ? p.name : o.source.substr(0, o.source.find_last_of('.')));
    std::string who = p.name.empty() ? "(untitled)" : p.name;
    if (!p.author.empty()) who += " by " + p.author;
    if (!p.released.empty()) who += " (" + p.released + ")";
    if (period)
        snprintf(b, sizeof b, "; %s: tune45 text of %s, song %d of %d: it loops from %.1f s, every %.1f s\n", who.c_str(),
                 o.source.c_str(), song, p.songs, loopFrom / run.callsPerSecond, period / run.callsPerSecond);
    else if (ends)
        snprintf(b, sizeof b, "; %s: tune45 text of %s, song %d of %d: it ends at %.1f s\n", who.c_str(), o.source.c_str(), song,
                 p.songs, loopFrom / run.callsPerSecond);
    else
        snprintf(b, sizeof b, "; %s: tune45 text of %s, song %d of %d, %g s%s\n", who.c_str(), o.source.c_str(), song, p.songs,
                 o.seconds, o.findLoop ? " (no loop found)" : "");
    t += b;
    snprintf(b, sizeof b, "; (tune45 import: a %s, its player run %s, %.2f calls a second, a tick each; %d SID%s)\n", p.magic.c_str(),
             run.cia ? "by its CIA timer" : "a frame", run.callsPerSecond, chips, chips > 1 ? "s" : "");
    t += b;
    if (volumeWrites > 4)
        t += "; Not carried over: " + std::to_string(volumeWrites) + " changes of the volume ($D418: fades, or samples played through it)\n";
    t += "\n";
    for (const auto& l : tableLines) t += l + "\n";
    for (const auto& l : instLines) t += l + "\n";
    // a tick's frames: as near as 8.8 holds them; where that drifts, the two
    // nearest in turn (tempo:), kept within a quarter of a frame of the tune's
    double want = 50.0 / run.callsPerSecond;
    int lo = std::max(26, (int)std::floor(want * 256)), hi = std::max(26, (int)std::ceil(want * 256));
    int now = (int)std::lround(want * 256);
    if (lo != hi) {
        double ideal = 0, played = 0;
        for (size_t k = 0; k < frames; k++) {
            int use = played - ideal > 0.25 ? lo : ideal - played > 0.25 ? hi : now;
            if (use != now) {
                snprintf(b, sizeof b, "tempo:%.8g ", use / 256.0);
                rows[0][k].tokens = b + rows[0][k].tokens;
                now = use;
            }
            ideal += want;
            played += now / 256.0;
        }
    }
    snprintf(b, sizeof b, "\ntune %s %.8g %s\n", name.c_str(), std::lround(want * 256) / 256.0, period ? "loop" : "once");
    std::string tuneLine = b;
    // each voice's tokens (a note's or a legato note's: its note beside it)
    struct Tok { std::string text; int note = -1; };
    std::vector<std::vector<Tok>> tv((size_t)last + 1);
    for (int v = 0; v <= last; v++) {
        const auto& R = rows[(size_t)v];
        auto& out = tv[(size_t)v];
        std::string cur, curArp = "arp:off";
        bool sounding = false;   // (a note begun: what follows its gate rings on)
        size_t end = endAt[(size_t)v], mark = lp[(size_t)v];
        std::set<std::string> insts, arps;   // (named again after the loop point where the voice changes them)
        for (size_t k = 0; k < end; k++) if (R[k].start) { insts.insert(R[k].inst); arps.insert(R[k].arp); }
        for (size_t k = 0; k < end;) {
            if (k == mark) {   // the loop point: the instrument and arpeggio named again after it
                out.push_back({"|:"});
                if (insts.size() > 1) cur.clear();
                if (arps.size() > 1 || (arps.size() == 1 && *arps.begin() != "arp:off")) curArp.clear();
            }
            size_t e = k + 1;
            while (e < end && e != mark && e - k < 4000 && !R[e].start && R[e].legato < 0 && R[e].tokens.empty() && R[e].gate == R[k].gate) e++;
            std::string n = std::to_string(e - k);   // (at most 4000 ticks: a long one goes on as ties or rests)
            if (!R[k].tokens.empty()) {
                std::string tk = R[k].tokens;
                tk.pop_back();
                out.push_back({tk});
            }
            if (R[k].start) {
                if (R[k].arp != curArp) { out.push_back({R[k].arp}); curArp = R[k].arp; }
                if (R[k].inst != cur) { out.push_back({"@" + R[k].inst}); cur = R[k].inst; }
                out.push_back({"N:" + n, R[k].note});
                sounding = true;
            } else if (R[k].legato >= 0 && sounding) out.push_back({"~N:" + n, R[k].legato});
            else if (R[k].gate && sounding) out.push_back({"~:" + n});
            else out.push_back({"r:" + n});
            k = e;
        }
    }
    auto text = [](const Tok& t, int tr) {   // (N: the note, transposed)
        if (t.note < 0) return t.text;
        std::string s = t.text;
        return s.replace(s.find('N'), 1, TuneCompiler::noteName(t.note + tr));
    };
    // the repeats: patterns, a copy also called transposed (*p3+5: its notes
    // so many semitones up). Each time the run of tokens that saves the most,
    // all its copies that do not overlap called, again while one saves
    // anything; a call's text is the text it stands for, so the tune plays as
    // before. A run is found by its notes' intervals (a suffix array of the
    // voices one after another; its first note's own interval not compared:
    // the copies' first notes give their transpositions)
    struct El { int tok = -1; int call = -1, tr = 0; };   // a token (in tv's), or a call
    std::vector<std::vector<El>> seq((size_t)last + 1);
    std::vector<std::vector<Tok>> pats;
    std::vector<std::pair<int, int>> tokAt;   // (voice, place) of each token as tv has it
    for (int v = 0; v <= last; v++)
        for (size_t i = 0; i < tv[(size_t)v].size(); i++) seq[(size_t)v].push_back({(int)i});
    auto tokOf = [&](int v, const El& e) -> const Tok& { return tv[(size_t)v][(size_t)e.tok]; };
    // a token's bytes in the data (a call: 4, transposed 5; a pattern's end 1)
    auto bytesOf = [](const std::string& text) {
        long n = 0;
        size_t at = 0;
        while (at < text.size()) {
            size_t e = text.find(' ', at);
            std::string w = text.substr(at, e == std::string::npos ? std::string::npos : e - at);
            n += w.rfind("pw:", 0) == 0 || w.rfind("cut:", 0) == 0 ? 4
               : w.rfind("~N", 0) == 0 || w.rfind("arp", 0) == 0 || w.rfind("tempo:", 0) == 0 ? 3 : 2;
            if (e == std::string::npos) break;
            at = e + 1;
        }
        return n;
    };
    // (the search twice: transposed copies too, and exact copies alone; the
    // smaller kept: taking a transposed copy early can keep a longer exact
    // one from being found)
    auto search = [&](bool transpose, std::vector<std::vector<El>>& seq, std::vector<std::vector<Tok>>& pats) {
        std::map<std::string, int> tmplId;
        for (int round = 0; round < 400; round++) {
            // the voices one after another: a token's template and its note's
            // interval from the voice's last (a call, a voice's end: unique)
            struct Sym { long long tmpl; int delta; bool note; };   // (delta 1000: none before)
            std::vector<Sym> all;
            std::vector<std::pair<int, size_t>> from;
            long long unique = -1;
            for (int v = 0; v <= last; v++) {
                int prev = -1;
                for (size_t i = 0; i < seq[(size_t)v].size(); i++) {
                    const El& e = seq[(size_t)v][i];
                    if (e.call >= 0 || tokOf(v, e).text == "|:") { all.push_back({unique--, 0, false}); prev = -1; }   // (a call, the loop point)
                    else {
                        const Tok& t = tokOf(v, e);
                        long long id = tmplId.emplace(t.text, (int)tmplId.size()).first->second;
                        if (t.note >= 0 && !transpose) all.push_back({id, t.note, false});   // (exact: the note itself)
                        else if (t.note >= 0) { all.push_back({id, prev < 0 ? 1000 : t.note - prev, true}); prev = t.note; }
                        else all.push_back({id, 0, false});
                    }
                    from.push_back({v, i});
                }
                all.push_back({unique--, 0, false});
                from.push_back({v, seq[(size_t)v].size()});
            }
            // the tokens two suffixes share (their first notes' intervals not compared)
            auto common = [&](size_t a, size_t b, size_t limit) {
                size_t n = 0;
                bool first = true;
                while (a + n < all.size() && b + n < all.size() && n < limit) {
                    const Sym &x = all[a + n], &y = all[b + n];
                    if (x.tmpl < 0 || x.tmpl != y.tmpl) break;
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
                if (x.tmpl != y.tmpl) return x.tmpl < y.tmpl;
                return x.delta < y.delta;
            });
            std::vector<size_t> lcp(sa.size(), 0);
            for (size_t k = 1; k < sa.size(); k++) lcp[k] = common(sa[k - 1], sa[k], 512);
            // a candidate: the run at a, len tokens; its copies (place, transposition)
            auto copies = [&](size_t a, size_t len, const std::vector<size_t>& at) {
                std::vector<std::pair<size_t, int>> out;
                int v0 = from[a].first;
                int n0 = -1;
                for (size_t q = 0; q < len; q++) {
                    const El& e = seq[(size_t)v0][from[a + q].second];
                    if (tokOf(v0, e).note >= 0) { n0 = tokOf(v0, e).note; break; }
                }
                size_t end = 0;
                for (size_t p : at) {
                    if (!out.empty() && p < end) continue;   // (overlapping)
                    int v = from[p].first, tr = 0;
                    if (n0 >= 0)
                        for (size_t q = 0; q < len; q++) {
                            const El& e = seq[(size_t)v][from[p + q].second];
                            if (tokOf(v, e).note >= 0) { tr = tokOf(v, e).note - n0; break; }
                        }
                    if (tr < -96 || tr > 96) continue;
                    out.push_back({p, tr});
                    end = p + len;
                }
                return out;
            };
            long bestGain = 0;
            size_t bestAt = 0, bestLen = 0;
            std::vector<size_t> bestPos;
            std::set<std::pair<size_t, size_t>> tried;
            for (size_t k = 1; k < sa.size(); k++) {
                size_t len = lcp[k];
                if (len < 4) continue;
                size_t lo = k - 1, hi = k;
                while (lo > 0 && lcp[lo] >= len) lo--;
                while (hi + 1 < sa.size() && lcp[hi + 1] >= len) hi++;
                if (!tried.insert({len, lo}).second) continue;
                std::vector<size_t> pos(sa.begin() + (long)lo, sa.begin() + (long)hi + 1);
                std::sort(pos.begin(), pos.end());
                auto cs = copies(pos[0], len, pos);
                long body = 0;
                int vb = from[pos[0]].first;
                for (size_t q = 0; q < len; q++) body += bytesOf(tokOf(vb, seq[(size_t)vb][from[pos[0] + q].second]).text);
                long gain = (long)(cs.size() - 1) * body - 1;
                for (const auto& c : cs) gain -= c.second ? 5 : 4;
                if (gain > bestGain) { bestGain = gain; bestAt = pos[0]; bestLen = len; bestPos = pos; }
            }
            if (!bestLen) break;
            // the pattern: the first copy's tokens; each copy a call
            int pid = (int)pats.size();
            int v0 = from[bestAt].first;
            std::vector<Tok> body;
            for (size_t q = 0; q < bestLen; q++) body.push_back(tokOf(v0, seq[(size_t)v0][from[bestAt + q].second]));
            pats.push_back(body);
            auto cs = copies(bestAt, bestLen, bestPos);
            std::map<int, std::map<size_t, int>> callAt;   // voice: place: transposition
            for (const auto& [p, tr] : cs) callAt[from[p].first][from[p].second] = tr;
            for (auto& [v, places] : callAt) {
                std::vector<El> y;
                auto& x = seq[(size_t)v];
                for (size_t i = 0; i < x.size();) {
                    auto it = places.find(i);
                    if (it != places.end()) { y.push_back({-1, pid, it->second}); i += bestLen; }
                    else y.push_back(x[i++]);
                }
                x = y;
            }
        }
    };
    auto estimate = [&](const std::vector<std::vector<El>>& seq, const std::vector<std::vector<Tok>>& pats) {
        long n = 0;
        for (int v = 0; v <= last; v++)
            for (const El& e : seq[(size_t)v]) n += e.call >= 0 ? (e.tr ? 5 : 4) : bytesOf(tokOf(v, e).text);
        for (const auto& b : pats) { n++; for (const auto& tk : b) n += bytesOf(tk.text); }
        return n;
    };
    if (o.patterns) {
        auto seqT = seq, seqE = seq;
        std::vector<std::vector<Tok>> patsT, patsE;
        search(true, seqT, patsT);
        search(false, seqE, patsE);
        bool t = estimate(seqT, patsT) <= estimate(seqE, patsE);
        seq = t ? seqT : seqE;
        pats = t ? patsT : patsE;
    }
    // the voices' text: calls (one again at once, as transposed: *p*n)
    std::vector<std::vector<std::string>> vt((size_t)last + 1);
    for (int v = 0; v <= last; v++) {
        const auto& x = seq[(size_t)v];
        for (size_t i = 0; i < x.size();) {
            if (x[i].call < 0) { vt[(size_t)v].push_back(text(tokOf(v, x[i]), 0)); i++; continue; }
            size_t n = 1;
            while (i + n < x.size() && x[i + n].call == x[i].call && x[i + n].tr == x[i].tr && n < 255) n++;
            std::string c = "*p" + std::to_string(x[i].call + 1);
            if (x[i].tr) c += (x[i].tr > 0 ? "+" : "") + std::to_string(x[i].tr);
            vt[(size_t)v].push_back(c + (n > 1 ? "*" + std::to_string(n) : ""));
            i += n;
        }
    }
    auto lines = [](const std::string& head, const std::vector<std::string>& toks) {
        std::string line = head;
        for (size_t k = 0; k < toks.size(); k++) line += (k && k % 16 == 0 ? " \\\n    " : " ") + toks[k];
        return line + "\n";
    };
    if (!pats.empty()) t += "\n; the tune's repeats\n";
    for (size_t k = 0; k < pats.size(); k++) {
        std::vector<std::string> body;
        for (const auto& tk : pats[k]) body.push_back(text(tk, 0));
        t += lines("pattern p" + std::to_string(k + 1) + ":", body);
    }
    t += tuneLine;
    for (int v = 0; v <= last; v++) t += lines("v" + std::to_string(v + 1) + ":", vt[(size_t)v]);
    return t;
}
