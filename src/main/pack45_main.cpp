// pack45 — packs a file into a format-1 stream (#345, Pack45.hpp) for the
// library's unpack() (<unpack.h>): its load address (cbm_load / cbm_load_far
// skip it), then the stream. Unpacks one (-x) to check it.
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <iterator>
#include "Logger.hpp"
#include "Pack45.hpp"
#include "PackAudio.hpp"
#include "Version.hpp"

static bool readFile(const std::string& path, std::vector<uint8_t>& out) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) return false;
    out.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    return true;
}

static bool writeFile(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) return false;
    out.write(reinterpret_cast<const char*>(data.data()), (std::streamsize)data.size());
    return (bool)out;
}

static bool parseAddr(const std::string& s, uint32_t& v) {
    try {
        size_t used = 0;
        std::string t = s;
        int base = 10;
        if (!t.empty() && t[0] == '$') { t = t.substr(1); base = 16; }
        else if (t.size() > 2 && t[0] == '0' && (t[1] == 'x' || t[1] == 'X')) { t = t.substr(2); base = 16; }
        unsigned long x = std::stoul(t, &used, base);
        if (used != t.size() || x > 0xFFFFFFFUL) return false;
        v = (uint32_t)x;
        return true;
    } catch (...) {
        return false;
    }
}

static void usage() {
    std::cout << "Usage: pack45 [options] <input> <output>\n"
                 "       pack45 -x <packed> <output>\n"
                 "       pack45 --suggest-rate [--keep PCT] [--max-rate HZ] <sound.wav> ...\n"
                 "Packs <input> into a stream unpack() (<unpack.h>) unpacks, written with a\n"
                 "two-byte load address in front (cbm_load / cbm_load_far skip it).\n"
                 "  -d, --dest ADDR   where it unpacks to: a 28-bit address ($40000, 0x8000000);\n"
                 "                    by default a .prg input's load address (its two bytes left out)\n"
                 "  --load ADDR       the load address written (default: dest + the in-place\n"
                 "                    offset, so it can be loaded and unpacked in place)\n"
                 "  --raw             no load address: the stream alone\n"
                 "  --best            the shortest stream (an optimal parse; slower)\n"
                 "  --prg             the input has a two-byte load address (also: a .prg name)\n"
                 "  -x                unpack <packed> (with its load address, or --raw) to <output>\n"
                 "  --suggest-rate    for each WAV: the lowest sample rate that keeps PCT of its\n"
                 "                    energy (fewer samples is what makes a sound smaller)\n"
                 "  --keep PCT        the energy kept, per cent (default 99.9)\n"
                 "  --max-rate HZ     the highest rate it will be played at: only the energy\n"
                 "                    that rate can hold counts, and no rate above it is suggested\n"
                 "  -V, --version     the version\n"
                 "  -v ... -vvvv      log to stderr\n"
                 "Prints the sizes, where it unpacks to and the in-place load address.\n";
}

// --suggest-rate: each sound's energy below which keep per cent lies, and
// the lowest usual rate whose pass band holds it; the bytes at 8 bits a
// sample, and the MEGA65 audio DMA's rate for it (Hz * 2^24 /
// 40.5 MHz)
static int suggestRates(const std::vector<std::string>& files, double keep, uint32_t maxRate) {
    if (files.empty()) { usage(); return 1; }
    int status = 0;
    for (const auto& f : files) {
        std::vector<uint8_t> in;
        pack45::Wav w;
        std::string err;
        if (!readFile(f, in)) { std::cerr << "pack45: cannot read " << f << "\n"; status = 1; continue; }
        if (!pack45::readWav(in, w, &err)) { std::cerr << "pack45: " << f << ": " << err << "\n"; status = 1; continue; }
        uint32_t top = maxRate && maxRate < w.rate ? maxRate : w.rate;
        double hz = pack45::energyBelow(w, keep / 100, top < w.rate ? 0.45 * top : 0);
        uint32_t r = pack45::rateFor(hz, top);
        double secs = (double)w.samples.size() / w.rate;
        char line[512];
        snprintf(line, sizeof line,
                 "pack45: %s: %u Hz, %d-bit, %d channel%s, %.2f s; %g%% of its energy below %.0f Hz: "
                 "%u Hz keeps it (%.0f bytes at 8 bits, %.0f%% of %u Hz); audio DMA rate $%06lX",
                 f.c_str(), (unsigned)w.rate, w.bits, w.channels, w.channels == 1 ? "" : "s", secs, keep, hz,
                 (unsigned)r, secs * r, 100.0 * r / w.rate, (unsigned)w.rate,
                 (unsigned long)std::lround((double)r * (1 << 24) / 40500000.0));
        std::cout << line << std::endl;
    }
    return status;
}

