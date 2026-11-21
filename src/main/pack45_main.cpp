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
#include <algorithm>
#include <cstdio>
#include "Font45.hpp"
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
                 "       pack45 --suggest-rate [--keep PCT] [--max-rate HZ] <sound> ...\n"
                 "       pack45 --sound [--rate HZ] [--unsigned] [--normalise] [options] <sound> <output>\n"
                 "       pack45 --image [--width W] [--palette FILE] [--palette-out FILE] [options] <image> <output>\n"
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
                 "  --suggest-rate    for each sound: the lowest sample rate that keeps PCT of its\n"
                 "                    energy (fewer samples is what makes a sound smaller)\n"
                 "  --sound           <sound> to 8-bit samples for the audio DMA, at --rate or the\n"
                 "                    rate --suggest-rate names; with -d packed as any file, else\n"
                 "                    the samples alone (--load ADDR: that load address in front)\n"
                 "  --rate HZ         the samples' rate (--sound)\n"
                 "  --unsigned        unsigned samples (offset by 128) instead of signed\n"
                 "  --normalise       the loudest sample to full scale\n"
                 "  --keep PCT        the energy kept, per cent (default 99.9)\n"
                 "  --max-rate HZ     the highest rate it will be played at: only the energy\n"
                 "                    that rate can hold counts, and no rate above it is suggested\n"
                 "  --image           <image> (up to 16 colours) as format 2: each pixel coded\n"
                 "                    against the one above or to its left (the smaller kept),\n"
                 "                    unpacking to 4-bit pixels (the left one the low nibble)\n"
                 "  --width W         the image is raw 4-bit pixels, W (even, 2-256) a row\n"
                 "  --palette FILE    R, G, B bytes of the palette's entries: an image's colours\n"
                 "                    are found in it, and the order is dark to light by it\n"
                 "  --palette-out FILE  without --palette: the image's colours (R, G, B, by\n"
                 "                    entry: they are numbered dark to light) written there\n"
                 "  --predict above|left  that prediction, not the smaller of the two\n"
                 "  -V, --version     the version\n"
                 "  -v ... -vvvv      log to stderr\n"
                 "Prints the sizes, where it unpacks to and the in-place load address.\n"
                 "Sounds: WAV, AU and MP3 read here; MP4, M4A and others through ffmpeg.\n";
}

// --image (#666): the image's pixels as palette entries 0-15 and the order of
// its colours, dark to light (by the palette when there is one); w x h.
// A PNG (or any image stb_image reads) without --palette: its colours
// numbered 0..N-1 dark to light (rgbOut: their R, G, B). With --width: raw
// 4-bit pixels, two a byte, the left one the low nibble
static int lum(const uint8_t* rgb) { return rgb[0] * 299 + rgb[1] * 587 + rgb[2] * 114; }

