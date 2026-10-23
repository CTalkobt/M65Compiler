// TuneMidi.cpp — tune45 import (see TuneMidi.hpp): a Standard MIDI File's
// tracks (or channels) as monophonic voices of tune45 text, on a grid of
// ticksPerBeat a quarter note, the tempo and the time signature its first
#include "TuneMidi.hpp"
#include "TuneCompiler.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <sstream>

namespace {

struct Note { long start, end; int pitch, program; };

struct Source {
    int number = 0;             // the track or channel (from 1)
    std::vector<Note> notes;
};

struct Reader {
    const std::vector<unsigned char>& d;
    size_t p = 0;
    bool bad = false;
    explicit Reader(const std::vector<unsigned char>& data) : d(data) {}
    unsigned u8() { if (p >= d.size()) { bad = true; return 0; } return d[p++]; }
    unsigned u16() { unsigned a = u8(); return (a << 8) | u8(); }
    unsigned long u32() { unsigned long a = u16(); return (a << 16) | u16(); }
    unsigned long vlq() {
        unsigned long v = 0;
        for (int k = 0; k < 4; k++) {
            unsigned b = u8();
            v = (v << 7) | (b & 0x7F);
            if (!(b & 0x80)) return v;
        }
        bad = true;
        return v;
    }
};

}  // namespace

