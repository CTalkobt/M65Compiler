// font45 — fonts for the MEGA65 (#466): bitmap fonts (BDF, PSF, C64
// charsets, hand-drawn glyphs) as 8x8 text-mode charsets or the library's
// proportional fonts (<font.h>), written as C, ca45 assembly or binary.
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
#include "Font45.hpp"
#include "Logger.hpp"
#include "Version.hpp"

namespace {

bool readFile(const std::string& path, std::vector<uint8_t>& out) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) return false;
    out.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    return true;
}

bool writeFile(const std::string& path, const std::string& s) {
    std::ofstream out(path, std::ios::binary);
    out << s;
    return (bool)out;
}

bool writeBytes(const std::string& path, const std::vector<uint8_t>& d) {
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char*>(d.data()), (std::streamsize)d.size());
    return (bool)out;
}

bool parseOrder(const std::string& s, font45::Order& o) {
    if (s == "ascii") o = font45::Order::Ascii;
    else if (s == "screencode") o = font45::Order::ScreenCode;
    else if (s == "petscii") o = font45::Order::Petscii;
    else return false;
    return true;
}

bool parseNum(const std::string& s, long& v) {
    if (s.empty()) return false;
    char* end = nullptr;
    if (s[0] == '$') v = std::strtol(s.c_str() + 1, &end, 16);
    else v = std::strtol(s.c_str(), &end, 0);
    return end && *end == 0;
}

void usage() {
    std::cout <<
        "Usage: font45 convert <font> [options] -o <output>\n"
        "       font45 list <font>\n"
        "Fonts for the MEGA65 (#466): a bitmap font as an 8x8 text-mode charset, or as\n"
        "<font.h>'s proportional font (font_from, font_draw_bitmap).\n"
        "Inputs (by extension, or --format): .bdf (X11), .psf / .psfu (PSF1 / PSF2),\n"
        ".bin / .64c / .chr (a C64 / C65 charset, 2048 or 4096 bytes, load address or not),\n"
        ".txt (hand-drawn glyphs: \"<code>: 01110 10001 ...\", a row of 0/1 or ./# each).\n"
        "  -o FILE            the output: .c (and its .h), .s45 / .asm, .bin / .prg (binary)\n"
        "  --c, --asm, --bin  the output's form, whatever its name\n"
        "  --name NAME        the array's / label's name (default: the output's base name)\n"
        "  --charset ORDER    an 8x8 1 bpp charset (VIC-IV text mode) in this order: ascii,\n"
        "                     screencode (the C64's lower/upper case set; 128-255 inverse)\n"
        "                     or petscii; --count 256 (default) or 512\n"
        "  --bpp 1|2|4        a <font.h> font, 1, 2 or 4 bits a pixel (coverage levels)\n"
        "  --encoding E       the font's codes: petscii (default: cc45's string literals),\n"
        "                     ascii (#pragma encoding ascii) or screencode\n"
        "  --proportional     a width per glyph (default: fixed, the widest glyph's cell)\n"
        "  --fixed W          a fixed cell W pixels wide\n"
        "  --range A-B        only codes A..B (also the font's first / last)\n"
        "  --map FROM=TO      a code's glyph also under another code (repeatable)\n"
        "  --bold, --italic, --outline   a derived variant\n"
        "  --load-at ADDR     binary: a two-byte load address in front (cbm_load / load_far)\n"
        "  --format F         the input's format: bdf, psf, charset, text\n"
        "  --from ORDER       a charset input's order: screencode (default), petscii, ascii\n"
        "  -V, --version      the version\n";
}

std::string baseName(const std::string& path) {
    auto slash = path.find_last_of('/');
    std::string b = slash == std::string::npos ? path : path.substr(slash + 1);
    auto dot = b.find('.');
    if (dot != std::string::npos) b = b.substr(0, dot);
    std::string out;
    for (char c : b) out += (std::isalnum((unsigned char)c) ? c : '_');
    if (out.empty() || std::isdigit((unsigned char)out[0])) out = "font_" + out;
    return out;
}

std::string extOf(const std::string& path) {
    auto dot = path.find_last_of('.');
    if (dot == std::string::npos) return "";
    std::string e = path.substr(dot + 1);
    for (auto& c : e) c = (char)std::tolower((unsigned char)c);
    return e;
}

}  // namespace