static bool readImageFile(const std::string& path, const std::vector<uint8_t>& in, int width,
                          const std::vector<uint8_t>& pal, pack45::Image& img, std::vector<uint8_t>& rgbOut,
                          std::string& err) {
    if (width) {
        if (width < 2 || width > 256 || (width & 1)) { err = "--width is an even number of pixels, 2-256"; return false; }
        size_t rowBytes = (size_t)width / 2;
        if (in.empty() || in.size() % rowBytes) { err = "the file's size is not a whole number of rows of " + std::to_string(rowBytes) + " bytes"; return false; }
        img.w = width;
        img.h = (int)(in.size() / rowBytes);
        for (uint8_t b : in) { img.px.push_back(b & 15); img.px.push_back(b >> 4); }
    } else {
        int w = 0, h = 0;
        std::vector<uint8_t> rgba;
        if (!font45::decodeRgba(in, w, h, rgba, err)) return false;
        img.w = w;
        img.h = h;
        img.px.resize((size_t)w * h);
        std::vector<uint32_t> colours;      // (R << 16 | G << 8 | B)
        std::vector<int> index((size_t)w * h);
        for (size_t i = 0; i < (size_t)w * h; i++) {
            uint32_t c = (uint32_t)rgba[4 * i] << 16 | rgba[4 * i + 1] << 8 | rgba[4 * i + 2];
            auto it = std::find(colours.begin(), colours.end(), c);
            if (it == colours.end()) {
                if (pal.empty() && colours.size() == 16) { err = "more than 16 colours"; return false; }
                colours.push_back(c);
                it = colours.end() - 1;
            }
            index[i] = (int)(it - colours.begin());
        }
        std::vector<int> entryOf(colours.size(), -1);
        if (!pal.empty()) {
            for (size_t k = 0; k < colours.size(); k++) {
                for (size_t e = 0; e < pal.size() / 3 && e < 16; e++)
                    if (((uint32_t)pal[3 * e] << 16 | pal[3 * e + 1] << 8 | pal[3 * e + 2]) == colours[k]) { entryOf[k] = (int)e; break; }
                if (entryOf[k] < 0) {
                    char m[96];
                    snprintf(m, sizeof m, "colour #%06X is not one of the palette's first 16", (unsigned)colours[k]);
                    err = m;
                    return false;
                }
            }
        } else {
            // dark to light: entries 0..N-1
            std::vector<int> byLum(colours.size());
            for (size_t k = 0; k < byLum.size(); k++) byLum[k] = (int)k;
            auto rgbOf = [&](int k) { uint8_t c[3] = {(uint8_t)(colours[k] >> 16), (uint8_t)(colours[k] >> 8), (uint8_t)colours[k]}; return lum(c); };
            std::stable_sort(byLum.begin(), byLum.end(), [&](int a, int b) {
                return rgbOf(a) != rgbOf(b) ? rgbOf(a) < rgbOf(b) : colours[a] < colours[b];
            });
            for (size_t e = 0; e < byLum.size(); e++) {
                entryOf[byLum[e]] = (int)e;
                uint32_t c = colours[byLum[e]];
                rgbOut.push_back((uint8_t)(c >> 16));
                rgbOut.push_back((uint8_t)(c >> 8));
                rgbOut.push_back((uint8_t)c);
            }
        }
        for (size_t i = 0; i < img.px.size(); i++) img.px[i] = (uint8_t)entryOf[index[i]];
    }
    (void)path;
    // the order: the colours used, dark to light by the palette (without one:
    // by entry, which for an image's own colours is dark to light already)
    bool used[16] = {};
    for (uint8_t p : img.px) used[p] = true;
    for (int e = 0; e < 16; e++)
        if (used[e]) img.order.push_back((uint8_t)e);
    if (!pal.empty())
        std::stable_sort(img.order.begin(), img.order.end(), [&](uint8_t a, uint8_t b) {
            int la = (size_t)a * 3 + 2 < pal.size() ? lum(&pal[3 * a]) : 0, lb = (size_t)b * 3 + 2 < pal.size() ? lum(&pal[3 * b]) : 0;
            return la < lb;
        });
    return true;
}

// --suggest-rate: each sound's energy below which keep per cent lies, and
// the lowest usual rate whose pass band holds it; the bytes at 8 bits a
// sample, and the MEGA65 audio DMA's rate for it (Hz * 2^24 /
// 40.5 MHz)
// the rate --suggest-rate names for w (and the frequency below which keep
// per cent of its energy lies, in *hz)
static uint32_t suggestedRate(const pack45::Wav& w, double keep, uint32_t maxRate, double* hz) {
    uint32_t top = maxRate && maxRate < w.rate ? maxRate : w.rate;
    *hz = pack45::energyBelow(w, keep / 100, top < w.rate ? 0.45 * top : 0);
    return pack45::rateFor(*hz, top);
}

static unsigned long dmaRate(uint32_t hz) { return (unsigned long)std::lround((double)hz * (1 << 24) / 40500000.0); }

static bool readSoundFile(const std::string& f, pack45::Wav& w) {
    std::vector<uint8_t> in;
    std::string err;
    if (!readFile(f, in)) { std::cerr << "pack45: cannot read " << f << "\n"; return false; }
    if (!pack45::readSound(f, in, w, &err)) { std::cerr << "pack45: " << f << ": " << err << "\n"; return false; }
    if (w.samples.empty()) { std::cerr << "pack45: " << f << ": no samples\n"; return false; }
    return true;
}

