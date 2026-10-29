// TuneCompiler.cpp — tune45's music text and the player's data (see TuneCompiler.hpp)
#include "TuneCompiler.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <map>
#include <sstream>

namespace {

std::string lower(std::string s) {
    for (auto& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}

bool parseInt(const std::string& s, long& v, int base = 10) {
    if (s.empty()) return false;
    char* end = nullptr;
    v = std::strtol(s.c_str(), &end, base);
    return end && *end == 0;
}

std::vector<std::string> words(const std::string& line) {
    std::vector<std::string> w;
    std::istringstream in(line);
    std::string t;
    while (in >> t) w.push_back(t);
    return w;
}

}  // namespace

void TuneCompiler::error(const std::string& file, int line, const std::string& msg) {
    errors_.push_back(file + ":" + std::to_string(line) + ": error: " + msg);
}

int TuneCompiler::noteNumber(const std::string& tok) {
    static const int base[7] = {9, 11, 0, 2, 4, 5, 7};   // a b c d e f g
    std::string t = lower(tok);
    if (t.size() < 2 || t[0] < 'a' || t[0] > 'g') return -1;
    int n = base[t[0] - 'a'];
    size_t p = 1;
    if (t[p] == '#') { n++; p++; }
    else if (t[p] == 'b' && p + 1 < t.size() && (std::isdigit((unsigned char)t[p + 1]) || t[p + 1] == '-')) { n--; p++; }
    long oct;
    if (!parseInt(t.substr(p), oct) || oct < -1 || oct > 9) return -1;
    n += 12 * ((int)oct + 1);
    return n >= 0 && n < 128 ? n : -1;
}

std::string TuneCompiler::noteName(int n) {
    static const char* names[12] = {"c", "c#", "d", "d#", "e", "f", "f#", "g", "g#", "a", "a#", "b"};
    return std::string(names[n % 12]) + std::to_string(n / 12 - 1);
}

bool TuneCompiler::parse(const std::string& text, const std::string& file, TuneFile& out) {
    errors_.clear();
    out = TuneFile();
    // lines: comments off, continuations joined (the line where it began)
    std::vector<std::pair<int, std::string>> lines;
    {
        std::istringstream in(text);
        std::string raw, cont;
        int no = 0, first = 0;
        while (std::getline(in, raw)) {
            no++;
            std::string line = raw.substr(0, raw.find(';'));
            while (!line.empty() && std::isspace((unsigned char)line.back())) line.pop_back();
            if (cont.empty()) first = no;
            if (!line.empty() && line.back() == '\\') {
                cont += line.substr(0, line.size() - 1) + " ";
                continue;
            }
            line = cont + line;
            cont.clear();
            if (line.find_first_not_of(" \t\r") != std::string::npos) lines.push_back({first, line});
        }
        if (!cont.empty()) lines.push_back({first, cont});
    }
    std::map<std::string, int> instId, tableId;
    static const std::map<std::string, int> waves = {{"tri", 0x10}, {"saw", 0x20}, {"pulse", 0x40}, {"noise", 0x80}};
    // a waveform: a name, names joined by + (pulse+saw), or a hex control value
    auto waveOf = [&](const std::string& tok, uint8_t& out) {
        long v;
        if (parseInt(tok, v, 16) && v >= 0 && v < 256) { out = (uint8_t)v; return true; }
        int bits = 0;
        std::string t = lower(tok);
        size_t at = 0;
        while (at <= t.size()) {
            size_t plus = t.find('+', at);
            auto wv = waves.find(t.substr(at, plus == std::string::npos ? std::string::npos : plus - at));
            if (wv == waves.end()) return false;
            bits |= wv->second;
            if (plus == std::string::npos) break;
            at = plus + 1;
        }
        out = (uint8_t)bits;
        return true;
    };
    for (const auto& [ln, line] : lines) {
        auto w = words(line);
        std::string kw = lower(w[0]);
        if (kw == "instrument") {
            if (w.size() < 8) { error(file, ln, "instrument <name> <wave> <ad> <sr> <pulse> <vibrato> <gap>"); continue; }
            TuneInstrument ins;
            ins.name = w[1];
            ins.line = ln;
            if (!waveOf(w[2], ins.wave)) { error(file, ln, "not a waveform: " + w[2] + " (tri, saw, pulse, noise, a+b, hex)"); continue; }
            bool ok = true;
            auto hex = [&](const std::string& s, long max, long& r, const char* what) {
                if (!parseInt(s, r, 16) || r < 0 || r > max) { error(file, ln, std::string("not ") + what + ": " + s); ok = false; }
            };
            long ad = 0, sr = 0, pw = 0, vib = 0, gap = 0;
            hex(w[3], 0xFF, ad, "an attack/decay pair (hex)");
            hex(w[4], 0xFF, sr, "a sustain/release pair (hex)");
            hex(w[5], 0xFFF, pw, "a pulse width (hex, 0-fff)");
            if (!parseInt(w[6], vib) || vib < 0 || vib > 15) { error(file, ln, "not a vibrato depth (0-15): " + w[6]); ok = false; }
            if (!parseInt(w[7], gap) || gap < 0 || gap > 255) { error(file, ln, "not a gap (ticks): " + w[7]); ok = false; }
            bool filterSet = false, filterTuned = false;
            long res = 0;
            for (size_t k = 8; k < w.size(); k++) {
                size_t eq = w[k].find('=');
                std::string key = lower(w[k].substr(0, eq));
                if (eq == std::string::npos) { error(file, ln, "expected key=value: " + w[k]); ok = false; continue; }
                std::string val = w[k].substr(eq + 1);
                // the keys with a word for a value
                if (key == "filter") {
                    static const std::map<std::string, int> modes = {{"lp", 0x10}, {"bp", 0x20}, {"hp", 0x40}, {"notch", 0x50}};
                    int bits = 0;
                    std::string t = lower(val);
                    size_t at = 0;
                    bool good = !t.empty();
                    while (good && at <= t.size()) {
                        size_t plus = t.find('+', at);
                        auto m = modes.find(t.substr(at, plus == std::string::npos ? std::string::npos : plus - at));
                        if (m == modes.end()) good = false;
                        else bits |= m->second;
                        if (plus == std::string::npos) break;
                        at = plus + 1;
                    }
                    if (!good) { error(file, ln, "filter=lp, bp, hp, notch or joined by + (lp+bp), not " + val); ok = false; continue; }
                    ins.filt = (uint8_t)bits;
                    filterSet = true;
                    continue;
                }
                if (key == "table") {
                    auto it = tableId.find(val);
                    if (it == tableId.end()) { error(file, ln, "no wave table " + val + " (a wavetable line before the instrument)"); ok = false; continue; }
                    ins.table = it->second;
                    continue;
                }
                if (key == "cutoff") {
                    long c;
                    if (!parseInt(val, c, 16) || c < 0 || c > 0x7FF) { error(file, ln, "cutoff=hex 0-7ff, not " + val); ok = false; continue; }
                    ins.cutoff = (uint16_t)c;
                    filterTuned = true;
                    continue;
                }
                long v2;
                if (!parseInt(val, v2)) { error(file, ln, "expected key=number: " + w[k]); ok = false; continue; }
                if (key == "pwm" && v2 >= -128 && v2 <= 127) ins.pwm = (int8_t)v2;
                else if (key == "vdelay" && v2 >= 0 && v2 <= 255) ins.vdelay = (uint8_t)v2;
                else if (key == "slide" && v2 >= -128 && v2 <= 127) ins.slide = (int8_t)v2;
                else if (key == "hr" && v2 >= 0 && v2 <= 3) ins.hr = (uint8_t)v2;
                else if (key == "res" && v2 >= 0 && v2 <= 15) { res = v2; filterTuned = true; }
                else if (key == "fsweep" && v2 >= -128 && v2 <= 127) { ins.fsweep = (int8_t)v2; filterTuned = true; }
                else {
                    error(file, ln, "unknown or out of range: " + w[k] +
                                    " (pwm, vdelay, slide, hr 0-3, filter, res 0-15, cutoff, fsweep, table)");
                    ok = false;
                }
            }
            if (filterTuned && !filterSet) { error(file, ln, "res, cutoff and fsweep need filter="); ok = false; }
            ins.filt = (uint8_t)(ins.filt | res);
            if (!ok) continue;
            ins.ad = (uint8_t)ad; ins.sr = (uint8_t)sr; ins.pw = (uint16_t)pw;
            ins.vib = (uint8_t)vib; ins.gap = (uint8_t)gap;
            if (instId.count(ins.name)) { error(file, ln, "instrument " + ins.name + " defined twice"); continue; }
            instId[ins.name] = (int)out.instruments.size();
            out.instruments.push_back(ins);
        } else if (kw == "wavetable") {
            if (w.size() < 3) { error(file, ln, "wavetable <name> <waveform[:semitones]> ... [loop <step>]"); continue; }
            TuneTable tb;
            tb.name = w[1];
            tb.line = ln;
            bool ok = true;
            for (size_t k = 2; k < w.size() && ok; k++) {
                if (lower(w[k]) == "loop") {
                    long n;
                    if (k + 2 != w.size() || !parseInt(w[k + 1], n) || n < 1 || n > (long)tb.steps.size()) {
                        error(file, ln, "loop <step> (1-" + std::to_string(tb.steps.size()) + ") ends the line");
                        ok = false;
                    } else tb.loop = (int)n - 1;
                    break;
                }
                size_t colon = w[k].find(':');
                uint8_t wave;
                long semi = 0;
                if (!waveOf(w[k].substr(0, colon), wave) ||
                    (colon != std::string::npos && (!parseInt(w[k].substr(colon + 1), semi) || semi < -96 || semi > 96))) {
                    error(file, ln, "a step is a waveform (tri, saw, pulse, noise, a+b, hex) and :semitones (-96 to 96): " + w[k]);
                    ok = false;
                    break;
                }
                tb.steps.push_back({wave, (int8_t)semi});
            }
            if (!ok) continue;
            if (tb.steps.empty() || tb.steps.size() > 64) { error(file, ln, "a wave table has 1-64 steps"); continue; }
            if (tableId.count(tb.name)) { error(file, ln, "wave table " + tb.name + " defined twice"); continue; }
            tableId[tb.name] = (int)out.tables.size();
            out.tables.push_back(tb);
        } else if (kw == "tune") {
            if (w.size() < 4) { error(file, ln, "tune <name> <frames per tick> loop|once [ticks a bar]"); continue; }
            Tune t;
            t.name = w[1];
            t.line = ln;
            char* end = nullptr;
            double fpt = std::strtod(w[2].c_str(), &end);
            if (!end || *end || fpt < 0.1 || fpt > 255) { error(file, ln, "not frames per tick: " + w[2]); continue; }
            t.fpt = (uint16_t)std::lround(fpt * 256);
            std::string l = lower(w[3]);
            if (l != "loop" && l != "once") { error(file, ln, "loop or once, not " + w[3]); continue; }
            t.loop = l == "loop";
            if (w.size() > 4) {
                long bar;
                if (!parseInt(w[4], bar) || bar < 1) { error(file, ln, "not ticks a bar: " + w[4]); continue; }
                t.barTicks = (unsigned)bar;
            }
            for (const auto& o : out.tunes)
                if (o.name == t.name) error(file, ln, "tune " + t.name + " defined twice");
            out.tunes.push_back(t);
        } else if (kw.size() >= 3 && kw[0] == 'v' && kw.back() == ':') {
            long vn;
            if (!parseInt(kw.substr(1, kw.size() - 2), vn) || vn < 1 || vn > 12) { error(file, ln, "a voice is v1: to v12:, not " + w[0]); continue; }
            if (out.tunes.empty()) { error(file, ln, "a voice before any tune"); continue; }
            Tune& t = out.tunes.back();
            if ((size_t)vn != t.voices.size() + 1) {
                error(file, ln, "tune " + t.name + ": v" + std::to_string(t.voices.size() + 1) + ": expected, not " + w[0]);
                continue;
            }
            TuneVoice v;
            v.line = ln;
            bool named = false;
            unsigned bar = 0;
            int barNo = 1;
            for (size_t k = 1; k < w.size(); k++) {
                const std::string& tok = w[k];
                if (tok == "|") {
                    if (t.barTicks && bar != t.barTicks)
                        error(file, ln, "tune " + t.name + " v" + std::to_string(vn) + ": bar " + std::to_string(barNo) + " is " +
                              std::to_string(bar) + " ticks, not " + std::to_string(t.barTicks));
                    bar = 0;
                    barNo++;
                    continue;
                }
                if (tok[0] == '@') {
                    auto it = instId.find(tok.substr(1));
                    if (it == instId.end()) { error(file, ln, "unknown instrument " + tok.substr(1)); continue; }
                    v.stream.push_back(0x81);
                    v.stream.push_back((uint8_t)it->second);   // (renumbered in blob)
                    v.instruments.push_back(it->second);
                    named = true;
                    continue;
                }
                std::string lt = lower(tok);
                if (lt.rfind("arp:", 0) == 0) {
                    std::string a = lt.substr(4);
                    long x = 0, y = 0;
                    if (a != "off") {
                        size_t c = a.find(',');
                        if (!parseInt(a.substr(0, c), x) || (c != std::string::npos && !parseInt(a.substr(c + 1), y)) ||
                            x < 0 || x > 24 || y < 0 || y > 24) {
                            error(file, ln, "arp:a,b (semitones 0-24) or arp:off, not " + tok); continue;
                        }
                    }
                    v.stream.push_back(0x83);
                    v.stream.push_back((uint8_t)x);
                    v.stream.push_back((uint8_t)y);
                    continue;
                }
                size_t colon = tok.find(':');
                long d;
                if (colon == std::string::npos || !parseInt(tok.substr(colon + 1), d)) {
                    error(file, ln, "expected note:ticks, r:ticks, ~:ticks, @instrument, arp: or |, not " + tok); continue;
                }
                if (d < 1 || d > 4096) { error(file, ln, "a length of " + std::to_string(d) + " ticks: " + tok); continue; }
                std::string what = lower(tok.substr(0, colon));
                // a glide: note^semitones[,speed]
                long gs = 0, gk = 0;
                size_t caret = what.find('^');
                if (caret != std::string::npos) {
                    std::string g = what.substr(caret + 1);
                    what = what.substr(0, caret);
                    size_t c = g.find(',');
                    gk = 3;
                    if (!parseInt(g.substr(0, c), gs) || (c != std::string::npos && !parseInt(g.substr(c + 1), gk)) ||
                        gs < -127 || gs > 127 || gs == 0 || gk < 1 || gk > 7 || what == "r" || what == "~") {
                        error(file, ln, "a glide is note^semitones[,speed] (semitones -127 to 127, not 0; speed 1-7): " + tok);
                        continue;
                    }
                }
                int code;
                if (what == "r") code = 0x80;
                else if (what == "~") code = 0x82;
                else {
                    code = noteNumber(what);
                    if (code < 0) { error(file, ln, "not a note (c4 is middle C; c#4, eb4): " + tok); continue; }
                    if (!named) v.defaultInstrument = true;
                    if (gk) {
                        v.stream.push_back(0x84);
                        v.stream.push_back((uint8_t)(int8_t)gs);
                        v.stream.push_back((uint8_t)gk);
                    }
                }
                // over 255 ticks: the rest goes on as ties (a note) or rests
                long left = d;
                bool first = true;
                while (left > 0) {
                    long part = std::min(left, 255L);
                    v.stream.push_back((uint8_t)(first ? code : (code == 0x80 ? 0x80 : 0x82)));
                    v.stream.push_back((uint8_t)part);
                    left -= part;
                    first = false;
                }
                v.ticks += (unsigned)d;
                bar += (unsigned)d;
            }
            if (t.barTicks && bar != 0 && bar != t.barTicks)
                error(file, ln, "tune " + t.name + " v" + std::to_string(vn) + ": the last bar is " + std::to_string(bar) +
                      " ticks, not " + std::to_string(t.barTicks));
            t.voices.push_back(v);
        } else {
            error(file, ln, "what is " + w[0] + "? (instrument, wavetable, tune, v1:...)");
        }
    }
    for (const auto& t : out.tunes) {
        if (t.voices.empty()) { error(file, t.line, "tune " + t.name + " has no voices"); continue; }
        bool same = true;
        for (const auto& v : t.voices) if (v.ticks != t.voices[0].ticks) same = false;
        if (!same) {
            std::string l;
            for (size_t k = 0; k < t.voices.size(); k++)
                l += (k ? ", " : "") + std::string("v") + std::to_string(k + 1) + " " + std::to_string(t.voices[k].ticks);
            error(file, t.line, "tune " + t.name + ": the voices are not as long (ticks: " + l + ")");
        }
        bool needDefault = false;
        for (const auto& v : t.voices) needDefault |= v.defaultInstrument;
        if (needDefault && out.instruments.empty())
            error(file, t.line, "tune " + t.name + " plays notes but no instrument is defined");
    }
    return errors_.empty();
}

std::vector<int> TuneCompiler::usedInstruments(const TuneFile& f, const Tune& t) {
    std::vector<int> used;
    bool needDefault = false;
    for (const auto& v : t.voices) needDefault |= v.defaultInstrument;
    if (needDefault && !f.instruments.empty()) used.push_back(0);   // (instrument 0 until a voice names one)
    for (const auto& v : t.voices)
        for (int i : v.instruments)
            if (std::find(used.begin(), used.end(), i) == used.end()) used.push_back(i);
    return used;
}

std::vector<uint8_t> TuneCompiler::blob(const TuneFile& f, const Tune& t) {
    std::vector<int> used = usedInstruments(f, t);
    std::map<int, int> renum;
    for (size_t k = 0; k < used.size(); k++) renum[used[k]] = (int)k;
    size_t nv = t.voices.size();
    size_t insAt = 8 + 2 * nv;
    std::vector<uint8_t> b(insAt, 0);
    b[0] = t.loop ? 1 : 0;
    b[1] = (uint8_t)nv;
    b[2] = (uint8_t)(t.fpt & 0xFF);
    b[3] = (uint8_t)(t.fpt >> 8);
    b[4] = (uint8_t)used.size();
    b[5] = kInstrumentSize;
    b[6] = (uint8_t)(insAt & 0xFF);
    b[7] = (uint8_t)(insAt >> 8);
    // the wave tables the instruments use, after them: length, loop ($FF
    // none), a waveform and semitones a step
    std::map<int, size_t> tableAt;
    size_t at = insAt + used.size() * kInstrumentSize;
    std::vector<uint8_t> tables;
    for (int i : used) {
        int tb = f.instruments[(size_t)i].table;
        if (tb < 0 || tableAt.count(tb)) continue;
        tableAt[tb] = at + tables.size();
        const auto& T = f.tables[(size_t)tb];
        tables.push_back((uint8_t)T.steps.size());
        tables.push_back(T.loop < 0 ? 0xFF : (uint8_t)T.loop);
        for (const auto& st : T.steps) { tables.push_back(st.first); tables.push_back((uint8_t)st.second); }
    }
    for (int i : used) {
        const auto& in = f.instruments[(size_t)i];
        uint16_t ta = in.table < 0 ? 0 : (uint16_t)tableAt[in.table];
        uint16_t cut = in.filt ? in.cutoff : 0;
        uint8_t r[kInstrumentSize] = {in.wave, in.ad, in.sr, (uint8_t)(in.pw & 0xFF), (uint8_t)(in.pw >> 8),
                                      in.vib, in.gap, (uint8_t)in.pwm, in.vdelay, (uint8_t)in.slide,
                                      in.hr, in.filt, (uint8_t)(cut & 0xFF), (uint8_t)(cut >> 8),
                                      (uint8_t)in.fsweep, (uint8_t)(ta & 0xFF), (uint8_t)(ta >> 8)};
        b.insert(b.end(), r, r + kInstrumentSize);
    }
    b.insert(b.end(), tables.begin(), tables.end());
    for (size_t v = 0; v < nv; v++) {
        size_t at = b.size();
        b[8 + 2 * v] = (uint8_t)(at & 0xFF);
        b[9 + 2 * v] = (uint8_t)(at >> 8);
        const auto& s = t.voices[v].stream;
        for (size_t k = 0; k < s.size(); k++) {
            uint8_t c = s[k];
            b.push_back(c);
            if (c < 0x80 || c == 0x80 || c == 0x82) { b.push_back(s[++k]); }
            else if (c == 0x81) { b.push_back((uint8_t)renum[s[++k]]); }
            else if (c == 0x83 || c == 0x84) { b.push_back(s[k + 1]); b.push_back(s[k + 2]); k += 2; }
        }
        b.push_back(t.loop ? 0xFE : 0xFF);
    }
    return b;
}
