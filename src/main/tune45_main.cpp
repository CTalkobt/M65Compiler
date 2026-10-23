// tune45 — music for the tune player (<audio_tune.h>)
//
//   tune45 convert music.txt -o music.c     C source and header (or --bin, --asm)
//   tune45 list music.txt                   each tune's size and length
// (play, create and import: see doc/bin/tune45.md)
#include "TuneCompiler.hpp"
#include "Version.hpp"
#include <cctype>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>

namespace {

void usage(std::ostream& o) {
    o << "Usage: tune45 <command> [options] <music.txt>\n"
         "Commands:\n"
         "  convert   the player's data: -o music.c (C source and music.h), --bin -o music.bin\n"
         "            (one blob for far memory; music.h gives each tune's offset), --asm -o music.s45\n"
         "  list      each tune: voices, bytes, ticks, seconds (PAL / NTSC), instruments\n"
         "Options:\n"
         "  -o <file>          output file\n"
         "  --bin, --asm       convert: a binary blob / ca45 source instead of C\n"
         "  --header <file>    convert: the header's name (default: the output's, .h)\n"
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
    for (int i = 2; i < argc; i++) {
        std::string a = argv[i];
        if (a == "-o" && i + 1 < argc) out = argv[++i];
        else if (a == "--bin") mode = "bin";
        else if (a == "--asm") mode = "asm";
        else if (a == "--header" && i + 1 < argc) header = argv[++i];
        else if (a == "-h" || a == "--help") { usage(std::cout); return 0; }
        else if (!a.empty() && a[0] == '-') { std::cerr << "tune45: unknown option " << a << "\n"; return 1; }
        else input = a;
    }
    if (cmd != "convert" && cmd != "list") { std::cerr << "tune45: unknown command " << cmd << "\n"; usage(std::cerr); return 1; }
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
    if (out.empty()) out = stem(input) + (mode == "bin" ? ".bin" : mode == "asm" ? ".s45" : ".c");
    return convert(f, input, out, mode, header);
}