static int suggestRates(const std::vector<std::string>& files, double keep, uint32_t maxRate) {
    if (files.empty()) { usage(); return 1; }
    int status = 0;
    for (const auto& f : files) {
        pack45::Wav w;
        if (!readSoundFile(f, w)) { status = 1; continue; }
        double hz;
        uint32_t r = suggestedRate(w, keep, maxRate, &hz);
        double secs = (double)w.samples.size() / w.rate;
        char line[512];
        snprintf(line, sizeof line,
                 "pack45: %s: %u Hz, %d-bit, %d channel%s, %.2f s; %g%% of its energy below %.0f Hz: "
                 "%u Hz keeps it (%.0f bytes at 8 bits, %.0f%% of %u Hz); audio DMA rate $%06lX",
                 f.c_str(), (unsigned)w.rate, w.bits, w.channels, w.channels == 1 ? "" : "s", secs, keep, hz,
                 (unsigned)r, secs * r, 100.0 * r / w.rate, (unsigned)w.rate,
                 dmaRate(r));
        std::cout << line << std::endl;
    }
    return status;
}

int main(int argc, char** argv) {
    Logger::setTool("pack45");
    argc = Logger::takeFlags(argc, argv);
    uint32_t dest = 0, load = 0;
    bool haveDest = false, haveLoad = false, raw = false, prg = false, extract = false, best = false, suggest = false;
    bool sound = false, isUnsigned = false, normalise = false, image = false;
    double keep = 99.9;
    uint32_t maxRate = 0, rate = 0, width = 0;
    int predict = -1;
    std::string paletteIn, paletteOut;
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
        if (standardFlag(a) == StdFlag::Help) { usage(); return 0; }
        else if (standardFlag(a) == StdFlag::Version) { std::cout << suiteVersionString("pack45") << std::endl; return 0; }
        else if (a == "-d" || a == "--dest") { value(dest); haveDest = true; }
        else if (a == "--load") { value(load); haveLoad = true; }
        else if (a == "--raw") raw = true;
        else if (a == "--best") best = true;
        else if (a == "--prg") prg = true;
        else if (a == "-x") extract = true;
        else if (a == "--suggest-rate") suggest = true;
        else if (a == "--sound") sound = true;
        else if (a == "--image") image = true;
        else if (a == "--width") { value(width); if (!width) { std::cerr << "pack45: --width needs a width in pixels\n"; return 1; } }
        else if (a == "--palette" || a == "--palette-out" || a == "--predict") {
            if (i + 1 >= argc) { std::cerr << "pack45: " << a << " needs a value\n"; return 1; }
            std::string v = argv[++i];
            if (a == "--palette") paletteIn = v;
            else if (a == "--palette-out") paletteOut = v;
            else if (v == "above") predict = 0;
            else if (v == "left") predict = 1;
            else { std::cerr << "pack45: --predict is above or left\n"; return 1; }
        }
        else if (a == "--unsigned") isUnsigned = true;
        else if (a == "--normalise" || a == "--normalize") normalise = true;
        else if (a == "--rate") { value(rate); if (rate < 1000 || rate > 192000) { std::cerr << "pack45: --rate needs a rate in Hz (1000-192000)\n"; return 1; } }
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
    if (sound) {
        // the sound to samples: written as they are, or (with -d) packed below
        pack45::Wav w;
        if (!readSoundFile(files[0], w)) return 1;
        double hz = 0;
        uint32_t r = rate ? rate : suggestedRate(w, keep, maxRate, &hz);
        in = pack45::toBytes(pack45::resample(w, r), isUnsigned, normalise);
        char line[512];
        snprintf(line, sizeof line, "pack45: %s: %u Hz, %d channel%s, %.2f s -> %u Hz%s, %zu %s 8-bit samples; audio DMA rate $%06lX",
                 files[0].c_str(), (unsigned)w.rate, w.channels, w.channels == 1 ? "" : "s",
                 (double)w.samples.size() / w.rate, (unsigned)r, rate ? "" : " (suggested)", in.size(),
                 isUnsigned ? "unsigned" : "signed", dmaRate(r));
        std::cout << line << std::endl;
        if (!haveDest) {
            std::vector<uint8_t> out;
            if (haveLoad) { out.push_back((uint8_t)load); out.push_back((uint8_t)(load >> 8)); }
            out.insert(out.end(), in.begin(), in.end());
            if (!writeFile(files[1], out)) { std::cerr << "pack45: cannot write " << files[1] << "\n"; return 1; }
            return 0;
        }
    } else if (!readFile(files[0], in)) { std::cerr << "pack45: cannot read " << files[0] << "\n"; return 1; }

    if (image && !extract) {
        if (sound) { std::cerr << "pack45: --image and --sound together\n"; return 1; }
        if (!haveDest) { std::cerr << "pack45: where does it unpack to? -d ADDR\n"; return 1; }
        std::vector<uint8_t> pal;
        if (!paletteIn.empty()) {
            if (!readFile(paletteIn, pal)) { std::cerr << "pack45: cannot read " << paletteIn << "\n"; return 1; }
            if (pal.size() < 3 || pal.size() % 3) { std::cerr << "pack45: " << paletteIn << ": not R, G, B bytes of entries\n"; return 1; }
            if (!paletteOut.empty()) { std::cerr << "pack45: --palette-out is for an image's own colours (without --palette)\n"; return 1; }
        }
        pack45::Image img;
        std::vector<uint8_t> rgb;
        std::string err;
        if (!readImageFile(files[0], in, (int)width, pal, img, rgb, err)) { std::cerr << "pack45: " << files[0] << ": " << err << "\n"; return 1; }
        pack45::Packed p;
        int chosen = 0;
        if (!pack45::packImage(img, dest, predict, p, &chosen, &err)) { std::cerr << "pack45: " << files[0] << ": " << err << "\n"; return 1; }
        std::vector<uint8_t> bytes = pack45::imageBytes(img), back;
        if (!pack45::unpack(p.stream, back, nullptr) || back != bytes || !pack45::checkInPlace(bytes, p, p.inPlace)) {
            std::cerr << "pack45: internal error: the stream does not unpack to the image\n";
            return 2;
        }
        size_t f1 = pack45::pack(bytes, dest).stream.size();
        if (!haveLoad) load = dest + p.inPlace;
        std::vector<uint8_t> out;
        if (!raw) { out.push_back((uint8_t)load); out.push_back((uint8_t)(load >> 8)); }
        out.insert(out.end(), p.stream.begin(), p.stream.end());
        if (!writeFile(files[1], out)) { std::cerr << "pack45: cannot write " << files[1] << "\n"; return 1; }
        if (!paletteOut.empty() && !writeFile(paletteOut, rgb)) { std::cerr << "pack45: cannot write " << paletteOut << "\n"; return 1; }
        char line[320];
        snprintf(line, sizeof line,
                 "pack45: %s: %dx%d, %zu colour%s, from %s: %zu -> %zu bytes (%zu%%; format 1: %zu), unpacks to $%X; "
                 "in place: load it at $%X (offset %u)",
                 files[0].c_str(), img.w, img.h, img.order.size(), img.order.size() == 1 ? "" : "s",
                 chosen ? "the left" : "above", bytes.size(), p.stream.size(), p.stream.size() * 100 / bytes.size(), f1,
                 (unsigned)dest, (unsigned)(dest + p.inPlace), (unsigned)p.inPlace);
        std::cout << line << std::endl;
        if (!rgb.empty() && paletteIn.empty()) {
            std::string m = "pack45: its colours, entry 0 up:";
            for (size_t k = 0; k + 2 < rgb.size(); k += 3) {
                char c[16];
                snprintf(c, sizeof c, " #%02X%02X%02X", rgb[k], rgb[k + 1], rgb[k + 2]);
                m += c;
            }
            std::cout << m << std::endl;
        }
        return 0;
    }

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
    if (!sound && lower.size() > 4 && lower.substr(lower.size() - 4) == ".prg") prg = true;
    if (prg && !sound) {
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
