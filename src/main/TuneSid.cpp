// TuneSid.cpp — tune45 import of a .sid (TuneSid.hpp)
#include "TuneSid.hpp"
#include "TuneCompiler.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
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
    int calls = std::max(1, (int)std::lround(o.seconds * run.callsPerSecond));
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
    const size_t frames = run.calls.size();
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
    char b[512];
    for (int v = 0; v < nv; v++) {
        const auto& F = fr[(size_t)v];
        auto& R = rows[(size_t)v];
        for (size_t k = 0; k < frames; k++) R[k].gate = F[k].gate;
        std::vector<size_t> starts;
        for (size_t k = 0; k < frames; k++) if (F[k].onset || (k == 0 && F[k].gate)) starts.push_back(k);
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
            const Frame& on = F[k];
            Line pl = lineAt(pwLines[(size_t)v], k);
            int pw = (int)std::lround(pl.value + pl.slope * (double)(k - pl.at));
            pw = std::max(0, std::min(0xFFF, pw));
            pw &= pw < 0x100 || pw > 0xEFF ? 0xFFF : 0xFC0;   // (finer where it is thin)
            snprintf(b, sizeof b, "%s %02x %02x %03x %d 0", waveName(steady).c_str(), on.ad, on.sr, pw, vib);
            std::string l = b;
            if (perFrame(pl.slope)) l += " pwm=" + std::to_string(perFrame(pl.slope)) + " free";
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
            if (a > g && after) l += " tail";
            auto it = instOf.find(l);
            if (it == instOf.end()) {
                snprintf(b, sizeof b, "i%02zu", instOf.size() + 1);
                it = instOf.emplace(l, b).first;
                instLines.push_back("instrument " + it->second + " " + l);
            }
            R[k].start = true;
            R[k].inst = it->second;
            R[k].note = base;
            R[k].arp = arp.empty() ? "arp:off" : arp;
        }
    }
    // pw: and cut: where a line starts inside a note (at its start the instrument has it)
    auto inside = [&](int v, size_t k) {
        for (const auto& [s0, s1] : audible[(size_t)v]) if (k > s0 && k < s1) return true;
        return false;
    };
    for (int v = 0; v < nv; v++)
        for (const auto& ln : pwLines[(size_t)v])
            if (ln.at < frames && inside(v, ln.at)) {
                snprintf(b, sizeof b, "pw:%x,%d ", (int)std::lround(ln.value) & 0xFFF, perFrame(ln.slope));
                rows[(size_t)v][ln.at].tokens += b;
            }
    for (int c = 0; c < chips; c++) {
        bool routed = false;   // (a chip whose filter a voice uses)
        for (int v = c * 3; v < c * 3 + 3; v++) for (const auto& f : fr[(size_t)v]) routed |= f.filtered && f.mode;
        if (!routed) continue;
        for (const auto& ln : cutLines[(size_t)c])
            if (ln.at > 0 && ln.at < frames) {
                snprintf(b, sizeof b, "cut:%x,%d ", std::max(0, std::min(0x7FF, (int)std::lround(ln.value))), perFrame(ln.slope));
                rows[(size_t)c * 3][ln.at].tokens += b;
            }
    }
    int last = -1;
    for (int v = 0; v < nv; v++) if (!audible[(size_t)v].empty()) last = v;
    if (last < 0) { error = "the tune played no notes in " + std::to_string((int)o.seconds) + " s"; return ""; }
    last = std::max(last, (chips - 1) * 3);   // (a chip's cut: tokens on its first voice)

    // the text
    std::string t;
    std::string name = ident(!o.name.empty() ? o.name : !p.name.empty() ? p.name : o.source.substr(0, o.source.find_last_of('.')));
    std::string who = p.name.empty() ? "(untitled)" : p.name;
    if (!p.author.empty()) who += " by " + p.author;
    if (!p.released.empty()) who += " (" + p.released + ")";
    snprintf(b, sizeof b, "; %s: tune45 text of %s, song %d of %d, %g s\n", who.c_str(), o.source.c_str(), song, p.songs, o.seconds);
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
    snprintf(b, sizeof b, "\ntune %s %.8g once\n", name.c_str(), std::lround(want * 256) / 256.0);
    std::string tuneLine = b;
    // each voice's tokens
    std::vector<std::vector<std::string>> vt((size_t)last + 1);
    for (int v = 0; v <= last; v++) {
        const auto& R = rows[(size_t)v];
        auto& out = vt[(size_t)v];
        std::string cur, curArp = "arp:off";
        bool sounding = false;   // (a note begun: what follows its gate rings on)
        for (size_t k = 0; k < frames;) {
            size_t e = k + 1;
            while (e < frames && !R[e].start && R[e].legato < 0 && R[e].tokens.empty() && R[e].gate == R[k].gate) e++;
            std::string n = std::to_string(e - k);
            if (!R[k].tokens.empty()) {
                std::string tk = R[k].tokens;
                tk.pop_back();
                out.push_back(tk);
            }
            if (R[k].start) {
                if (R[k].arp != curArp) { out.push_back(R[k].arp); curArp = R[k].arp; }
                if (R[k].inst != cur) { out.push_back("@" + R[k].inst); cur = R[k].inst; }
                out.push_back(TuneCompiler::noteName(R[k].note) + ":" + n);
                sounding = true;
            } else if (R[k].legato >= 0 && sounding) out.push_back("~" + TuneCompiler::noteName(R[k].legato) + ":" + n);
            else if (R[k].gate && sounding) out.push_back("~:" + n);
            else out.push_back("r:" + n);
            k = e;
        }
    }
    // the repeats: patterns (each the run of tokens that saves the most, all
    // its copies that do not overlap called, again while one saves something;
    // a call's text is the text it stands for, so the tune plays as before)
    std::vector<std::vector<std::string>> pats;
    if (o.patterns) {
        std::map<std::string, int> idOf;
        std::vector<std::vector<int>> ids((size_t)last + 1);   // (a call: -1 - the pattern)
        for (int v = 0; v <= last; v++)
            for (const auto& tk : vt[(size_t)v]) ids[(size_t)v].push_back(idOf.emplace(tk, (int)idOf.size()).first->second);
        std::vector<std::string> nameOf(idOf.size());
        for (const auto& [tk, id] : idOf) nameOf[(size_t)id] = tk;
        for (int round = 0; round < 400; round++) {
            // the best run: (count - 1) * length - (count + 1) (a call, the end:
            // about a token each), among the maximal repeats (a suffix array of
            // the voices one after another, calls and the voices' ends unique)
            long bestGain = 0;
            std::vector<int> best;
            std::vector<int> all;
            std::vector<std::pair<int, size_t>> from;   // each position's voice and place
            int unique = -1000000;
            for (int v = 0; v <= last; v++) {
                for (size_t i = 0; i < ids[(size_t)v].size(); i++) {
                    int x = ids[(size_t)v][i];
                    all.push_back(x < 0 ? unique-- : x);
                    from.push_back({v, i});
                }
                all.push_back(unique--);
                from.push_back({v, ids[(size_t)v].size()});
            }
            std::vector<size_t> sa(all.size());
            for (size_t i = 0; i < sa.size(); i++) sa[i] = i;
            std::sort(sa.begin(), sa.end(), [&](size_t a, size_t b) {
                return std::lexicographical_compare(all.begin() + (long)a, all.end(), all.begin() + (long)b, all.end());
            });
            std::vector<size_t> lcp(sa.size(), 0);   // with the one before
            for (size_t k = 1; k < sa.size(); k++) {
                size_t a = sa[k - 1], b = sa[k], n = 0;
                while (a + n < all.size() && b + n < all.size() && all[a + n] == all[b + n] && all[a + n] >= 0) n++;
                lcp[k] = n;
            }
            std::set<std::pair<size_t, size_t>> tried;   // (length, the group's first)
            for (size_t k = 1; k < sa.size(); k++) {
                size_t len = std::min<size_t>(lcp[k], 512);
                if (len < 4) continue;
                size_t lo = k - 1, hi = k;   // the suffixes sharing len tokens
                while (lo > 0 && lcp[lo] >= len) lo--;
                while (hi + 1 < sa.size() && lcp[hi + 1] >= len) hi++;
                if (!tried.insert({len, lo}).second) continue;
                std::vector<size_t> pos(sa.begin() + (long)lo, sa.begin() + (long)hi + 1);
                std::sort(pos.begin(), pos.end());
                int count = 0;
                size_t end = 0;
                for (size_t q : pos)
                    if (!count || q >= end) { count++; end = q + len; }   // (not overlapping)
                long gain = (long)(count - 1) * (long)len - (count + 1);
                if (gain > bestGain) {
                    bestGain = gain;
                    best.assign(all.begin() + (long)sa[k], all.begin() + (long)(sa[k] + len));
                }
            }
            if (best.empty()) break;
            int pid = (int)pats.size();
            std::vector<std::string> body;
            for (int id : best) body.push_back(nameOf[(size_t)id]);
            pats.push_back(body);
            for (auto& x : ids) {
                std::vector<int> y;
                for (size_t i = 0; i < x.size();) {
                    if (i + best.size() <= x.size() && std::equal(best.begin(), best.end(), x.begin() + (long)i)) {
                        y.push_back(-1 - pid);
                        i += best.size();
                    } else y.push_back(x[i++]);
                }
                x = y;
            }
        }
        // the voices again: calls (one again at once: *p*n)
        for (int v = 0; v <= last; v++) {
            auto& out = vt[(size_t)v];
            out.clear();
            const auto& x = ids[(size_t)v];
            for (size_t i = 0; i < x.size();) {
                if (x[i] >= 0) { out.push_back(nameOf[(size_t)x[i++]]); continue; }
                size_t n = 1;
                while (i + n < x.size() && x[i + n] == x[i] && n < 255) n++;
                out.push_back("*p" + std::to_string(-x[i]) + (n > 1 ? "*" + std::to_string(n) : ""));
                i += n;
            }
        }
    }
    auto lines = [](const std::string& head, const std::vector<std::string>& toks) {
        std::string line = head;
        for (size_t k = 0; k < toks.size(); k++) line += (k && k % 16 == 0 ? " \\\n    " : " ") + toks[k];
        return line + "\n";
    };
    if (!pats.empty()) t += "\n; the tune's repeats\n";
    for (size_t k = 0; k < pats.size(); k++) t += lines("pattern p" + std::to_string(k + 1) + ":", pats[k]);
    t += tuneLine;
    for (int v = 0; v <= last; v++) t += lines("v" + std::to_string(v + 1) + ":", vt[(size_t)v]);
    return t;
}
