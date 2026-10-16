#pragma once
// The code overlay file ln45 writes (<program>.ovl) and its description,
// read by nm45 and objdump45.
//
//   load address (2 bytes: the far address's low 16 bits)
//   the overlays' code, one after another (mapped overlays page-aligned)
//   the description:
//     "OVL45" 1                       signature, version
//     region (2) flags (1) count (1)  where overlays run; bit 0: mapped
//     per overlay: far address (3), length (2), name length (1), name
//     symbol count (2)
//     per symbol: overlay (1, 1..n), address (2), name length (1), name
//     the description's length (4) "OVL45"
//
// The program loads the whole file (the description too) into far memory;
// only the code is used there.
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

struct O45OverlayFile {
    struct Overlay { std::string name; uint32_t at = 0; uint32_t length = 0; size_t fileOffset = 0; };
    struct Symbol { int overlay = 0; uint32_t address = 0; std::string name; };
    uint32_t loadAddress = 0;     // the far address of the file's first byte (bits 0-15 in the file)
    uint32_t region = 0;          // where the overlays run
    bool mapped = false;
    std::vector<Overlay> overlays;
    std::vector<Symbol> symbols;

    static constexpr const char* kSig = "OVL45";

    // The description (to append after the code)
    std::vector<uint8_t> describe() const {
        std::vector<uint8_t> d;
        auto put = [&](uint32_t v, int n) { for (int i = 0; i < n; i++) d.push_back((v >> (8 * i)) & 0xFF); };
        auto str = [&](const std::string& s) { d.push_back((uint8_t)std::min<size_t>(s.size(), 255)); for (size_t i = 0; i < s.size() && i < 255; i++) d.push_back((uint8_t)s[i]); };
        for (const char* p = kSig; *p; p++) d.push_back((uint8_t)*p);
        d.push_back(1);
        put(region, 2);
        d.push_back(mapped ? 1 : 0);
        d.push_back((uint8_t)overlays.size());
        for (const auto& o : overlays) { put(o.at, 3); put(o.length, 2); str(o.name); }
        put((uint32_t)symbols.size(), 2);
        for (const auto& s : symbols) { d.push_back((uint8_t)s.overlay); put(s.address, 2); str(s.name); }
        uint32_t len = (uint32_t)d.size() + 9;
        put(len, 4);
        for (const char* p = kSig; *p; p++) d.push_back((uint8_t)*p);
        return d;
    }

    // Is this an overlay file (its description at the end)?
    static bool is(const std::vector<uint8_t>& f) {
        if (f.size() < 2 + 9 + 10) return false;
        for (int i = 0; i < 5; i++) if (f[f.size() - 5 + i] != (uint8_t)kSig[i]) return false;
        return true;
    }

    static bool read(const std::vector<uint8_t>& f, O45OverlayFile& out, std::string& err) {
        out = O45OverlayFile();
        if (!is(f)) { err = "not a code overlay file (no OVL45 description at its end)"; return false; }
        size_t n = f.size();
        uint32_t len = f[n - 9] | (f[n - 8] << 8) | (f[n - 7] << 16) | ((uint32_t)f[n - 6] << 24);
        if (len < 19 || len > n - 2) { err = "bad overlay description length"; return false; }
        size_t p = n - len, end = n - 9;
        auto need = [&](size_t k) { if (p + k > end) throw 0; };
        auto get = [&](int k) { need(k); uint32_t v = 0; for (int i = 0; i < k; i++) v |= (uint32_t)f[p++] << (8 * i); return v; };
        auto str = [&]() { uint32_t l = get(1); need(l); std::string s(f.begin() + p, f.begin() + p + l); p += l; return s; };
        try {
            for (int i = 0; i < 5; i++) if (f[p + i] != (uint8_t)kSig[i]) { err = "bad overlay description"; return false; }
            p += 5;
            if (get(1) != 1) { err = "unknown overlay file version"; return false; }
            out.region = get(2);
            out.mapped = (get(1) & 1) != 0;
            uint32_t count = get(1);
            uint32_t load16 = f[0] | (f[1] << 8);
            for (uint32_t i = 0; i < count; i++) {
                Overlay o;
                o.at = get(3);
                o.length = get(2);
                o.name = str();
                out.overlays.push_back(o);
            }
            // the far address of the file: the first overlay's bank with
            // the load address's 16 bits
            out.loadAddress = (out.overlays.empty() ? 0 : (out.overlays[0].at & ~0xFFFFu)) | load16;
            for (auto& o : out.overlays) {
                o.fileOffset = 2 + (o.at - out.loadAddress);
                if (o.at < out.loadAddress || o.fileOffset + o.length > n - len) { err = "overlay outside the file"; return false; }
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