int main(int argc, char** argv) {
    Logger::setTool("pack45");
    argc = Logger::takeFlags(argc, argv);
    uint32_t dest = 0, load = 0;
    bool haveDest = false, haveLoad = false, raw = false, prg = false, extract = false, best = false, suggest = false;
    double keep = 99.9;
    uint32_t maxRate = 0;
    std::vector<std::string> files;
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        auto value = [&](uint32_t& v) {
            if (i + 1 >= argc || !parseAddr(argv[i + 1], v)) {
                std::cerr << "pack45: " << a << " needs an address (a 28-bit number: $40000, 0x8000000, 262144)\n";
                exit(1);
            }
            i++;
        };
        if (a == "-?" || a == "-h" || a == "--help") { usage(); return 0; }
        else if (a == "-V" || a == "--version") { std::cout << suiteVersionString("pack45") << std::endl; return 0; }
        else if (a == "-d" || a == "--dest") { value(dest); haveDest = true; }
        else if (a == "--load") { value(load); haveLoad = true; }
        else if (a == "--raw") raw = true;
        else if (a == "--best") best = true;
        else if (a == "--prg") prg = true;
        else if (a == "-x") extract = true;
        else if (a == "--suggest-rate") suggest = true;
        else if (a == "--max-rate") { value(maxRate); if (!maxRate) { std::cerr << "pack45: --max-rate needs a rate in Hz\n"; return 1; } }
        else if (a == "--keep") {
            char* end = nullptr;
            keep = i + 1 < argc ? strtod(argv[i + 1], &end) : 0;
            if (!end || *end || keep <= 0 || keep > 100) {
                std::cerr << "pack45: --keep needs a per cent (0 < PCT <= 100: 99.9)\n";
                return 1;
            }
            i++;
        }
        else if (!a.empty() && a[0] == '-' && a.size() > 1) { std::cerr << "pack45: unknown option " << a << "\n"; return 1; }
        else files.push_back(a);
    }
    if (suggest) return suggestRates(files, keep, maxRate);
    if (files.size() != 2) { usage(); return 1; }
    std::vector<uint8_t> in;
    if (!readFile(files[0], in)) { std::cerr << "pack45: cannot read " << files[0] << "\n"; return 1; }

    if (extract) {
        std::vector<uint8_t> s(in.begin() + (raw || in.size() < 2 ? 0 : 2), in.end());
        std::vector<uint8_t> out;
        uint32_t d = 0;
        std::string err;
        if (!pack45::unpack(s, out, &d, &err)) { std::cerr << "pack45: " << files[0] << ": " << err << "\n"; return 1; }
        if (!writeFile(files[1], out)) { std::cerr << "pack45: cannot write " << files[1] << "\n"; return 1; }
        std::cout << "pack45: " << files[0] << ": " << out.size() << " bytes, unpacking to $" << std::hex
                  << std::uppercase << d << std::dec << std::endl;
        return 0;
    }

    std::string lower = files[0];
    for (auto& c : lower) c = (char)tolower((unsigned char)c);
    if (lower.size() > 4 && lower.substr(lower.size() - 4) == ".prg") prg = true;
    if (prg) {
        if (in.size() < 2) { std::cerr << "pack45: " << files[0] << ": no load address\n"; return 1; }
        if (!haveDest) { dest = in[0] | (in[1] << 8); haveDest = true; }
        in.erase(in.begin(), in.begin() + 2);
    }
    if (!haveDest) { std::cerr << "pack45: where does it unpack to? -d ADDR (or a .prg input)\n"; return 1; }
    pack45::Packed p = pack45::pack(in, dest, best);
    LOG_INFO("pack45", files[0] << ": in-place offset " << p.inPlace);
    // (the packer's own check: the model unpacks it, in place too)
    std::vector<uint8_t> back;
    if (!pack45::unpack(p.stream, back, nullptr) || back != in || !pack45::checkInPlace(in, p, p.inPlace)) {
        std::cerr << "pack45: internal error: the stream does not unpack to the input\n";
        return 2;
    }
    if (!haveLoad) load = dest + p.inPlace;
    std::vector<uint8_t> out;
    if (!raw) { out.push_back((uint8_t)load); out.push_back((uint8_t)(load >> 8)); }
    out.insert(out.end(), p.stream.begin(), p.stream.end());
    if (!writeFile(files[1], out)) { std::cerr << "pack45: cannot write " << files[1] << "\n"; return 1; }
    char line[256];
    snprintf(line, sizeof line, "pack45: %s: %zu -> %zu bytes (%zu%%), unpacks to $%X; in place: load it at $%X (offset %u)",
             files[0].c_str(), in.size(), p.stream.size(), in.empty() ? (size_t)0 : p.stream.size() * 100 / in.size(),
             (unsigned)dest, (unsigned)(dest + p.inPlace), (unsigned)p.inPlace);
    std::cout << line << std::endl;
    if (haveLoad && !raw && (load & 0xFFFF0000u) != 0 && load < dest + p.inPlace && load + p.stream.size() > dest)
        std::cerr << "pack45: warning: loaded at $" << std::hex << std::uppercase << load << std::dec
                  << ", it is not far enough into the area to unpack in place\n";
    return 0;
}