int main(int argc, char** argv) {
    Logger::setTool("font45");
    argc = Logger::takeFlags(argc, argv);
    if (argc < 2) { usage(); return 1; }
    std::string cmd = argv[1];
    if (cmd == "-h" || cmd == "--help") { usage(); return 0; }
    if (cmd == "-V" || cmd == "--version") { std::cout << suiteVersionString("font45") << std::endl; return 0; }
    if (cmd != "convert" && cmd != "list") { std::cerr << "font45: unknown command " << cmd << " (convert, list)\n"; return 1; }

    std::string input, output, name, format, form, charset, encoding = "petscii", from = "screencode";
    int bpp = 0, count = 256, fixedW = 0;
    long loadAt = -1, lo = -1, hi = -1;
    bool proportional = false, bold = false, italic = false, outline = false;
    std::vector<std::pair<int, int>> maps;
    for (int i = 2; i < argc; i++) {
        std::string a = argv[i];
        auto need = [&](const char* what) -> std::string {
            if (i + 1 >= argc) { std::cerr << "font45: " << a << " needs " << what << "\n"; std::exit(1); }
            return argv[++i];
        };
        if (a == "-o") output = need("a file");
        else if (a == "--name") name = need("a name");
        else if (a == "--format") format = need("a format (bdf, psf, charset, text)");
        else if (a == "--c" || a == "--asm" || a == "--bin") form = a.substr(2);
        else if (a == "--encoding") encoding = need("an encoding (petscii, ascii, screencode)");
        else if (a == "--from") from = need("an order (screencode, petscii, ascii)");
        else if (a == "--charset") charset = need("an order (ascii, screencode, petscii)");
        else if (a == "--count") { long v; if (!parseNum(need("256 or 512"), v) || (v != 256 && v != 512)) { std::cerr << "font45: --count is 256 or 512\n"; return 1; } count = (int)v; }
        else if (a == "--bpp") { long v; if (!parseNum(need("1, 2 or 4"), v) || (v != 1 && v != 2 && v != 4)) { std::cerr << "font45: --bpp is 1, 2 or 4\n"; return 1; } bpp = (int)v; }
        else if (a == "--proportional") proportional = true;
        else if (a == "--fixed") { long v; if (!parseNum(need("a width"), v) || v < 1 || v > 255) { std::cerr << "font45: --fixed needs a width 1-255\n"; return 1; } fixedW = (int)v; }
        else if (a == "--range") {
            std::string r = need("A-B");
            auto dash = r.find('-', 1);
            if (dash == std::string::npos || !parseNum(r.substr(0, dash), lo) || !parseNum(r.substr(dash + 1), hi) || lo > hi || hi > 255) {
                std::cerr << "font45: --range needs A-B (0-255)\n"; return 1;
            }
        } else if (a == "--map") {
            std::string m = need("FROM=TO");
            auto eq = m.find('=');
            long f, t;
            if (eq == std::string::npos || !parseNum(m.substr(0, eq), f) || !parseNum(m.substr(eq + 1), t)) {
                std::cerr << "font45: --map needs FROM=TO (codes)\n"; return 1;
            }
            maps.push_back({(int)f, (int)t});
        } else if (a == "--bold") bold = true;
        else if (a == "--italic") italic = true;
        else if (a == "--outline") outline = true;
        else if (a == "--load-at") { if (!parseNum(need("an address"), loadAt) || loadAt < 0 || loadAt > 0xFFFF) { std::cerr << "font45: --load-at needs an address 0-$FFFF\n"; return 1; } }
        else if (!a.empty() && a[0] == '-') { std::cerr << "font45: unknown option " << a << "\n"; return 1; }
        else if (input.empty()) input = a;
        else { std::cerr << "font45: one font at a time (" << a << ")\n"; return 1; }
    }
    if (input.empty()) { std::cerr << "font45: no font given\n"; return 1; }

    std::vector<uint8_t> data;
    if (!readFile(input, data)) { std::cerr << "font45: cannot read " << input << "\n"; return 1; }
    if (format.empty()) format = font45::formatOf(input);
    font45::Font f;
    std::string err;
    bool ok;
    if (format == "bdf") ok = font45::readBDF(data, f, err);
    else if (format == "psf") ok = font45::readPSF(data, f, err);
    else if (format == "charset") ok = font45::readCharset(data, f, err);
    else if (format == "text") ok = font45::readGlyphText(std::string(data.begin(), data.end()), f, err);
    else { std::cerr << "font45: " << input << ": an unknown format (--format bdf, psf, charset, text)\n"; return 1; }
    if (!ok) { std::cerr << "font45: " << input << ": " << err << "\n"; return 1; }
    font45::Order fromOrder, encOrder;
    if (!parseOrder(from, fromOrder)) { std::cerr << "font45: --from is screencode, petscii or ascii\n"; return 1; }
    if (!parseOrder(encoding, encOrder)) { std::cerr << "font45: --encoding is petscii, ascii or screencode\n"; return 1; }
    if (format == "charset") font45::decodeCharset(f, fromOrder);
    // (a <font.h> font's codes are the program's: PETSCII unless told; an 8x8
    // charset's order is --charset's, from the character codes)
    if (charset.empty() && cmd == "convert") font45::reencode(f, encOrder);

    for (auto [from, to] : maps) font45::remap(f, from, to);
    if (lo >= 0) font45::keepRange(f, (int)lo, (int)hi);
    if (bold) font45::makeBold(f);
    if (italic) font45::makeItalic(f);
    if (outline) font45::makeOutline(f);

    if (cmd == "list") { std::cout << font45::describe(f); return 0; }

    if (output.empty()) { std::cerr << "font45: no output (-o FILE)\n"; return 1; }
    if (form.empty()) {
        std::string e = extOf(output);
        form = (e == "c" || e == "h") ? "c" : (e == "s45" || e == "asm" || e == "s") ? "asm" : "bin";
    }
    if (name.empty()) name = baseName(output);
    if (f.glyphs.empty()) { std::cerr << "font45: no glyphs left (--range)\n"; return 1; }

    std::vector<uint8_t> out;
    std::string what;
    bool isFont = charset.empty();
    if (!isFont) {
        font45::Order order;
        if (charset == "ascii") order = font45::Order::Ascii;
        else if (charset == "screencode") order = font45::Order::ScreenCode;
        else if (charset == "petscii") order = font45::Order::Petscii;
        else { std::cerr << "font45: --charset is ascii, screencode or petscii\n"; return 1; }
        if (f.height > 8) std::cerr << "font45: warning: glyphs " << f.height << " rows high: the top 8 used\n";
        out = font45::charset8x8(f, order, count);
        what = "an 8x8 charset, " + std::to_string(count) + " characters, " + charset + " order";
    } else {
        if (!bpp) bpp = 1;
        int first = lo >= 0 ? (int)lo : f.glyphs.begin()->first, last = hi >= 0 ? (int)hi : f.glyphs.rbegin()->first;
        if (first > 255 || last > 255) {
            std::cerr << "font45: codes above 255 (" << last << "): --range to choose 256 of them\n";
            return 1;
        }
        if (f.height > 255) { std::cerr << "font45: glyphs over 255 rows high\n"; return 1; }
        out = font45::fontData(f, bpp, proportional, first, last, fixedW);
        what = std::string(proportional ? "a proportional" : "a fixed") + " font, " + std::to_string(bpp) +
               " bpp, codes " + std::to_string(first) + "-" + std::to_string(last) + ", " + std::to_string(f.height) +
               " rows";
    }

    if (form == "c") {
        std::string h = output;
        auto dot = h.find_last_of('.');
        h = (dot == std::string::npos ? h : h.substr(0, dot)) + ".h";
        if (!writeFile(output, font45::asC(out, name, what)) || !writeFile(h, font45::asHeader(out, name, what, isFont))) {
            std::cerr << "font45: cannot write " << output << "\n"; return 1;
        }
    } else if (form == "asm") {
        if (!writeFile(output, font45::asAsm(out, name, what))) { std::cerr << "font45: cannot write " << output << "\n"; return 1; }
    } else {
        if (!writeBytes(output, font45::asBinary(out, (int)loadAt))) { std::cerr << "font45: cannot write " << output << "\n"; return 1; }
    }
    std::cout << "font45: " << input << ": " << what << ", " << out.size() << " bytes -> " << output << std::endl;
    return 0;
}
