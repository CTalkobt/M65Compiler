#pragma once
// The code overlay files ln45 writes and their description, read by nm45
// and objdump45.
//
// <program>.ov1 (the code; more banks: .ov2, ...):
//   load address (2 bytes: the far address's low 16 bits)
//   the overlays' code, one after another (mapped overlays page-aligned)
//
// <program>.ovd (the description, which the program never loads):
//   "OVL45" 2                       signature, version
//   region (2) flags (1) count (1)  where overlays run; bit 0: mapped
//   per overlay: file (1, 1: .ov1), offset in it (3), far address (3),
//                length (2), name length (1), name
//   symbol count (2)
//   per symbol: overlay (1, 1..n), address (2), name length (1), name
//   the description's length (4) "OVL45"
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

struct O45OverlayFile {
    struct Overlay {
        std::string name;
        uint32_t at = 0;          // the far address it is kept at
        uint32_t length = 0;
        int file = 1;             // the code file it is in (1: .ov1)
        size_t fileOffset = 0;    // where in that file
    };
    struct Symbol { int overlay = 0; uint32_t address = 0; std::string name; };
    uint32_t loadAddress = 0;     // the far address of the first code file's first byte
    uint32_t region = 0;          // where the overlays run
    bool mapped = false;
    std::vector<Overlay> overlays;
    std::vector<Symbol> symbols;
    std::vector<std::vector<uint8_t>> files;   // the code files read by open() (files[0]: .ov1)

    static constexpr const char* kSig = "OVL45";

    // The description (the .ovd file)
    std::vector<uint8_t> describe() const {
        std::vector<uint8_t> d;
        auto put = [&](uint32_t v, int n) { for (int i = 0; i < n; i++) d.push_back((v >> (8 * i)) & 0xFF); };
        auto str = [&](const std::string& s) { d.push_back((uint8_t)std::min<size_t>(s.size(), 255)); for (size_t i = 0; i < s.size() && i < 255; i++) d.push_back((uint8_t)s[i]); };
        for (const char* p = kSig; *p; p++) d.push_back((uint8_t)*p);
        d.push_back(2);
        put(region, 2);
        d.push_back(mapped ? 1 : 0);
        d.push_back((uint8_t)overlays.size());
        for (const auto& o : overlays) {
            d.push_back((uint8_t)o.file);
            put((uint32_t)o.fileOffset, 3);
            put(o.at, 3);
            put(o.length, 2);
            str(o.name);
        }
        put((uint32_t)symbols.size(), 2);
        for (const auto& s : symbols) { d.push_back((uint8_t)s.overlay); put(s.address, 2); str(s.name); }
        uint32_t len = (uint32_t)d.size() + 9;
        put(len, 4);
        for (const char* p = kSig; *p; p++) d.push_back((uint8_t)*p);
        return d;
    }

    static bool endsWithSig(const std::vector<uint8_t>& f) {
        if (f.size() < 2 + 9 + 10) return false;
        for (int i = 0; i < 5; i++) if (f[f.size() - 5 + i] != (uint8_t)kSig[i]) return false;
        return true;
    }
    // A description (.ovd): the signature at both ends
    static bool isDescription(const std::vector<uint8_t>& f) {
        if (!endsWithSig(f)) return false;
        for (int i = 0; i < 5; i++) if (f[i] != (uint8_t)kSig[i]) return false;
        return true;
    }
    // A code file (.ov1, .ov2, ...): the name ends in .ov and a digit
    static int codeFileNumber(const std::string& path) {
        size_t n = path.size();
        if (n < 4 || path[n - 4] != '.' || std::tolower((unsigned char)path[n - 3]) != 'o' ||
            std::tolower((unsigned char)path[n - 2]) != 'v' || path[n - 1] < '1' || path[n - 1] > '9') return 0;
        return path[n - 1] - '0';
    }
    // Is this (path, contents) an overlay file nm45 / objdump45 should read
    // with open(): a description or a code file?
    static bool is(const std::string& path, const std::vector<uint8_t>& f) {
        return isDescription(f) || codeFileNumber(path) > 0;
    }

