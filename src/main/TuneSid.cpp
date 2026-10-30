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

    // the notes, their instruments and wave tables
    std::map<std::string, std::string> instOf;     // its line's text (without the name): the name
    std::vector<std::string> instLines, tableLines;
    std::map<std::string, std::string> tableOf;
    std::vector<std::vector<Note>> notes((size_t)nv);
    for (int v = 0; v < nv; v++) {
        const auto& F = fr[(size_t)v];
        for (size_t k = 0; k < frames;) {
            if (!(F[k].onset || (k == 0 && F[k].gate))) { k++; continue; }
            Note n;
            n.at = k;
            size_t e = k + 1;
            while (e < frames && F[e].gate && !F[e].onset) e++;
            n.len = e - k;
            // the first frames: a waveform or a pitch of their own (a table)
            uint8_t steady = F[std::min(e - 1, k + 8)].ctrl & 0xFE;
            size_t T = 0;
            for (size_t i = 0; i < std::min<size_t>(8, n.len); i++)
                if ((F[k + i].ctrl & 0xFE) != steady) T = i + 1;
            double basePitch = pitch(F[std::min(e - 1, k + T)].freq);
            // the pitch after them: an arpeggio, or legato notes
            std::vector<int> semi;
            for (size_t i = T; i < n.len; i++) {
                double pp = pitch(F[k + i].freq);
                semi.push_back(pp < 0 ? 0 : (int)std::lround(pp - basePitch));
            }
            int base = basePitch < 0 ? 60 : (int)std::lround(basePitch);
            std::set<int> distinct(semi.begin(), semi.end());
            int changes = 0;
            for (size_t i = 1; i < semi.size(); i++) changes += semi[i] != semi[i - 1];
            if (distinct.size() >= 2 && distinct.size() <= 3 && semi.size() >= 4 && changes * 10 >= (int)(semi.size() - 1) * 6) {
                int lo = *distinct.begin();   // (the arpeggio's root: its lowest)
                std::vector<int> up;
                for (int d : distinct) if (d != lo) up.push_back(d - lo);
                base += lo;
                n.arpA = up[0];
                n.arpB = up.size() > 1 ? up[1] : up[0];
            } else {
                // legato: a pitch held two frames or more where it differs
                int cur = 0;
                size_t i = 0;
                while (i < semi.size()) {
                    size_t j = i;
                    while (j < semi.size() && semi[j] == semi[i]) j++;
                    if (semi[i] != cur && j - i >= 2 && i > 0) {
                        n.legato.push_back({base + semi[i], T + i});   // (its frame from the note's start; lengths below)
                        cur = semi[i];
                    }
                    i = j;
                }
            }
            n.base = std::max(0, std::min(127, base));
            for (auto& l : n.legato) l.first = std::max(0, std::min(127, l.first));
            // the instrument
            const Frame& on = F[k];
            char b[200];
            snprintf(b, sizeof b, "%s %02x %02x %03x 0 0", waveName(steady).c_str(), on.ad, on.sr, on.pw & 0xFF0);
            std::string l = b;
            if (n.len > 2) {
                int d = (int)F[k + 1].pw - (int)on.pw;
                if (d && std::abs(d) < 128 && (int)F[k + 2].pw - (int)F[k + 1].pw == d) l += " pwm=" + std::to_string(d);
            }
            if (on.filtered && on.mode) {
                std::string m;
                static const char* modes[3] = {"lp", "bp", "hp"};
                for (int q = 0; q < 3; q++) if (on.mode & (0x10 << q)) m += (m.empty() ? "" : "+") + std::string(modes[q]);
                snprintf(b, sizeof b, " filter=%s res=%d cutoff=%x", m.c_str(), on.res, on.cutoff);
                l += b;
                if (n.len > 1) {
                    int d = (int)F[k + 1].cutoff - (int)on.cutoff;
                    if (d && std::abs(d) < 128) l += " fsweep=" + std::to_string(d);
                }
            }
            if (T) {
                std::string tb;
                for (size_t i = 0; i < T; i++) {
                    double pp = pitch(F[k + i].freq);
                    int s = pp < 0 ? 0 : std::max(-127, std::min(127, (int)std::lround(pp) - n.base));
                    tb += " " + waveName(F[k + i].ctrl & 0xFE) + (s ? ":" + std::to_string(s) : "");
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
            auto it = instOf.find(l);
            if (it == instOf.end()) {
                snprintf(b, sizeof b, "i%02zu", instOf.size() + 1);
                it = instOf.emplace(l, b).first;
                instLines.push_back("instrument " + it->second + " " + l);
            }
            n.inst = it->second;
            notes[(size_t)v].push_back(n);
            k = e;
        }
    }
    int last = -1;
    for (int v = 0; v < nv; v++) if (!notes[(size_t)v].empty()) last = v;
    if (last < 0) { error = "the tune played no notes in " + std::to_string((int)o.seconds) + " s"; return ""; }

    // the text
    std::string t;
    char b[512];
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
    snprintf(b, sizeof b, "\ntune %s %.4g once\n", name.c_str(), 50.0 / run.callsPerSecond);
    t += b;
    for (int v = 0; v <= last; v++) {
        std::string line = "v" + std::to_string(v + 1) + ":";
        std::string cur;
        int arpA = 0, arpB = 0;
        size_t at = 0;
        int count = 0;
        auto add = [&](const std::string& tok) {
            line += " " + tok;
            if (++count % 16 == 0) line += " \\\n   ";
        };
        for (const auto& n : notes[(size_t)v]) {
            if (n.at > at) add("r:" + std::to_string(n.at - at));
            if (n.arpA != arpA || n.arpB != arpB) {
                add(n.arpA ? "arp:" + std::to_string(n.arpA) + "," + std::to_string(n.arpB) : std::string("arp:off"));
                arpA = n.arpA;
                arpB = n.arpB;
            }
            if (n.inst != cur) { add("@" + n.inst); cur = n.inst; }
            size_t first = n.legato.empty() ? n.len : n.legato[0].second;
            add(TuneCompiler::noteName(n.base) + ":" + std::to_string(first));
            for (size_t k = 0; k < n.legato.size(); k++) {
                size_t end = k + 1 < n.legato.size() ? n.legato[k + 1].second : n.len;
                add("~" + TuneCompiler::noteName(n.legato[k].first) + ":" + std::to_string(end - n.legato[k].second));
            }
            at = n.at + n.len;
        }
        if (at < frames) add("r:" + std::to_string(frames - at));
        while (line.size() > 4 && line.substr(line.size() - 5) == " \\\n   ") line.erase(line.size() - 5);
        t += line + "\n";
    }
    return t;
}
