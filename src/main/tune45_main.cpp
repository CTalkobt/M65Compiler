// tune45 — music for the tune player (<audio_tune.h>)
//
//   tune45 convert music.txt -o music.c     C source and header (or --bin, --asm)
//   tune45 list music.txt                   each tune's size and length
//   tune45 play music.txt [--tune name]     on the PC (reSIDfp), or a .wav
// (create and import: see doc/bin/tune45.md)
#include "TuneCompiler.hpp"
#include "TunePlayer.hpp"
#include "TuneAudio.hpp"
#include "BasicTooling.hpp"
#include <csignal>
#include <climits>
#include <sys/stat.h>
#include <sys/wait.h>
#include <thread>
#include <chrono>
#include "Version.hpp"
#include <cctype>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstdlib>

namespace {

void usage(std::ostream& o) {
    o << "Usage: tune45 <command> [options] <music.txt>\n"
         "Commands:\n"
         "  convert   the player's data: -o music.c (C source and music.h), --bin -o music.bin\n"
         "            (one blob for far memory; music.h gives each tune's offset), --asm -o music.s45\n"
         "  list      each tune: voices, bytes, ticks, seconds (PAL / NTSC), instruments\n"
         "  play      play the tunes on the PC (reSIDfp: four SIDs, stereo) through pw-play,\n"
         "            paplay or aplay, or into a .wav file\n"
         "Options:\n"
         "  -o <file>          output file\n"
         "  --bin, --asm       convert: a binary blob / ca45 source instead of C\n"
         "  --header <file>    convert: the header's name (default: the output's, .h)\n"
         "  --tune <name>      play: that tune (again for more; default: all, in order)\n"
         "  --loops <n>        play: times through a looping tune (default 1)\n"
         "  --wav <file>       play: into a WAV file instead of the speakers\n"
         "  --ntsc             play: an NTSC machine (60 Hz, its SID clock)\n"
         "  --6581             play: the MOS 6581 SID (default: the 8580)\n"
         "  --mono             play: SIDs 1-2 only (without the mirror on SIDs 3-4)\n"
         "  --emulator         play: on the MEGA65 in xemu (a player program with the tunes)\n"
         "  --emulator-path <f> the xemu-xmega65 binary (default: on PATH)\n"
         "  --send             play: on a MEGA65 (etherload -r, else m65 -F -r)\n"
         "  --send-cmd <cmd>   the command for --send; {prg} is the program\n"
         "  --watch            play: again whenever the text file changes (Ctrl-C stops)\n"
         "  --prg <file>       play: keep the player program (--emulator / --send) there\n"
         "  -V, --version      version\n"
         "  -h, --help         this help\n"
         "The text format: doc/bin/tune45.md (pfest's music.txt format).\n";
}

bool readFile(const std::string& path, std::string& out) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    std::stringstream ss;
    ss << in.rdbuf();
    out = ss.str();
    return true;
}

std::string stem(const std::string& path) {
    size_t slash = path.find_last_of('/');
    size_t dot = path.find_last_of('.');
    return dot != std::string::npos && (slash == std::string::npos || dot > slash) ? path.substr(0, dot) : path;
}