    static bool readFile(const std::string& path, std::vector<uint8_t>& out) {
        std::ifstream in(path, std::ios::binary);
        if (!in) return false;
        out.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
        return true;
    }

    // The description and the code of the overlay file at path (contents f):
    // a .ovd with its .ov1, .ov2, ... next to it, or a code file with its
    // .ovd. The code files are in files (empty when missing: the
    // description alone is enough for the symbols).
    static bool open(const std::string& path, const std::vector<uint8_t>& f, O45OverlayFile& out, std::string& err) {
        out = O45OverlayFile();
        std::string base = path;
        size_t dot = base.find_last_of('.'), sl = base.find_last_of('/');
        if (dot != std::string::npos && (sl == std::string::npos || dot > sl)) base = base.substr(0, dot);
        std::vector<uint8_t> desc;
        if (isDescription(f)) desc = f;
        else if (!readFile(base + ".ovd", desc)) {
            err = "no description " + base + ".ovd next to the overlay code file";
            return false;
        }
        if (!isDescription(desc)) { err = "not a code overlay description"; return false; }
        if (!parse(desc.data(), desc.size(), out, err)) return false;
        int nfiles = 0;
        for (const auto& o : out.overlays) nfiles = std::max(nfiles, o.file);
        for (int i = 1; i <= nfiles; i++) {
            std::vector<uint8_t> code;
            if (codeFileNumber(path) == i) code = f;
            else readFile(base + ".ov" + std::to_string(i), code);
            out.files.push_back(code);
        }
        if (!out.files.empty() && out.files[0].size() >= 2 && !out.overlays.empty())
            out.loadAddress = (out.overlays[0].at & ~0xFFFFu) | out.files[0][0] | (out.files[0][1] << 8);
        for (const auto& o : out.overlays) {
            size_t fi = (size_t)o.file - 1;
            if (fi < out.files.size() && !out.files[fi].empty() && o.fileOffset + o.length > out.files[fi].size()) {
                err = "overlay outside the file";
                return false;
            }
        }
        return true;
    }

    // The code of overlay i (empty when its file was not found)
    std::vector<uint8_t> code(size_t i) const {
        const auto& o = overlays[i];
        size_t fi = (size_t)o.file - 1;
        if (fi >= files.size() || files[fi].empty()) return {};
        return std::vector<uint8_t>(files[fi].begin() + o.fileOffset, files[fi].begin() + o.fileOffset + o.length);
    }

private:
    // A description of len bytes at d
    static bool parse(const uint8_t* d, size_t len, O45OverlayFile& out, std::string& err) {
        size_t p = 0, end = len - 9;
        auto need = [&](size_t k) { if (p + k > end) throw 0; };
        auto get = [&](int k) { need(k); uint32_t v = 0; for (int i = 0; i < k; i++) v |= (uint32_t)d[p++] << (8 * i); return v; };
        auto str = [&]() { uint32_t l = get(1); need(l); std::string s(d + p, d + p + l); p += l; return s; };
        try {
            for (int i = 0; i < 5; i++) if (d[p + i] != (uint8_t)kSig[i]) { err = "bad overlay description"; return false; }
            p += 5;
            if (get(1) != 2) { err = "unknown overlay file version"; return false; }
            out.region = get(2);
            out.mapped = (get(1) & 1) != 0;
            uint32_t count = get(1);
            for (uint32_t i = 0; i < count; i++) {
                Overlay o;
                o.file = (int)get(1);
                o.fileOffset = get(3);
                o.at = get(3);
                o.length = get(2);
                o.name = str();
                out.overlays.push_back(o);
            }
            uint32_t ns = get(2);
            for (uint32_t i = 0; i < ns; i++) {
                Symbol s;
                s.overlay = (int)get(1);
                s.address = get(2);
                s.name = str();
                out.symbols.push_back(s);
            }
        } catch (...) {
            err = "overlay description cut short";
            return false;
        }
        return true;
    }
};