std::string midiImport(const std::vector<unsigned char>& file, const MidiImportOptions& o, std::string& error) {
    Reader r(file);
    if (file.size() < 14 || std::string(file.begin(), file.begin() + 4) != "MThd") { error = "not a MIDI file (no MThd)"; return ""; }
    r.p = 4;
    unsigned long hlen = r.u32();
    unsigned format = r.u16(), ntracks = r.u16(), division = r.u16();
    r.p = 8 + hlen;
    if (division & 0x8000) { error = "SMPTE time in the MIDI file: only ticks a quarter note are read"; return ""; }
    if (!division) { error = "a MIDI file of 0 ticks a quarter note"; return ""; }
    unsigned long tempo = 500000;       // microseconds a quarter note (120 bpm)
    bool tempoSeen = false, sigSeen = false;
    int sigNum = 4, sigDen = 4;
    bool byChannel = o.channels || format == 0;
    std::map<int, Source> sources;      // by track or channel
    std::map<int, std::string> names;   // track names
    for (unsigned t = 0; t < ntracks && !r.bad; t++) {
        if (r.p + 8 > file.size()) break;
        std::string id(file.begin() + (long)r.p, file.begin() + (long)r.p + 4);
        r.p += 4;
        unsigned long len = r.u32(), end = r.p + len;
        if (id != "MTrk") { r.p = end; continue; }
        long now = 0;
        unsigned status = 0;
        int program[16] = {};
        std::map<int, std::pair<long, int>> on;   // (channel << 8 | pitch) -> start, program
        while (r.p < end && !r.bad) {
            now += (long)r.vlq();
            unsigned b = r.u8();
            if (b == 0xFF) {
                unsigned type = r.u8();
                unsigned long l = r.vlq();
                size_t at = r.p;
                if (type == 0x51 && l >= 3 && !tempoSeen) {
                    tempo = ((unsigned long)file[at] << 16) | ((unsigned long)file[at + 1] << 8) | file[at + 2];
                    tempoSeen = true;
                } else if (type == 0x58 && l >= 2 && !sigSeen) {
                    sigNum = file[at];
                    sigDen = 1 << file[at + 1];
                    sigSeen = true;
                } else if (type == 0x03 && l > 0) {
                    names[(int)t + 1] = std::string(file.begin() + (long)at, file.begin() + (long)(at + l));
                }
                r.p = at + l;
                if (type == 0x2F) break;
                continue;
            }
            if (b == 0xF0 || b == 0xF7) { r.p += r.vlq(); continue; }
            unsigned d1;
            if (b & 0x80) { status = b; d1 = r.u8(); }
            else { d1 = b; }   // running status
            unsigned type = status & 0xF0, ch = status & 0x0F;
            unsigned d2 = (type == 0xC0 || type == 0xD0) ? 0 : r.u8();
            if (type == 0xC0) { program[ch] = (int)d1; continue; }
            if (type != 0x80 && type != 0x90) continue;
            if (ch == 9 && !o.drums) continue;
            int key = (int)(ch << 8 | d1);
            int src = byChannel ? (int)ch + 1 : (int)t + 1;
            auto it = on.find(key);
            if (it != on.end()) {       // (a note on again or off: the one sounding ends)
                sources[src].number = src;
                sources[src].notes.push_back({it->second.first, now, (int)d1, it->second.second});
                on.erase(it);
            }
            if (type == 0x90 && d2) on[key] = {now, program[ch]};
        }
        r.p = end;
    }
    // the sources: those named, else the first with notes
    std::vector<Source*> use;
    if (!o.sources.empty()) {
        for (int s : o.sources) {
            auto it = sources.find(s);
            if (it == sources.end()) { error = std::string(byChannel ? "channel " : "track ") + std::to_string(s) + " has no notes"; return ""; }
            use.push_back(&it->second);
        }
    } else {
        for (auto& [n, s] : sources) if ((int)use.size() < o.voices) use.push_back(&s);
    }
    if (use.empty()) { error = "no notes in the MIDI file"; return ""; }
    // monophonic, on the grid: the newest of overlapping notes wins
    const long grid = o.ticksPerBeat;
    auto q = [&](long t) { return (long)std::lround((double)t * grid / division); };
    long barTicks = std::max(1L, (long)sigNum * 4 * grid / std::max(1, sigDen));
    std::vector<std::vector<Note>> voices;
    long total = 0;
    for (Source* s : use) {
        std::sort(s->notes.begin(), s->notes.end(), [](const Note& a, const Note& b) { return a.start < b.start; });
        std::vector<Note> v;
        for (const Note& n : s->notes) {
            Note x{q(n.start), q(n.end), n.pitch + o.transpose, n.program};
            if (x.pitch < 0 || x.pitch > 127) continue;
            if (!v.empty() && v.back().end > x.start) v.back().end = x.start;   // cut by the new one
            if (!v.empty() && v.back().end <= v.back().start) v.pop_back();     // (nothing left of it)
            if (x.end <= x.start) x.end = x.start + 1;
            v.push_back(x);
        }
        if (!v.empty()) total = std::max(total, v.back().end);
        voices.push_back(v);
    }
    total = (total + barTicks - 1) / barTicks * barTicks;

    // instruments: given (gm<program> for a program, voice<n> for a voice), else three of ours
    std::string text;
    std::ostringstream t;
    double bpm = 60000000.0 / (double)tempo;
    char fpt[16];
    snprintf(fpt, sizeof fpt, "%.2f", 3000.0 / (bpm * grid));
    t << "; made by tune45 import: " << use.size() << (byChannel ? " channels" : " tracks") << " (";
    for (size_t k = 0; k < use.size(); k++) {
        t << (k ? ", " : "") << use[k]->number;
        auto nm = names.find(use[k]->number);
        if (!byChannel && nm != names.end()) t << " " << nm->second;
    }
    t << "), " << (int)std::lround(bpm) << " bpm, " << sigNum << "/" << sigDen << "\n";
    std::map<std::string, bool> have;
    if (!o.instruments.empty()) {
        t << o.instruments << (o.instruments.back() == '\n' ? "" : "\n");
        std::istringstream in(o.instruments);
        std::string line;
        while (std::getline(in, line)) {
            std::istringstream w(line);
            std::string kw, name;
            w >> kw >> name;
            if (kw == "instrument") have[name] = true;
        }
    }
    static const char* defaults[3] = {
        "pulse 09 a6 0600 1 0", "tri   0a 00 0800 0 1", "saw   26 96 0800 1 0"};
    for (size_t k = 0; k < voices.size(); k++) {
        std::string vn = "voice" + std::to_string(k + 1);
        if (!have[vn]) { t << "instrument " << vn << " " << defaults[k % 3] << "\n"; have[vn] = true; }
    }
    t << "\ntune " << o.name << " " << fpt << " " << (o.loop ? "loop" : "once") << " " << barTicks << "\n";
    for (size_t k = 0; k < voices.size(); k++) {
        t << "v" << k + 1 << ": @voice" << k + 1;
        long cursor = 0;
        int program = -1;
        // a run of ticks: split at the bar lines (a note goes on as ~)
        auto emit = [&](const std::string& what, long from, long len) {
            std::string tok = what;
            while (len > 0) {
                long room = barTicks - from % barTicks;
                long part = std::min(len, room);
                t << " " << tok << ":" << part;
                from += part;
                len -= part;
                if (from % barTicks == 0 && from < total) t << " |";
                if (tok != "r") tok = "~";
            }
        };
        for (const Note& n : voices[k]) {
            if (n.start > cursor) emit("r", cursor, n.start - cursor);
            std::string gm = "gm" + std::to_string(n.program);
            if (n.program != program && have[gm]) t << " @" << gm;
            program = n.program;
            emit(TuneCompiler::noteName(n.pitch), n.start, n.end - n.start);
            cursor = n.end;
        }
        if (cursor < total) emit("r", cursor, total - cursor);
        t << "\n";
    }
    return t.str();
}