std::string baseName(const std::string& path) {
    size_t slash = path.find_last_of('/');
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

std::string upper(std::string s) {
    for (auto& c : s) c = (char)std::toupper((unsigned char)c);
    return s;
}

bool isIdent(const std::string& s) {
    if (s.empty() || std::isdigit((unsigned char)s[0])) return false;
    for (char c : s) if (!std::isalnum((unsigned char)c) && c != '_') return false;
    return true;
}

// 16 a line: lead starts a line, sep separates, lineEnd ends all but the last
std::string bytes(const std::vector<uint8_t>& b, const char* lead, const char* sep, const char* lineEnd) {
    std::string s;
    char buf[8];
    for (size_t k = 0; k < b.size(); k++) {
        if (k % 16 == 0) s += (k ? std::string(lineEnd) + "\n" : "") + std::string(lead);
        snprintf(buf, sizeof buf, "%s%u", k % 16 ? sep : "", b[k]);
        s += buf;
    }
    return s;
}

int convert(const TuneFile& f, const std::string& src, const std::string& outPath, const std::string& mode,
            std::string header) {
    for (const auto& t : f.tunes)
        if (!isIdent(t.name)) {
            std::cerr << src << ":" << t.line << ": error: a tune's name must be a C identifier: " << t.name << "\n";
            return 1;
        }
    if (header.empty()) header = stem(outPath) + ".h";
    std::string guardNote = "/* made by tune45 from " + baseName(src) + " - do not edit */\n";
    std::ofstream h(header);
    if (!h) { std::cerr << "tune45: cannot write " << header << "\n"; return 1; }
    h << guardNote << "#pragma once\n\n#define NTUNES " << f.tunes.size() << "\n";
    for (size_t k = 0; k < f.tunes.size(); k++) h << "#define TUNE_" << upper(f.tunes[k].name) << " " << k << "\n";

    if (mode == "bin") {
        // [tunes][offset (2) each] then the blobs; offsets from the start
        std::vector<uint8_t> out(1 + 2 * f.tunes.size(), 0);
        out[0] = (uint8_t)f.tunes.size();
        h << "\n/* " << baseName(outPath) << ": tune_play_far(<its address> + TUNE_<NAME>_AT) */\n";
        for (size_t k = 0; k < f.tunes.size(); k++) {
            size_t at = out.size();
            out[1 + 2 * k] = (uint8_t)(at & 0xFF);
            out[2 + 2 * k] = (uint8_t)(at >> 8);
            auto b = TuneCompiler::blob(f, f.tunes[k]);
            out.insert(out.end(), b.begin(), b.end());
            char buf[16];
            snprintf(buf, sizeof buf, "0x%04zx", at);
            h << "#define TUNE_" << upper(f.tunes[k].name) << "_AT " << buf << "\n";
        }
        if (out.size() > 0xFFFF) { std::cerr << "tune45: over 64 KB of tunes\n"; return 1; }
        h << "#define TUNES_SIZE " << out.size() << "\n";
        std::ofstream o(outPath, std::ios::binary);
        o.write((const char*)out.data(), (std::streamsize)out.size());
        if (!o) { std::cerr << "tune45: cannot write " << outPath << "\n"; return 1; }
        return 0;
    }
    h << "\n";
    for (const auto& t : f.tunes)
        h << "extern const unsigned char tune_" << t.name << "[" << TuneCompiler::blob(f, t).size() << "];\n";
    h << "extern const unsigned char *const tunes[NTUNES];   /* tune_play(tunes[TUNE_...]) */\n";
    std::ofstream o(outPath);
    if (!o) { std::cerr << "tune45: cannot write " << outPath << "\n"; return 1; }
    if (mode == "asm") {
        o << "; made by tune45 from " << baseName(src) << " - do not edit\n\n.segment \"data\"\n";
        for (const auto& t : f.tunes) o << ".global _tune_" << t.name << "\n";
        o << ".global _tunes\n";
        for (const auto& t : f.tunes) o << "\n_tune_" << t.name << ":\n" << bytes(TuneCompiler::blob(f, t), "    .byte ", ", ", "") << "\n";
        o << "\n_tunes:\n";
        for (const auto& t : f.tunes) o << "    .word _tune_" << t.name << "\n";
    } else {
        o << guardNote << "#include \"" << baseName(header) << "\"\n";
        for (const auto& t : f.tunes) {
            auto b = TuneCompiler::blob(f, t);
            o << "\nconst unsigned char tune_" << t.name << "[" << b.size() << "] = {\n" << bytes(b, "    ", ", ", ",") << ",\n};\n";
        }
        o << "\nconst unsigned char *const tunes[NTUNES] = {";
        for (size_t k = 0; k < f.tunes.size(); k++) o << (k ? ", " : " ") << "tune_" << f.tunes[k].name;
        o << " };\n";
    }
    return o ? 0 : 1;
}

time_t mtime(const std::string& p);

struct PlayOptions {
    std::vector<std::string> tunes;
    unsigned loops = 1, frames = 0;
    std::string wav, emulatorPath, sendCmd, prg;
    bool ntsc = false, mos6581 = false, mono = false, emulator = false, send = false, watch = false;
};

// The tunes named (all when none), or an error
bool chosen(const TuneFile& f, const PlayOptions& o, std::vector<const Tune*>& out) {
    if (o.tunes.empty()) { for (const auto& t : f.tunes) out.push_back(&t); return true; }
    for (const auto& n : o.tunes) {
        const Tune* found = nullptr;
        for (const auto& t : f.tunes) if (t.name == n) found = &t;
        if (!found) { std::cerr << "tune45: no tune " << n << "\n"; return false; }
        out.push_back(found);
    }
    return true;
}

int play(const std::string& src, TuneFile f, const PlayOptions& o) {
    std::vector<const Tune*> list;
    if (!chosen(f, o, list)) return 1;
    time_t seen = mtime(src);
    TuneAudio audio(o.ntsc, o.mos6581);
    TunePlayer player([&](uint8_t i, uint8_t v) { audio.write(i, v); });
    player.setNtsc(o.ntsc);
    player.setMirror(!o.mono);
    // the volume up (as sid_init: the player leaves it to the program)
    for (uint8_t c = 0; c < 4; c++) audio.write((uint8_t)(c * 0x20 + 0x18), 15);
    AudioSink sink;
    if (!sink.open(o.wav, audio.rate())) { std::cerr << "tune45: " << sink.error() << "\n"; return 1; }
    const double hz = o.ntsc ? 60.0 : 50.0;
    std::vector<int16_t> buf;
    for (size_t ti = 0; ti < list.size() || (o.watch && (ti = 0, !list.empty())); ti++) {
        const Tune* t = list[ti];
        unsigned ticks = t->voices[0].ticks;
        double frames = ticks * (t->fpt / 256.0) * (o.ntsc ? 60.0 / 50.0 : 1.0);
        unsigned total = t->loop ? (unsigned)(frames * o.loops + 0.5) : 0;
        char line[160];
        snprintf(line, sizeof line, "playing %s: %.1f s%s (via %s)\n", t->name.c_str(), frames / hz,
                 t->loop ? (o.loops > 1 ? ", looping" : ", once round") : "", sink.via().c_str());
        std::cout << line << std::flush;
        player.play(TuneCompiler::blob(f, *t));
        bool changed = false;
        for (unsigned n = 0; (t->loop ? n < total || o.watch : player.playing()); n++) {
            player.update();
            audio.frame(buf);
            if (buf.size() >= 4096) { sink.put(buf); buf.clear(); }
            // --watch: the text changed: the tunes again, from the start
            if (o.watch && n % 25 == 0 && mtime(src) != seen) { changed = true; break; }
        }
        if (changed) {
            seen = mtime(src);
            std::string text;
            TuneCompiler c;
            TuneFile nf;
            if (readFile(src, text) && c.parse(text, src, nf)) {
                f = nf;
                list.clear();
                if (!chosen(f, o, list)) return 1;
                std::cout << "tune45: " << src << " changed\n";
            } else {
                for (const auto& e : c.errors()) std::cerr << e << "\n";
            }
            player.stop();
            buf.clear();
            ti = (size_t)-1;   /* (from the first tune again) */
            continue;
        }
        player.stop();
        for (unsigned n = 0; n < (unsigned)hz; n++) audio.frame(buf);   // a second for the release
        sink.put(buf);
        buf.clear();
    }
    sink.close();
    return 0;
}

// The model's registers after each frame (the tests compare them with the
// real player's): a line a frame, 128 bytes in hex
int trace(const TuneFile& f, const PlayOptions& o) {
    std::vector<const Tune*> list;
    if (!chosen(f, o, list) || list.empty()) return 1;
    uint8_t regs[128] = {};
    TunePlayer player([&](uint8_t i, uint8_t v) { regs[i & 0x7F] = v; });
    player.setNtsc(o.ntsc);
    player.setMirror(!o.mono);
    player.play(TuneCompiler::blob(f, *list[0]));
    for (unsigned n = 0; n < (o.frames ? o.frames : 100); n++) {
        player.update();
        for (uint8_t b : regs) { char h[4]; snprintf(h, sizeof h, "%02x", b); std::cout << h; }
        std::cout << "\n";
    }
    return 0;
}

// The directory tune45 is in (cc45, ln45 are next to it)
std::string toolDir() {
    char buf[PATH_MAX];
    ssize_t n = readlink("/proc/self/exe", buf, sizeof buf - 1);
    if (n <= 0) return "";
    buf[n] = 0;
    std::string p = buf;
    return p.substr(0, p.find_last_of('/') + 1);
}

bool exists(const std::string& p) { struct stat st; return stat(p.c_str(), &st) == 0; }

time_t mtime(const std::string& p) { struct stat st; return stat(p.c_str(), &st) == 0 ? st.st_mtime : 0; }

std::string quote(const std::string& s) {
    std::string q = "'";
    for (char c : s) { if (c == '\'') q += "'\\''"; else q += c; }
    return q + "'";
}

// The player program (lib tuneplay.o45) with the tunes: prg, or "" with a message
std::string buildPlayer(const TuneFile& f, const std::string& src, const PlayOptions& o, const std::string& prg) {
    std::string bin = toolDir(), lib;
    for (const char* d : {"../lib/build/", "../lib/cc45/"})
        if (exists(bin + d + "tuneplay.o45")) { lib = bin + d; break; }
    if (lib.empty()) { std::cerr << "tune45: no tuneplay.o45 (lib/build or lib/cc45 next to " << bin << ")\n"; return ""; }
    char tmpl[] = "/tmp/tune45-XXXXXX";
    const char* tmp = mkdtemp(tmpl);
    if (!tmp) { std::cerr << "tune45: no temporary directory\n"; return ""; }
    std::string dir = tmp, c = dir + "/tunes.c", obj = dir + "/tunes.o45";
    {
        std::ofstream out(c);
        unsigned start = 0;
        if (!o.tunes.empty())
            for (size_t k = 0; k < f.tunes.size(); k++) if (f.tunes[k].name == o.tunes[0]) start = (unsigned)k;
        out << "/* made by tune45 play */\n";
        for (const auto& t : f.tunes) {
            auto b = TuneCompiler::blob(f, t);
            out << "static const unsigned char tune_" << t.name << "[" << b.size() << "] = {\n" << bytes(b, "    ", ", ", ",") << ",\n};\n";
        }
        out << "const unsigned char *const tunes[" << f.tunes.size() << "] = {";
        for (size_t k = 0; k < f.tunes.size(); k++) out << (k ? ", " : " ") << "tune_" << f.tunes[k].name;
        out << " };\nconst char *const tune_names[" << f.tunes.size() << "] = {";
        for (size_t k = 0; k < f.tunes.size(); k++) out << (k ? ", " : " ") << "\"" << f.tunes[k].name << "\"";
        out << " };\nconst unsigned char tune_count = " << f.tunes.size() << ";\nconst unsigned char tune_start = " << start
            << ";\nconst char tune_file[] = \"" << baseName(src) << "\";\n";
    }
    std::string out = prg.empty() ? dir + "/tuneplay.prg" : prg;
    std::string cmd = quote(bin + "cc45") + " -O2 -c " + quote(c) + " -o " + quote(obj) + " && " +
                      quote(bin + "ln45") + " -basic -o " + quote(out) + " " + quote(lib + "tuneplay.o45") + " " + quote(obj) +
                      " " + quote(lib + "c45.lib") + " >/dev/null";
    int rc = system(cmd.c_str());
    remove(c.c_str());
    remove(obj.c_str());
    if (rc != 0) { std::cerr << "tune45: building the player program failed\n"; rmdir(dir.c_str()); return ""; }
    return out;
}

// --emulator / --send: the player program to xemu or a MEGA65; with --watch
// again whenever src changes
int playOnMega65(const std::string& src, TuneFile f, const PlayOptions& o) {
    std::string prg = buildPlayer(f, src, o, o.prg);
    if (prg.empty()) return 1;
    auto send = [&](const std::string& p) {
        std::string cmd = o.sendCmd;
        if (cmd.empty()) {
            if (!EmulatorLauncher::findOnPath("etherload").empty()) cmd = "etherload -r {prg}";
            else if (!EmulatorLauncher::findOnPath("m65").empty()) cmd = "m65 -F -r {prg}";
            else { std::cerr << "tune45: neither etherload nor m65 is on PATH (--send-cmd)\n"; return false; }
        }
        size_t at = cmd.find("{prg}");
        if (at != std::string::npos) cmd.replace(at, 5, quote(p));
        else cmd += " " + quote(p);
        std::cout << "Sending: " << cmd << "\n";
        return system(cmd.c_str()) == 0;
    };
    if (!o.watch) {
        bool ok = o.send ? send(prg) : EmulatorLauncher().launch(prg, o.emulatorPath);
        if (o.prg.empty()) { std::string d = prg.substr(0, prg.find_last_of('/')); remove(prg.c_str()); rmdir(d.c_str()); }
        return ok ? 0 : 1;
    }
    int pid = -1;
    time_t seen = mtime(src);
    for (;;) {
        if (!prg.empty()) {
            if (o.send) send(prg);
            else {
                if (pid > 0) { kill(pid, SIGTERM); waitpid(pid, nullptr, 0); }
                pid = EmulatorLauncher::launchAsync(prg, o.emulatorPath);
            }
        }
        while (mtime(src) == seen) std::this_thread::sleep_for(std::chrono::milliseconds(500));
        seen = mtime(src);
        std::string text;
        TuneCompiler c;
        if (!readFile(src, text) || !c.parse(text, src, f)) {
            for (const auto& e : c.errors()) std::cerr << e << "\n";
            prg.clear();
            continue;
        }
        prg = buildPlayer(f, src, o, o.prg.empty() ? prg : o.prg);
        std::cout << "tune45: " << src << " changed\n";
    }
}

int list(const TuneFile& f) {
    size_t total = 0;
    for (const auto& t : f.tunes) {
        auto b = TuneCompiler::blob(f, t);
        total += b.size();
        unsigned ticks = t.voices.empty() ? 0 : t.voices[0].ticks;
        double sec = ticks * (t.fpt / 256.0) / 50.0;   // PAL frames; NTSC plays as long
        char buf[160];
        snprintf(buf, sizeof buf, "%-12s %2zu voices %5zu bytes  %5u ticks  %6.1f s  %.2f frames/tick  %s\n",
                 t.name.c_str(), t.voices.size(), b.size(), ticks, sec, t.fpt / 256.0, t.loop ? "loop" : "once");
        std::cout << buf;
        std::cout << "             voices:";
        for (size_t v = 0; v < t.voices.size(); v++) std::cout << " v" << v + 1 << " " << t.voices[v].stream.size() + 1;
        std::cout << " bytes\n             instruments:";
        for (int i : TuneCompiler::usedInstruments(f, t)) std::cout << " " << f.instruments[(size_t)i].name;
        std::cout << "\n";
    }
    std::cout << f.tunes.size() << " tunes, " << total << " bytes\n";
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) { usage(std::cerr); return 1; }
    std::string cmd = argv[1];
    if (cmd == "-h" || cmd == "--help") { usage(std::cout); return 0; }
    if (cmd == "-V" || cmd == "--version") { std::cout << "tune45 " << SUITE_VERSION << "\n"; return 0; }
    std::string out, mode = "c", header, input;
    PlayOptions po;
    for (int i = 2; i < argc; i++) {
        std::string a = argv[i];
        if (a == "-o" && i + 1 < argc) out = argv[++i];
        else if (a == "--tune" && i + 1 < argc) po.tunes.push_back(argv[++i]);
        else if (a == "--loops" && i + 1 < argc) po.loops = (unsigned)std::max(1, atoi(argv[++i]));
        else if (a == "--frames" && i + 1 < argc) po.frames = (unsigned)std::max(1, atoi(argv[++i]));
        else if (a == "--wav" && i + 1 < argc) po.wav = argv[++i];
        else if (a == "--ntsc") po.ntsc = true;
        else if (a == "--6581") po.mos6581 = true;
        else if (a == "--mono") po.mono = true;
        else if (a == "--emulator") po.emulator = true;
        else if (a == "--emulator-path" && i + 1 < argc) { po.emulator = true; po.emulatorPath = argv[++i]; }
        else if (a == "--send") po.send = true;
        else if (a == "--send-cmd" && i + 1 < argc) { po.send = true; po.sendCmd = argv[++i]; }
        else if (a == "--watch") po.watch = true;
        else if (a == "--prg" && i + 1 < argc) po.prg = argv[++i];
        else if (a == "--bin") mode = "bin";
        else if (a == "--asm") mode = "asm";
        else if (a == "--header" && i + 1 < argc) header = argv[++i];
        else if (a == "-h" || a == "--help") { usage(std::cout); return 0; }
        else if (!a.empty() && a[0] == '-') { std::cerr << "tune45: unknown option " << a << "\n"; return 1; }
        else input = a;
    }
    if (cmd != "convert" && cmd != "list" && cmd != "play" && cmd != "trace") { std::cerr << "tune45: unknown command " << cmd << "\n"; usage(std::cerr); return 1; }
    if (input.empty()) { std::cerr << "tune45: no input file\n"; return 1; }
    std::string text;
    if (!readFile(input, text)) { std::cerr << "tune45: cannot read " << input << "\n"; return 1; }
    TuneFile f;
    TuneCompiler c;
    if (!c.parse(text, input, f)) {
        for (const auto& e : c.errors()) std::cerr << e << "\n";
        return 1;
    }
    if (cmd == "list") return list(f);
    if (cmd == "play") return po.emulator || po.send ? playOnMega65(input, f, po) : play(input, f, po);
    if (cmd == "trace") return trace(f, po);
    if (out.empty()) out = stem(input) + (mode == "bin" ? ".bin" : mode == "asm" ? ".s45" : ".c");
    return convert(f, input, out, mode, header);
}
