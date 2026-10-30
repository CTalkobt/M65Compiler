// TuneCompiler.cpp — tune45's music text and the player's data (see TuneCompiler.hpp)
#include "TuneCompiler.hpp"
#include "PackAudio.hpp"
#include <algorithm>
#include <fstream>
#include <iterator>
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
    warnings_.clear();
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
    std::map<std::string, int> instId, tableId, sampleId, patternId;
    // a voice's tokens with its pattern calls (*name, *name*n) expanded for
    // the counting (ticks, bars, hits), between markers: \x01C id n (the
    // call; the first time through its stream is the pattern's), \x01M (the
    // rest of the times: counted only), \x01E
    auto expand = [&](std::vector<std::string>& w, int ln) {
        std::vector<std::string> o = {w[0]};
        for (size_t k = 1; k < w.size(); k++) {
            if (w[k].size() < 2 || w[k][0] != '*') { o.push_back(w[k]); continue; }
            std::string n = w[k].substr(1);
            long times = 1;
            size_t star = n.find('*');
            if (star != std::string::npos) {
                if (!parseInt(n.substr(star + 1), times) || times < 1 || times > 255) { error(file, ln, "*pattern*1-255, not " + w[k]); continue; }
                n = n.substr(0, star);
            }
            auto it = patternId.find(n);
            if (it == patternId.end()) { error(file, ln, "unknown pattern " + n + " (a pattern line before the voice)"); continue; }
            const auto& body = out.patterns[(size_t)it->second].tokens;
            o.push_back("\001C " + std::to_string(it->second) + " " + std::to_string(times));
            o.insert(o.end(), body.begin(), body.end());
            o.push_back("\001M");
            for (long r = 1; r < times; r++) o.insert(o.end(), body.begin(), body.end());
            o.push_back("\001E");
        }
        w = o;
    };
    // a marker in a voice's tokens: the call put in its stream, the stream
    // turned to the pattern's body, the junk, or back (mode 0 SID, 1 digi)
    struct Call { int id = -1; std::vector<uint8_t> saved; };
    auto marker = [&](const std::string& tok, TuneVoice& v, Call& c, int mode) {
        if (tok[1] == 'C') {
            int id, times;
            sscanf(tok.c_str() + 3, "%d %d", &id, &times);
            v.stream.push_back(0x86);
            v.stream.push_back((uint8_t)(id & 0xFF));
            v.stream.push_back((uint8_t)(id >> 8));
            v.stream.push_back((uint8_t)times);
            c.id = id;
            c.saved.swap(v.stream);
            v.stream.clear();
        } else if (tok[1] == 'M') {
            auto& p = out.patterns[(size_t)c.id];
            if (!p.have[mode]) { p.body[mode] = v.stream; p.have[mode] = true; }
            v.stream.clear();
        } else {
            v.stream.swap(c.saved);
            c.saved.clear();
        }
    };
    std::map<std::pair<std::vector<int>, int>, int> hitId;   // (samples, semitones): the hit
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
        if (kw == "pattern") {
            // pattern <name>: tokens (as a voice's; no pattern calls)
            std::string n = w.size() > 1 ? w[1] : "";
            size_t k = 2;
            if (!n.empty() && n.back() == ':') n.pop_back();
            else if (w.size() > 2 && w[2] == ":") k = 3;
            if (n.empty() || n.find('*') != std::string::npos) { error(file, ln, "pattern <name>: <tokens>"); continue; }
            if (patternId.count(n)) { error(file, ln, "pattern " + n + " defined twice"); continue; }
            TunePattern pt;
            pt.name = n;
            pt.line = ln;
            for (; k < w.size(); k++) {
                if (w[k][0] == '*') { error(file, ln, "a pattern calls no pattern: " + w[k]); continue; }
                pt.tokens.push_back(w[k]);
            }
            if (pt.tokens.empty()) { error(file, ln, "pattern " + n + " is empty"); continue; }
            patternId[n] = (int)out.patterns.size();
            out.patterns.push_back(pt);
            continue;
        }
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
                if (key == "tail" && eq == std::string::npos) { ins.hr |= 0x80; continue; }   // (its effects through the release)
                if (key == "free" && eq == std::string::npos) { ins.hr |= 0x40; continue; }   // (its pulse-width sweep without bounces)
                if (key == "pwrun" && eq == std::string::npos) { ins.hr |= 0x20; continue; }   // (its pulse width not set by a note)
                if (key == "cutrun" && eq == std::string::npos) { ins.hr |= 0x10; continue; }   // (its chip's cutoff not set by its note)
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
                else if (key == "vspeed" && v2 >= 1 && v2 <= 16) ins.vspeed = (uint8_t)v2;
                else if (key == "slide" && v2 >= -128 && v2 <= 127) ins.slide = (int8_t)v2;
                else if (key == "hr" && v2 >= 0 && v2 <= 3) ins.hr = (uint8_t)((ins.hr & 0xF0) | v2);
                else if (key == "res" && v2 >= 0 && v2 <= 15) { res = v2; filterTuned = true; }
                else if (key == "fsweep" && v2 >= -128 && v2 <= 127) { ins.fsweep = (int8_t)v2; filterTuned = true; }
                else {
                    error(file, ln, "unknown or out of range: " + w[k] +
                                    " (pwm, vdelay, vspeed 1-16, slide, hr 0-3, tail, free, pwrun, cutrun, filter, res 0-15, cutoff, fsweep, table)");
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
        } else if (kw == "sample") {
            if (w.size() < 3) { error(file, ln, "sample <name> <file> [rate=hz] [gain=percent] [from=s] [len=s]"); continue; }
            TuneSample sm;
            sm.name = w[1];
            sm.path = w[2];
            sm.line = ln;
            bool ok = true;
            for (size_t k = 3; k < w.size(); k++) {
                size_t eq = w[k].find('=');
                long v;
                std::string key = lower(w[k].substr(0, eq));
                if (eq != std::string::npos && key == "note") {
                    int n = noteNumber(lower(w[k].substr(eq + 1)));
                    if (n < 0) { error(file, ln, "note=<note> (c4 is middle C), not " + w[k]); ok = false; }
                    else sm.note = n;
                    continue;
                }
                if (lower(w[k]) == "loop") { sm.loop = 0; continue; }
                if (eq != std::string::npos && (key == "from" || key == "len" || key == "loop")) {
                    // seconds, with a fraction
                    char* end = nullptr;
                    std::string val = w[k].substr(eq + 1);
                    double secs = std::strtod(val.c_str(), &end);
                    if (val.empty() || !end || *end || secs < 0 || secs > 3600 || (key == "len" && secs <= 0)) {
                        error(file, ln, key + "=seconds (0.25), not " + val);
                        ok = false;
                    } else (key == "from" ? sm.from : key == "len" ? sm.len : sm.loop) = secs;
                    continue;
                }
                if (eq == std::string::npos || !parseInt(w[k].substr(eq + 1), v)) { error(file, ln, "expected key=number: " + w[k]); ok = false; }
                else if (key == "rate" && v >= 1000 && v <= 48000) sm.rate = (uint32_t)v;
                else if (key == "gain" && v >= 1 && v <= 1000) sm.gain = (int)v;
                else { error(file, ln, "unknown or out of range: " + w[k] + " (rate 1000-48000, gain 1-1000 per cent, from, len, note, loop)"); ok = false; }
            }
            if (!ok) continue;
            if (sm.name.find('+') != std::string::npos || sm.name.find(':') != std::string::npos ||
                lower(sm.name) == "r" || sm.name == "-" || lower(sm.name) == "vol") {
                error(file, ln, "a sample's name has no + or : and is not r, - or vol: " + sm.name);
                continue;
            }
            if (sampleId.count(sm.name)) { error(file, ln, "sample " + sm.name + " defined twice"); continue; }
            sampleId[sm.name] = (int)out.samples.size();
            out.samples.push_back(sm);
        } else if (kw.size() >= 3 && kw[0] == 'd' && kw.back() == ':' && std::isdigit((unsigned char)kw[1])) {
            long dn;
            if (!parseInt(kw.substr(1, kw.size() - 2), dn) || dn < 1 || dn > 4) { error(file, ln, "a digi voice is d1: to d4:, not " + w[0]); continue; }
            if (out.tunes.empty()) { error(file, ln, "a voice before any tune"); continue; }
            Tune& t = out.tunes.back();
            if ((size_t)dn != t.digi.size() + 1) {
                error(file, ln, "tune " + t.name + ": d" + std::to_string(t.digi.size() + 1) + ": expected, not " + w[0]);
                continue;
            }
            TuneVoice v;
            v.line = ln;
            unsigned bar = 0;
            int barNo = 1;
            expand(w, ln);
            Call call;
            for (size_t k = 1; k < w.size(); k++) {
                const std::string& tok = w[k];
                if (tok[0] == '\x01') { marker(tok, v, call, 1); continue; }
                if (tok == "|") {
                    if (t.barTicks && bar != t.barTicks)
                        error(file, ln, "tune " + t.name + " d" + std::to_string(dn) + ": bar " + std::to_string(barNo) + " is " +
                              std::to_string(bar) + " ticks, not " + std::to_string(t.barTicks));
                    bar = 0;
                    barNo++;
                    continue;
                }
                if (lower(tok).rfind("vol:", 0) == 0) {   // the voice's volume from here (0-64), no time
                    long vv;
                    if (!parseInt(tok.substr(4), vv) || vv < 0 || vv > 64) { error(file, ln, "vol:0-64, not " + tok); continue; }
                    v.stream.push_back(0x83);
                    v.stream.push_back((uint8_t)std::min(255L, vv * 4));
                    continue;
                }
                size_t colon = tok.rfind(':');
                long d;
                if (colon == std::string::npos || colon == 0 || !parseInt(tok.substr(colon + 1), d)) {
                    error(file, ln, "expected sample:ticks, sample:note:ticks, a+b:ticks, r:ticks, ~:ticks, -:ticks, vol: or |, not " + tok);
                    continue;
                }
                if (d < 1 || d > 4096) { error(file, ln, "a length of " + std::to_string(d) + " ticks: " + tok); continue; }
                std::string what = tok.substr(0, colon);
                int code;
                int pitch = -1;   // a pitched hit's note
                size_t c2 = what.rfind(':');
                if (c2 != std::string::npos) {
                    pitch = noteNumber(lower(what.substr(c2 + 1)));
                    if (pitch < 0) { error(file, ln, "not a note (c4 is middle C) in " + tok); continue; }
                    what = what.substr(0, c2);
                }
                if (lower(what) == "r" && pitch < 0) code = 0x80;
                else if (what == "~" && pitch < 0) code = 0x82;
                else if (what == "-" && pitch < 0) { code = 0x81; v.hits.push_back({-1, v.ticks}); }   // stopped
                else {
                    // the samples sounding at once: one hit, mixed
                    std::vector<int> ids;
                    bool good = true;
                    size_t at = 0;
                    while (good) {
                        size_t plus = what.find('+', at);
                        auto it = sampleId.find(what.substr(at, plus == std::string::npos ? std::string::npos : plus - at));
                        if (it == sampleId.end()) { error(file, ln, "unknown sample in " + tok + " (a sample line before the voice)"); good = false; break; }
                        if (std::find(ids.begin(), ids.end(), it->second) == ids.end()) ids.push_back(it->second);
                        if (plus == std::string::npos) break;
                        at = plus + 1;
                    }
                    if (!good) continue;
                    std::sort(ids.begin(), ids.end());
                    int semis = pitch < 0 ? 0 : pitch - out.samples[(size_t)ids[0]].note;
                    auto h = hitId.find({ids, semis});
                    if (h == hitId.end()) {
                        if (out.hits.size() >= 256) { error(file, ln, "over 256 different hits in the file"); continue; }
                        h = hitId.emplace(std::make_pair(ids, semis), (int)out.hits.size()).first;
                        TuneHit nh;
                        nh.samples = ids;
                        nh.semis = semis;
                        out.hits.push_back(nh);
                    }
                    code = 0x100 + h->second;   // (a hit: its number in the file, 16 bits in the stream; blob numbers it)
                    v.hits.push_back({h->second, v.ticks});
                }
                long left = d;
                bool first = true;
                while (left > 0) {
                    long part = std::min(left, 255L);
                    if (first && code >= 0x100) {
                        v.stream.push_back(0x01);
                        v.stream.push_back((uint8_t)(code & 0xFF));
                        v.stream.push_back((uint8_t)((code - 0x100) >> 8));
                    } else v.stream.push_back((uint8_t)(first ? code : code == 0x80 || code == 0x81 ? 0x80 : 0x82));
                    v.stream.push_back((uint8_t)part);
                    left -= part;
                    first = false;
                }
                v.ticks += (unsigned)d;
                bar += (unsigned)d;
            }
            if (t.barTicks && bar != 0 && bar != t.barTicks)
                error(file, ln, "tune " + t.name + " d" + std::to_string(dn) + ": the last bar is " + std::to_string(bar) +
                      " ticks, not " + std::to_string(t.barTicks));
            t.digi.push_back(v);
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
            expand(w, ln);
            Call call;
            for (size_t k = 1; k < w.size(); k++) {
                const std::string& tok = w[k];
                if (tok[0] == '\x01') { marker(tok, v, call, 0); continue; }
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
                if (lt.rfind("tempo:", 0) == 0) {
                    char* end = nullptr;
                    double x = std::strtod(lt.c_str() + 6, &end);
                    if (lt.size() == 6 || !end || *end || x < 0.1 || x > 255) {
                        error(file, ln, "tempo:frames per tick (0.1-255), not " + tok); continue;
                    }
                    uint16_t q = (uint16_t)std::lround(x * 256);
                    v.stream.push_back(0x85);
                    v.stream.push_back((uint8_t)(q & 0xFF));
                    v.stream.push_back((uint8_t)(q >> 8));
                    t.tempos.push_back({v.ticks, q});
                    continue;
                }
                if (lt.rfind("arp2:", 0) == 0) {   // two notes in turn: the note, +a
                    long x;
                    if (!parseInt(lt.substr(5), x) || x < 0 || x > 24) { error(file, ln, "arp2:a (semitones 0-24), not " + tok); continue; }
                    v.stream.push_back(0x83);
                    v.stream.push_back((uint8_t)x);
                    v.stream.push_back(0x80);
                    continue;
                }
                if (lt.rfind("pw:", 0) == 0 || lt.rfind("cut:", 0) == 0) {   // pw:hex,sweep / cut:hex,sweep
                    bool pwTok = lt[0] == 'p';
                    std::string a = lt.substr(pwTok ? 3 : 4);
                    size_t c = a.find(',');
                    long val, sw = 0;
                    long max = pwTok ? 0xFFF : 0x7FF;
                    if (!parseInt(a.substr(0, c), val, 16) || val < 0 || val > max ||
                        (c != std::string::npos && !parseInt(a.substr(c + 1), sw)) || sw < -128 || sw > 127) {
                        error(file, ln, std::string(pwTok ? "pw:hex 0-fff" : "cut:hex 0-7ff") + "[,sweep -128..127], not " + tok);
                        continue;
                    }
                    v.stream.push_back(pwTok ? 0x89 : 0x8A);
                    v.stream.push_back((uint8_t)(val & 0xFF));
                    v.stream.push_back((uint8_t)(val >> 8));
                    v.stream.push_back((uint8_t)(int8_t)sw);
                    continue;
                }
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
                    error(file, ln, "expected note:ticks, ~note:ticks, r:ticks, ~:ticks, @instrument, arp:, arp2:, pw:, cut:, tempo: or |, not " + tok); continue;
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
                int code, legato = -1;
                if (what == "r") code = 0x80;
                else if (what == "~") code = 0x82;
                else if (what.size() > 1 && what[0] == '~') {   // ~e4: legato (the pitch; the note goes on)
                    legato = noteNumber(what.substr(1));
                    if (legato < 0 || gk) { error(file, ln, "a legato note is ~note:ticks (no glide): " + tok); continue; }
                    code = 0x88;
                } else {
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
                    if (first && code == 0x88) v.stream.push_back((uint8_t)legato);
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
            error(file, ln, "what is " + w[0] + "? (instrument, wavetable, sample, pattern, tune, v1:..., d1:...)");
        }
    }
    for (auto& t : out.tunes)           // (one tick: the later voice's wins, as the player reads them)
        std::stable_sort(t.tempos.begin(), t.tempos.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    for (const auto& t : out.tunes) {
        if (t.voices.empty() && t.digi.empty()) { error(file, t.line, "tune " + t.name + " has no voices"); continue; }
        unsigned len = t.voices.empty() ? t.digi[0].ticks : t.voices[0].ticks;
        bool same = true;
        for (const auto& v : t.voices) if (v.ticks != len) same = false;
        for (const auto& v : t.digi) if (v.ticks != len) same = false;
        if (!same) {
            std::string l;
            for (size_t k = 0; k < t.voices.size(); k++)
                l += (k ? ", " : "") + std::string("v") + std::to_string(k + 1) + " " + std::to_string(t.voices[k].ticks);
            for (size_t k = 0; k < t.digi.size(); k++)
                l += (l.empty() ? "" : ", ") + std::string("d") + std::to_string(k + 1) + " " + std::to_string(t.digi[k].ticks);
            error(file, t.line, "tune " + t.name + ": the voices are not as long (ticks: " + l + ")");
        }
        bool needDefault = false;
        for (const auto& v : t.voices) needDefault |= v.defaultInstrument;
        if (needDefault && out.instruments.empty())
            error(file, t.line, "tune " + t.name + " plays notes but no instrument is defined");
    }
    if (errors_.empty() && !out.hits.empty()) loadSamples(file, out);
    return errors_.empty();
}

uint32_t TuneCompiler::dmaRate(uint32_t hz) { return (uint32_t)std::lround((double)hz * 16777216.0 / 40500000.0); }

std::string TuneCompiler::hitName(const TuneFile& f, int hit) {
    std::string n;
    const TuneHit& h = f.hits[(size_t)hit];
    for (int s : h.samples) n += (n.empty() ? "" : "+") + f.samples[(size_t)s].name;
    if (h.semis) n += ":" + noteName(f.samples[(size_t)h.samples[0]].note + h.semis);
    return n;
}

// The samples the hits use, read (beside the text file) and made 8-bit at
// their rates; each hit mixed (its samples at the highest of their rates,
// added, clipped) into the bank. Then each digi voice's hits that the next
// cuts short (the next hit on a channel stops the one before): a warning
bool TuneCompiler::loadSamples(const std::string& file, TuneFile& f) {
    std::string dir;
    size_t slash = file.find_last_of('/');
    if (slash != std::string::npos) dir = file.substr(0, slash + 1);
    std::vector<pack45::Wav> wav(f.samples.size());
    std::vector<bool> used(f.samples.size(), false);
    for (const auto& h : f.hits) for (int s : h.samples) used[(size_t)s] = true;
    for (size_t k = 0; k < f.samples.size(); k++) {
        if (!used[k]) continue;
        TuneSample& sm = f.samples[k];
        std::string path = !sm.path.empty() && sm.path[0] == '/' ? sm.path : dir + sm.path;
        std::ifstream in(path, std::ios::binary);
        if (!in) { error(file, sm.line, "cannot read " + path); continue; }
        std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        std::string err;
        if (!pack45::readSound(path, bytes, wav[k], &err) || wav[k].samples.empty()) {
            error(file, sm.line, path + ": " + (err.empty() ? "no samples" : err));
            continue;
        }
        // a part of it (from, len), its end faded over 5 ms (no click where it is cut)
        if (sm.from > 0 || sm.len > 0) {
            auto& a = wav[k].samples;
            size_t at = (size_t)std::lround(sm.from * wav[k].rate);
            if (at >= a.size()) {
                char m[96];
                snprintf(m, sizeof m, ": from=%g is past its end (%.2f s)", sm.from, (double)a.size() / wav[k].rate);
                error(file, sm.line, path + m);
                continue;
            }
            size_t n = sm.len > 0 ? std::min(a.size() - at, (size_t)std::lround(sm.len * wav[k].rate)) : a.size() - at;
            a = std::vector<double>(a.begin() + (long)at, a.begin() + (long)(at + n));
            if (sm.len > 0) {
                size_t fade = std::min(n, (size_t)(wav[k].rate / 200));
                for (size_t i = 0; i < fade; i++) a[n - 1 - i] *= (double)i / fade;
            }
        }
        if (!sm.rate) {
            uint32_t top = std::min<uint32_t>(12000, wav[k].rate);
            sm.rate = pack45::rateFor(pack45::energyBelow(wav[k], 0.999, top < wav[k].rate ? 0.45 * top : 0), top);
        }
        for (double& x : wav[k].samples) x *= sm.gain / 100.0;
    }
    if (!errors_.empty()) return false;
    f.bank.clear();
    // each set of samples once in the bank; its hits at every pitch share it
    // (a pitched hit: its rate 2^(semitones/12) times the set's)
    std::map<std::vector<int>, const TuneHit*> placed;
    for (size_t hk = 0; hk < f.hits.size(); hk++) {
        TuneHit& h = f.hits[hk];
        auto was = placed.find(h.samples);
        uint32_t rate = 0;
        if (was != placed.end()) {
            h.at = was->second->at;
            h.len = was->second->len;
            h.loop = was->second->loop;
            rate = (uint32_t)std::lround(was->second->rate * std::pow(2.0, -was->second->semis / 12.0));
        } else {
            for (int s : h.samples) rate = std::max(rate, f.samples[(size_t)s].rate);
            std::vector<double> mix;
            for (int s : h.samples) {
                auto part = pack45::resample(wav[(size_t)s], rate);
                if (part.size() > mix.size()) mix.resize(part.size(), 0.0);
                for (size_t i = 0; i < part.size(); i++) mix[i] += part[i];
            }
            auto b = pack45::toBytes(mix, true, false);
            if (b.size() > 0xFFFF) {
                error(file, f.samples[(size_t)h.samples[0]].line, hitName(f, (int)hk) + ": over 65535 samples (" +
                      std::to_string(b.size()) + ") at " + std::to_string(rate) + " Hz");
                return false;
            }
            h.at = (uint32_t)f.bank.size();
            h.len = (uint32_t)b.size();
            // a loop: one sample's (a mix is played once)
            const TuneSample& first = f.samples[(size_t)h.samples[0]];
            h.loop = h.samples.size() == 1 && first.loop >= 0 ? (int32_t)std::min<double>(h.len - 1, std::lround(first.loop * rate)) : -1;
            f.bank.insert(f.bank.end(), b.begin(), b.end());
        }
        h.rate = (uint32_t)std::lround(rate * std::pow(2.0, h.semis / 12.0));
        if (was == placed.end()) placed[h.samples] = &h;
        if (h.rate > 48000) {
            error(file, f.samples[(size_t)h.samples[0]].line, hitName(f, (int)hk) + ": played at " + std::to_string(h.rate) +
                  " Hz (the audio DMA plays at most 48000: a lower note, or a lower rate=)");
            return false;
        }
    }
    if (f.bank.size() > 0x10000) {
        error(file, f.samples[0].line, "the samples are " + std::to_string(f.bank.size()) + " bytes: over 64 KB (one bank)");
        return false;
    }
    for (const auto& t : f.tunes)
        for (size_t d = 0; d < t.digi.size(); d++) {
            const auto& hs = t.digi[d].hits;
            std::map<int, std::pair<int, double>> cut;   // hit: times cut, the shortest it rang
            for (size_t k = 0; k + 1 < hs.size(); k++) {
                if (hs[k].first < 0) continue;   // (a stop)
                double rang = (t.frames(hs[k + 1].second) - t.frames(hs[k].second)) / 50.0;
                const TuneHit& h = f.hits[(size_t)hs[k].first];
                if (h.loop < 0 && hs[k + 1].first >= 0 && rang < (double)h.len / h.rate) {
                    auto& c = cut[hs[k].first];
                    c.second = c.first ? std::min(c.second, rang) : rang;
                    c.first++;
                }
            }
            for (const auto& [hit, c] : cut) {
                const TuneHit& h = f.hits[(size_t)hit];
                char m[256];
                snprintf(m, sizeof m, "tune %s d%zu: %s (%.2f s) is cut short by the next hit %d time%s (after %.2f s at least)",
                         t.name.c_str(), d + 1, hitName(f, hit).c_str(), (double)h.len / h.rate, c.first,
                         c.first == 1 ? "" : "s", c.second);
                warnings_.push_back(file + ":" + std::to_string(t.digi[d].line) + ": warning: " + m);
            }
        }
    return true;
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

std::vector<int> TuneCompiler::usedHits(const Tune& t) {
    std::vector<int> used;
    for (const auto& v : t.digi)
        for (const auto& h : v.hits)
            if (h.first >= 0 && std::find(used.begin(), used.end(), h.first) == used.end()) used.push_back(h.first);
    return used;
}

std::vector<uint8_t> TuneCompiler::blob(const TuneFile& f, const Tune& t) {
    std::vector<int> used = usedInstruments(f, t);
    std::map<int, int> renum;
    for (size_t k = 0; k < used.size(); k++) renum[used[k]] = (int)k;
    std::vector<int> hits = usedHits(t);
    std::map<int, int> hitNum;
    for (size_t k = 0; k < hits.size(); k++) hitNum[hits[k]] = (int)k;
    size_t nv = t.voices.size(), nd = t.digi.size();
    // the header, each voice's offset (the SID voices', the digi voices'),
    // and with digi voices the sample table's
    size_t insAt = 8 + 2 * (nv + nd) + (nd ? 2 : 0);
    std::vector<uint8_t> b(insAt, 0);
    b[0] = (uint8_t)((t.loop ? 1 : 0) | (nd << 1) | (nd ? 0x10 : 0));   // (bit 4: 9-byte sample entries, a loop's)
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
                                      (uint8_t)(in.vib | (in.vspeed - 1) << 4), in.gap, (uint8_t)in.pwm, in.vdelay, (uint8_t)in.slide,
                                      in.hr, in.filt, (uint8_t)(cut & 0xFF), (uint8_t)(cut >> 8),
                                      (uint8_t)in.fsweep, (uint8_t)(ta & 0xFF), (uint8_t)(ta >> 8)};
        b.insert(b.end(), r, r + kInstrumentSize);
    }
    b.insert(b.end(), tables.begin(), tables.end());
    // the sample table: each hit's place in the bank, its bytes, its rate
    // (the audio DMA's register value)
    if (nd) {
        size_t st = b.size();
        b[8 + 2 * (nv + nd)] = (uint8_t)(st & 0xFF);
        b[9 + 2 * (nv + nd)] = (uint8_t)(st >> 8);
        for (int h : hits) {
            const TuneHit& th = f.hits[(size_t)h];
            uint32_t r = dmaRate(th.rate);
            uint16_t lp = th.loop < 0 ? 0xFFFF : (uint16_t)th.loop;
            uint8_t e[9] = {(uint8_t)th.at, (uint8_t)(th.at >> 8), (uint8_t)th.len, (uint8_t)(th.len >> 8),
                            (uint8_t)r, (uint8_t)(r >> 8), (uint8_t)(r >> 16), (uint8_t)lp, (uint8_t)(lp >> 8)};
            b.insert(b.end(), e, e + 9);
        }
    }
    // a stream into the data: a SID voice's (instruments renumbered) or a
    // digi voice's (hits numbered in the tune's table); a pattern call's
    // offset filled in once its body is placed
    std::vector<std::pair<size_t, int>> calls;   // where an offset goes; the pattern (id * 2 + mode)
    auto emitSid = [&](const std::vector<uint8_t>& s) {
        for (size_t k = 0; k < s.size(); k++) {
            uint8_t c = s[k];
            b.push_back(c);
            if (c < 0x80 || c == 0x80 || c == 0x82) { b.push_back(s[++k]); }
            else if (c == 0x81) { b.push_back((uint8_t)renum[s[++k]]); }
            else if (c == 0x83 || c == 0x84 || c == 0x85 || c == 0x88) { b.push_back(s[k + 1]); b.push_back(s[k + 2]); k += 2; }
            else if (c == 0x89 || c == 0x8A) { b.push_back(s[k + 1]); b.push_back(s[k + 2]); b.push_back(s[k + 3]); k += 3; }
            else if (c == 0x86) {
                calls.push_back({b.size(), (s[k + 1] | s[k + 2] << 8) * 2});
                b.push_back(0); b.push_back(0); b.push_back(s[k + 3]);
                k += 3;
            }
        }
    };
    auto emitDigi = [&](const std::vector<uint8_t>& s) {
        for (size_t k = 0; k + 1 < s.size();) {
            if (s[k] == 0x01) {   // a hit: its number in the tune's table ($84 h d past 127)
                int n = hitNum[s[k + 1] | s[k + 2] << 8];
                if (n >= 0x80) b.push_back(0x84);
                b.push_back((uint8_t)n);
                b.push_back(s[k + 3]);
                k += 4;
            } else if (s[k] == 0x86) {
                b.push_back(0x86);
                calls.push_back({b.size(), (s[k + 1] | s[k + 2] << 8) * 2 + 1});
                b.push_back(0); b.push_back(0); b.push_back(s[k + 3]);
                k += 4;
            } else {
                b.push_back(s[k]);
                b.push_back(s[k + 1]);
                k += 2;
            }
        }
    };
    for (size_t v = 0; v < nv; v++) {
        size_t at = b.size();
        b[8 + 2 * v] = (uint8_t)(at & 0xFF);
        b[9 + 2 * v] = (uint8_t)(at >> 8);
        if (v == 0 && !t.tempos.empty()) {   // a tune with tempo: changes: its own again from the start (a loop)
            b.push_back(0x85);
            b.push_back((uint8_t)(t.fpt & 0xFF));
            b.push_back((uint8_t)(t.fpt >> 8));
        }
        emitSid(t.voices[v].stream);
        b.push_back(t.loop ? 0xFE : 0xFF);
    }
    for (size_t d = 0; d < nd; d++) {
        size_t at = b.size();
        b[8 + 2 * (nv + d)] = (uint8_t)(at & 0xFF);
        b[9 + 2 * (nv + d)] = (uint8_t)(at >> 8);
        emitDigi(t.digi[d].stream);
        b.push_back(t.loop ? 0xFE : 0xFF);
    }
    // the patterns called, once each, each ending $87
    std::map<int, size_t> placed;
    for (size_t k = 0; k < calls.size(); k++) {   // (a body places none of its own: no calls in patterns)
        int key = calls[k].second;
        if (placed.count(key)) continue;
        placed[key] = b.size();
        const auto& p = f.patterns[(size_t)key / 2];
        if (key % 2) emitDigi(p.body[1]);
        else emitSid(p.body[0]);
        b.push_back(0x87);
    }
    for (const auto& [at, key] : calls) {
        b[at] = (uint8_t)(placed[key] & 0xFF);
        b[at + 1] = (uint8_t)(placed[key] >> 8);
    }
    return b;
}
