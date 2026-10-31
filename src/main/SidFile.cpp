// SidFile.cpp — a PSID/RSID file read, and its tune run on an emulated
// 6502 recording the SID writes (SidFile.hpp)
#include "SidFile.hpp"
#include <cstdio>
#include <initializer_list>
#include <memory>

// ---------------------------------------------------------------- the 6502

namespace {

enum Mode { IMP, ACC, IMM, ZP, ZPX, ZPY, ABS, ABX, ABY, IND, IZX, IZY, REL, JAM };

enum : uint8_t { FC = 0x01, FZ = 0x02, FI = 0x04, FD = 0x08, FB = 0x10, FU = 0x20, FV = 0x40, FN = 0x80 };

// the NMOS cycles, without page crossings and taken branches
const uint8_t kCycles[256] = {
    7,6,0,8,3,3,5,5,3,2,2,2,4,4,6,6,  2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
    6,6,0,8,3,3,5,5,4,2,2,2,4,4,6,6,  2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
    6,6,0,8,3,3,5,5,3,2,2,2,3,4,6,6,  2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
    6,6,0,8,3,3,5,5,4,2,2,2,5,4,6,6,  2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
    2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,  2,6,0,6,4,4,4,4,2,5,2,5,5,5,5,5,
    2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,  2,5,0,5,4,4,4,4,2,4,2,4,4,4,4,4,
    2,6,2,8,3,3,5,5,2,2,2,2,4,4,6,6,  2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
    2,6,2,8,3,3,5,5,2,2,2,2,4,4,6,6,  2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
};

// the addressing mode of each opcode (the 6502's opcode matrix is regular)
struct ModeTable {
    uint8_t m[256];
    ModeTable() {
        for (int op = 0; op < 256; op++) {
            int col = op & 0x1F, row = op >> 5;
            Mode md = IMP;
            switch (col) {
            case 0x00: md = op == 0x20 ? ABS : row >= 4 ? IMM : IMP; break;
            case 0x01: case 0x03: md = IZX; break;
            case 0x02: md = row >= 4 && op != 0x92 && op != 0xB2 && op != 0xD2 && op != 0xF2 ? IMM : JAM; break;
            case 0x04: case 0x05: case 0x06: case 0x07: md = ZP; break;
            case 0x08: case 0x18: case 0x1A: md = IMP; break;
            case 0x09: case 0x0B: md = IMM; break;
            case 0x0A: md = row < 4 ? ACC : IMP; break;
            case 0x0C: case 0x0D: case 0x0E: case 0x0F: md = op == 0x6C ? IND : ABS; break;
            case 0x10: md = REL; break;
            case 0x11: case 0x13: md = IZY; break;
            case 0x12: md = JAM; break;
            case 0x14: case 0x15: md = ZPX; break;
            case 0x16: case 0x17: md = row == 4 || row == 5 ? ZPY : ZPX; break;
            case 0x19: case 0x1B: md = ABY; break;
            case 0x1C: case 0x1D: md = ABX; break;
            case 0x1E: case 0x1F: md = row == 4 || row == 5 ? ABY : ABX; break;
            }
            m[op] = uint8_t(md);
        }
    }
};
const ModeTable kModes;

} // namespace

void Cpu6502::nz(uint8_t v) { p = uint8_t((p & ~(FN | FZ)) | (v & FN) | (v ? 0 : FZ)); }

uint16_t Cpu6502::ea(int mode, bool penalty) {
    switch (mode) {
    case ZP: return fetch();
    case ZPX: return uint8_t(fetch() + x);
    case ZPY: return uint8_t(fetch() + y);
    case ABS: return fetch16();
    case ABX: case ABY: {
        base_ = fetch16();
        uint16_t r = uint16_t(base_ + (mode == ABX ? x : y));
        if (penalty && ((base_ ^ r) & 0xFF00)) extra_++;
        return r;
    }
    case IZX: {
        uint8_t z = uint8_t(fetch() + x);
        return uint16_t(read(z) | read(uint8_t(z + 1)) << 8);
    }
    case IZY: {
        uint8_t z = fetch();
        base_ = uint16_t(read(z) | read(uint8_t(z + 1)) << 8);
        uint16_t r = uint16_t(base_ + y);
        if (penalty && ((base_ ^ r) & 0xFF00)) extra_++;
        return r;
    }
    case IND: {
        uint16_t t = fetch16();   // the NMOS bug: the high byte from the same page
        return uint16_t(read(t) | read(uint16_t((t & 0xFF00) | ((t + 1) & 0xFF))) << 8);
    }
    }
    return 0;
}

void Cpu6502::adc(uint8_t v) {
    int c = p & FC;
    if (p & FD) {
        // the NMOS: Z from the binary sum, N and V from the intermediate result
        int al = (a & 0x0F) + (v & 0x0F) + c;
        if (al >= 0x0A) al = ((al + 0x06) & 0x0F) + 0x10;
        int r = (a & 0xF0) + (v & 0xF0) + al;
        p &= uint8_t(~(FN | FV | FZ | FC));
        if (((a + v + c) & 0xFF) == 0) p |= FZ;
        if (r & 0x80) p |= FN;
        if (~(a ^ v) & (a ^ r) & 0x80) p |= FV;
        if (r >= 0xA0) r += 0x60;
        if (r >= 0x100) p |= FC;
        a = uint8_t(r);
    } else {
        int r = a + v + c;
        p &= uint8_t(~(FV | FC));
        if (~(a ^ v) & (a ^ r) & 0x80) p |= FV;
        if (r > 0xFF) p |= FC;
        a = uint8_t(r);
        nz(a);
    }
}

void Cpu6502::sbc(uint8_t v) {
    int c = p & FC;
    int r = a - v - (1 - c);   // the flags are the binary subtraction's, decimal or not
    p &= uint8_t(~(FV | FC));
    if ((a ^ v) & (a ^ r) & 0x80) p |= FV;
    if (r >= 0) p |= FC;
    uint8_t bin = uint8_t(r);
    if (p & FD) {
        int al = (a & 0x0F) - (v & 0x0F) + c - 1;
        if (al < 0) al = ((al - 0x06) & 0x0F) - 0x10;
        int d = (a & 0xF0) - (v & 0xF0) + al;
        if (d < 0) d -= 0x60;
        a = uint8_t(d);
    } else
        a = bin;
    nz(bin);
}

void Cpu6502::cmp(uint8_t r, uint8_t v) {
    int t = r - v;
    p = uint8_t((p & ~FC) | (t >= 0 ? FC : 0));
    nz(uint8_t(t));
}

void Cpu6502::branch(bool cond) {
    int8_t off = int8_t(fetch());
    if (!cond) return;
    uint16_t t = uint16_t(pc + off);
    extra_ += (t ^ pc) & 0xFF00 ? 2 : 1;
    pc = t;
}

int Cpu6502::step() {
    if (jammed) return 0;
    uint16_t at = pc;
    uint8_t op = fetch();
    int mode = kModes.m[op];
    extra_ = 0;
    // operand helpers: a read (page crossing counted), an address for a store or RMW
    auto rd = [&]() -> uint8_t { return mode == IMM ? fetch() : read(ea(mode, true)); };
    auto addr = [&]() -> uint16_t { return ea(mode, false); };
    auto rmw = [&](auto f) {
        if (mode == ACC) { a = f(a); return a; }
        uint16_t t = addr();
        uint8_t v = f(read(t));
        write(t, v);
        return v;
    };
    auto asl = [&](uint8_t v) { p = uint8_t((p & ~FC) | (v >> 7)); v <<= 1; nz(v); return v; };
    auto lsr = [&](uint8_t v) { p = uint8_t((p & ~FC) | (v & 1)); v >>= 1; nz(v); return v; };
    auto rol = [&](uint8_t v) { uint8_t c = p & FC; p = uint8_t((p & ~FC) | (v >> 7)); v = uint8_t(v << 1 | c); nz(v); return v; };
    auto ror = [&](uint8_t v) { uint8_t c = p & FC; p = uint8_t((p & ~FC) | (v & 1)); v = uint8_t(v >> 1 | c << 7); nz(v); return v; };
    auto inc = [&](uint8_t v) { v++; nz(v); return v; };
    auto dec = [&](uint8_t v) { v--; nz(v); return v; };
    // SHA/SHX/SHY/TAS: reg & (the base's high byte + 1) (the page-crossing address quirk not modelled)
    auto shStore = [&](uint8_t v) { uint16_t t = addr(); write(t, uint8_t(v & ((base_ >> 8) + 1))); };

    switch (op) {
    // loads, stores, transfers
    case 0xA9: case 0xA5: case 0xB5: case 0xAD: case 0xBD: case 0xB9: case 0xA1: case 0xB1: a = rd(); nz(a); break;
    case 0xA2: case 0xA6: case 0xB6: case 0xAE: case 0xBE: x = rd(); nz(x); break;
    case 0xA0: case 0xA4: case 0xB4: case 0xAC: case 0xBC: y = rd(); nz(y); break;
    case 0x85: case 0x95: case 0x8D: case 0x9D: case 0x99: case 0x81: case 0x91: write(addr(), a); break;
    case 0x86: case 0x96: case 0x8E: write(addr(), x); break;
    case 0x84: case 0x94: case 0x8C: write(addr(), y); break;
    case 0xAA: x = a; nz(x); break;
    case 0xA8: y = a; nz(y); break;
    case 0x8A: a = x; nz(a); break;
    case 0x98: a = y; nz(a); break;
    case 0xBA: x = s; nz(x); break;
    case 0x9A: s = x; break;
    // the stack
    case 0x48: push(a); break;
    case 0x08: push(p | FB | FU); break;
    case 0x68: a = pull(); nz(a); break;
    case 0x28: p = uint8_t((pull() | FU) & ~FB); break;
    // logic and arithmetic
    case 0x09: case 0x05: case 0x15: case 0x0D: case 0x1D: case 0x19: case 0x01: case 0x11: a |= rd(); nz(a); break;
    case 0x29: case 0x25: case 0x35: case 0x2D: case 0x3D: case 0x39: case 0x21: case 0x31: a &= rd(); nz(a); break;
    case 0x49: case 0x45: case 0x55: case 0x4D: case 0x5D: case 0x59: case 0x41: case 0x51: a ^= rd(); nz(a); break;
    case 0x69: case 0x65: case 0x75: case 0x6D: case 0x7D: case 0x79: case 0x61: case 0x71: adc(rd()); break;
    case 0xE9: case 0xEB: case 0xE5: case 0xF5: case 0xED: case 0xFD: case 0xF9: case 0xE1: case 0xF1: sbc(rd()); break;
    case 0xC9: case 0xC5: case 0xD5: case 0xCD: case 0xDD: case 0xD9: case 0xC1: case 0xD1: cmp(a, rd()); break;
    case 0xE0: case 0xE4: case 0xEC: cmp(x, rd()); break;
    case 0xC0: case 0xC4: case 0xCC: cmp(y, rd()); break;
    case 0x24: case 0x2C: {
        uint8_t v = rd();
        p = uint8_t((p & ~(FN | FV | FZ)) | (v & (FN | FV)) | ((a & v) ? 0 : FZ));
        break;
    }
    // shifts, increments
    case 0x0A: case 0x06: case 0x16: case 0x0E: case 0x1E: rmw(asl); break;
    case 0x4A: case 0x46: case 0x56: case 0x4E: case 0x5E: rmw(lsr); break;
    case 0x2A: case 0x26: case 0x36: case 0x2E: case 0x3E: rmw(rol); break;
    case 0x6A: case 0x66: case 0x76: case 0x6E: case 0x7E: rmw(ror); break;
    case 0xE6: case 0xF6: case 0xEE: case 0xFE: rmw(inc); break;
    case 0xC6: case 0xD6: case 0xCE: case 0xDE: rmw(dec); break;
    case 0xE8: x++; nz(x); break;
    case 0xC8: y++; nz(y); break;
    case 0xCA: x--; nz(x); break;
    case 0x88: y--; nz(y); break;
    // flags
    case 0x18: p &= uint8_t(~FC); break;
    case 0x38: p |= FC; break;
    case 0x58: p &= uint8_t(~FI); break;
    case 0x78: p |= FI; break;
    case 0xB8: p &= uint8_t(~FV); break;
    case 0xD8: p &= uint8_t(~FD); break;
    case 0xF8: p |= FD; break;
    // branches
    case 0x10: branch(!(p & FN)); break;
    case 0x30: branch(p & FN); break;
    case 0x50: branch(!(p & FV)); break;
    case 0x70: branch(p & FV); break;
    case 0x90: branch(!(p & FC)); break;
    case 0xB0: branch(p & FC); break;
    case 0xD0: branch(!(p & FZ)); break;
    case 0xF0: branch(p & FZ); break;
    // jumps
    case 0x4C: case 0x6C: pc = addr(); break;
    case 0x20: { uint16_t t = fetch16(); uint16_t r = uint16_t(pc - 1); push(uint8_t(r >> 8)); push(uint8_t(r)); pc = t; break; }
    case 0x60: { uint8_t l = pull(); pc = uint16_t((l | pull() << 8) + 1); break; }
    case 0x40: { p = uint8_t((pull() | FU) & ~FB); uint8_t l = pull(); pc = uint16_t(l | pull() << 8); break; }
    case 0x00: {
        pc++;
        push(uint8_t(pc >> 8)); push(uint8_t(pc)); push(p | FB | FU);
        p |= FI;
        pc = uint16_t(read(0xFFFE) | read(0xFFFF) << 8);
        break;
    }
    case 0xEA: break;

    // undocumented: the stable ones
    case 0x07: case 0x17: case 0x0F: case 0x1F: case 0x1B: case 0x03: case 0x13:   // SLO
        a |= rmw(asl); nz(a); break;
    case 0x27: case 0x37: case 0x2F: case 0x3F: case 0x3B: case 0x23: case 0x33:   // RLA
        a &= rmw(rol); nz(a); break;
    case 0x47: case 0x57: case 0x4F: case 0x5F: case 0x5B: case 0x43: case 0x53:   // SRE
        a ^= rmw(lsr); nz(a); break;
    case 0x67: case 0x77: case 0x6F: case 0x7F: case 0x7B: case 0x63: case 0x73:   // RRA
        adc(rmw(ror)); break;
    case 0xC7: case 0xD7: case 0xCF: case 0xDF: case 0xDB: case 0xC3: case 0xD3:   // DCP
        cmp(a, rmw([](uint8_t v) { return uint8_t(v - 1); })); break;
    case 0xE7: case 0xF7: case 0xEF: case 0xFF: case 0xFB: case 0xE3: case 0xF3:   // ISC
        sbc(rmw([](uint8_t v) { return uint8_t(v + 1); })); break;
    case 0x87: case 0x97: case 0x8F: case 0x83: write(addr(), a & x); break;         // SAX
    case 0xA7: case 0xB7: case 0xAF: case 0xBF: case 0xA3: case 0xB3:               // LAX
        a = x = rd(); nz(a); break;
    case 0x0B: case 0x2B: a &= fetch(); nz(a); p = uint8_t((p & ~FC) | (a >> 7)); break;   // ANC
    case 0x4B: a &= fetch(); p = uint8_t((p & ~FC) | (a & 1)); a >>= 1; nz(a); break;     // ALR
    case 0x6B: {                                                                        // ARR
        uint8_t t = a & fetch(), c = p & FC;
        uint8_t r = uint8_t(t >> 1 | c << 7);
        if (p & FD) {
            p = uint8_t((p & ~(FN | FV | FZ | FC)) | (c ? FN : 0) | (r ? 0 : FZ) | ((t ^ r) & FV));
            if ((t & 0x0F) + (t & 0x01) > 5) r = uint8_t((r & 0xF0) | ((r + 6) & 0x0F));
            if ((t & 0xF0) + (t & 0x10) > 0x50) { r = uint8_t((r & 0x0F) | ((r + 0x60) & 0xF0)); p |= FC; }
        } else {
            nz(r);
            p = uint8_t((p & ~(FV | FC)) | ((r >> 6) & 1) | (((r >> 6) ^ (r >> 5)) & 1 ? FV : 0));
        }
        a = r;
        break;
    }
    case 0xCB: { int t = (a & x) - fetch(); x = uint8_t(t); p = uint8_t((p & ~FC) | (t >= 0 ? FC : 0)); nz(x); break; }  // SBX
    // NOPs
    case 0x1A: case 0x3A: case 0x5A: case 0x7A: case 0xDA: case 0xFA: break;
    case 0x80: case 0x82: case 0x89: case 0xC2: case 0xE2: fetch(); break;
    case 0x04: case 0x44: case 0x64: case 0x14: case 0x34: case 0x54: case 0x74: case 0xD4: case 0xF4:
    case 0x0C: case 0x1C: case 0x3C: case 0x5C: case 0x7C: case 0xDC: case 0xFC: rd(); break;
    // undocumented: the unstable ones, approximated
    case 0x8B: a = uint8_t((a | 0xEE) & x & fetch()); nz(a); break;                 // XAA/ANE
    case 0xAB: a = x = uint8_t((a | 0xEE) & fetch()); nz(a); break;                 // LAX #/LXA
    case 0xBB: a = x = s = uint8_t(rd() & s); nz(a); break;                         // LAS
    case 0x93: case 0x9F: shStore(a & x); break;                                    // SHA/AHX
    case 0x9E: shStore(x); break;                                                   // SHX
    case 0x9C: shStore(y); break;                                                   // SHY
    case 0x9B: s = a & x; shStore(s); break;                                        // TAS
    default:                                                                        // JAM/KIL
        jammed = true;
        pc = at;
        return 0;
    }
    int n = kCycles[op] + extra_;
    cycles += uint64_t(n);
    return n;
}

// ------------------------------------------------------------ the file

namespace {

int be16(const std::vector<uint8_t>& f, size_t at) { return f[at] << 8 | f[at + 1]; }

std::string latin1(const std::vector<uint8_t>& f, size_t at, size_t n) {
    std::string s;
    for (size_t i = at; i < at + n && i < f.size() && f[i]; i++) {
        uint8_t c = f[i];
        if (c < 0x80) s += char(c);
        else { s += char(0xC0 | c >> 6); s += char(0x80 | (c & 0x3F)); }
    }
    return s;
}

std::string hex4(unsigned v) { char b[8]; std::snprintf(b, sizeof b, "$%04X", v & 0xFFFF); return b; }
std::string hex2(unsigned v) { char b[8]; std::snprintf(b, sizeof b, "$%02X", v & 0xFF); return b; }

} // namespace

bool readPsid(const std::vector<uint8_t>& f, PsidFile& p, std::string& error) {
    p = PsidFile();
    if (f.size() < 0x76) { error = "not a .sid file: too short for a PSID header"; return false; }
    p.magic.assign(f.begin(), f.begin() + 4);
    if (p.magic != "PSID" && p.magic != "RSID") { error = "not a .sid file: no PSID or RSID tag"; return false; }
    p.version = be16(f, 4);
    if (p.version < 1 || p.version > 4 || (p.rsid() && p.version < 2)) {
        error = p.magic + " version " + std::to_string(p.version) + ": not supported";
        return false;
    }
    p.dataOffset = uint16_t(be16(f, 6));
    if (p.dataOffset < (p.version == 1 ? 0x76 : 0x7C) || p.dataOffset > f.size()) {
        error = "the data offset " + std::to_string(p.dataOffset) + " is out of the file";
        return false;
    }
    p.load = uint16_t(be16(f, 8));
    p.init = uint16_t(be16(f, 0x0A));
    p.play = uint16_t(be16(f, 0x0C));
    p.songs = be16(f, 0x0E);
    p.startSong = be16(f, 0x10);
    p.speed = uint32_t(f[0x12]) << 24 | uint32_t(f[0x13]) << 16 | uint32_t(f[0x14]) << 8 | f[0x15];
    p.name = latin1(f, 0x16, 32);
    p.author = latin1(f, 0x36, 32);
    p.released = latin1(f, 0x56, 32);
    if (p.version >= 2) {
        p.flags = uint16_t(be16(f, 0x76));
        p.startPage = f[0x78];
        p.pageLength = f[0x79];
        auto sidAt = [](uint8_t b) -> uint16_t {   // valid: $D420-$D7E0 and $DE00-$DFE0, even xx
            if ((b & 1) || !((b >= 0x42 && b <= 0x7F) || (b >= 0xE0 && b <= 0xFE))) return 0;
            return uint16_t(0xD000 | b << 4);
        };
        if (p.version >= 3) p.sid2 = sidAt(f[0x7A]);
        if (p.version >= 4) p.sid3 = sidAt(f[0x7B]);
    }
    size_t at = p.dataOffset;
    if (p.load == 0) {
        if (f.size() < at + 2) { error = "no load address in the data"; return false; }
        p.load = uint16_t(f[at] | f[at + 1] << 8);
        at += 2;
    } else if (p.rsid()) {
        error = "an RSID's header load address must be 0";
        return false;
    }
    p.data.assign(f.begin() + long(at), f.end());
    if (p.data.empty()) { error = "no C64 data in the file"; return false; }
    if (p.load + p.data.size() > 0x10000) {
        error = "the data (" + std::to_string(p.data.size()) + " bytes at " + hex4(p.load) + ") runs past $FFFF";
        return false;
    }
    if (p.init == 0 && !p.rsid()) p.init = p.load;
    if (p.songs < 1) p.songs = 1;
    if (p.songs > 256) p.songs = 256;
    if (p.startSong < 1 || p.startSong > p.songs) p.startSong = 1;
    return true;
}

// ------------------------------------------------------------ the C64

namespace {

const double kPalClock = 985248, kNtscClock = 1022727;
const uint16_t kSentinel = 0xFFFF;   // init/play return here (a stop, never run)

class C64 : public Cpu6502 {
public:
    bool ntsc = false;
    uint16_t sid2 = 0, sid3 = 0;
    std::vector<uint8_t> known = std::vector<uint8_t>(0x10000, 0);   // 1 loaded/written, 2 a KERNAL stub
    std::array<uint8_t, 0x20> sidRegs{};
    std::array<uint8_t, 0x40> vic{};
    std::array<uint8_t, 0x10> cia[2]{};
    uint16_t latchA[2]{}, latchB[2]{};
    uint8_t ciaMask[2]{};
    uint32_t lfsr = 0x7FFFF8;
    SidCall* rec = nullptr;

    bool ioVisible() const {
        uint8_t port = uint8_t((mem[1] | ~mem[0]) & 7);   // inputs read as pulled up
        return (port & 4) && (port & 3);
    }
    bool kernalVisible() const { return ((mem[1] | ~mem[0]) & 2) != 0; }
    int sidChip(uint16_t addr) const {
        if (sid2 && addr >= sid2 && addr < sid2 + 0x20) return 1;
        if (sid3 && addr >= sid3 && addr < sid3 + 0x20) return 2;
        if (addr >= 0xD400 && addr < 0xD800) return 0;
        return -1;
    }
    int rasterLine() const {
        int perLine = ntsc ? 65 : 63, lines = ntsc ? 263 : 312;
        return int((cycles / uint64_t(perLine)) % uint64_t(lines));
    }
    uint16_t timer(int c, int t) const {   // a CIA timer's counter: counting down from its latch while it runs
        uint16_t latch = t ? latchB[c] : latchA[c];
        bool running = cia[c][t ? 0x0F : 0x0E] & 1;
        if (!running) return latch;
        return uint16_t(latch - cycles % (uint64_t(latch) + 1));
    }

protected:
    uint8_t read(uint16_t addr) override {
        if (addr < 0xD000 || addr >= 0xE000 || !ioVisible()) return mem[addr];
        if (addr < 0xD400) {
            int r = addr & 0x3F;
            if (r == 0x11) return uint8_t((vic[0x11] & 0x7F) | (rasterLine() & 0x100 ? 0x80 : 0));
            if (r == 0x12) return uint8_t(rasterLine());
            if (r == 0x19) return 0xF1;
            return vic[r];
        }
        int chip = sidChip(addr);
        if (chip >= 0) {
            int r = addr & 0x1F;
            if (r == 0x19 || r == 0x1A) return 0xFF;
            if (r == 0x1B) {   // osc 3: a 23-bit noise LFSR, stepped each read
                for (int i = 0; i < 8; i++) {
                    uint32_t bit = ((lfsr >> 22) ^ (lfsr >> 17)) & 1;
                    lfsr = ((lfsr << 1) | bit) & 0x7FFFFF;
                }
                return uint8_t(lfsr >> 15);
            }
            return 0;   // write-only (and env 3: 0)
        }
        if (addr >= 0xDC00 && addr < 0xDE00) {
            int c = addr >= 0xDD00, r = addr & 0x0F;
            switch (r) {
            case 0x04: return uint8_t(timer(c, 0));
            case 0x05: return uint8_t(timer(c, 0) >> 8);
            case 0x06: return uint8_t(timer(c, 1));
            case 0x07: return uint8_t(timer(c, 1) >> 8);
            case 0x0D: return 0x81;
            default: return cia[c][r];
            }
        }
        return mem[addr];   // colour RAM, $DE00-$DFFF
    }

    void write(uint16_t addr, uint8_t v) override {
        if (addr < 0xD000 || addr >= 0xE000 || !ioVisible()) {
            mem[addr] = v;
            known[addr] = 1;
            return;
        }
        int chip = sidChip(addr);
        if (chip >= 0) {
            int r = addr & 0x1F;
            if (chip == 0) sidRegs[r] = v;
            if (rec && r <= 0x1C) rec->writes.push_back(SidWrite{uint8_t(r), v, uint8_t(chip)});
            return;
        }
        if (addr < 0xD400) { vic[addr & 0x3F] = v; return; }
        if (addr >= 0xDC00 && addr < 0xDE00) {
            int c = addr >= 0xDD00, r = addr & 0x0F;
            switch (r) {
            case 0x04: latchA[c] = uint16_t((latchA[c] & 0xFF00) | v); break;
            case 0x05: latchA[c] = uint16_t((latchA[c] & 0x00FF) | v << 8); break;
            case 0x06: latchB[c] = uint16_t((latchB[c] & 0xFF00) | v); break;
            case 0x07: latchB[c] = uint16_t((latchB[c] & 0x00FF) | v << 8); break;
            case 0x0D: ciaMask[c] = v & 0x80 ? uint8_t(ciaMask[c] | (v & 0x1F)) : uint8_t(ciaMask[c] & ~v); break;
            }
            cia[c][r] = v;
            return;
        }
        mem[addr] = v;
    }

public:
    void poke(uint16_t addr, uint8_t v, uint8_t kind) { mem[addr] = v; known[addr] = kind; }
    void stub(uint16_t addr, std::initializer_list<uint8_t> code) {   // only where the tune loaded nothing
        uint16_t a2 = addr;
        for (size_t i = 0; i < code.size(); i++) if (known[uint16_t(a2 + i)]) return;
        for (uint8_t b : code) poke(addr++, b, 2);
    }
    bool isStub(uint16_t addr) const { return known[addr] == 2; }
    bool isExit(uint16_t addr) const {
        return (addr == 0xEA31 || addr == 0xEA81 || addr == 0xFEBC) && isStub(addr);
    }
    uint16_t vec(uint16_t at) const { return uint16_t(mem[at] | mem[uint16_t(at + 1)] << 8); }

    enum class Call { Jsr, Irq, KernalIrq };
    // call `to` as a subroutine or an interrupt handler, recording into `call`
    bool run(uint16_t to, Call how, uint64_t limit, SidCall& call, const std::string& what, std::string& error) {
        s = 0xFF;
        p = uint8_t(p | FI | FU) & uint8_t(~(FD | FB));
        if (how == Call::Jsr) {
            push(uint8_t((kSentinel - 1) >> 8)); push(uint8_t(kSentinel - 1));
        } else {
            push(uint8_t(kSentinel >> 8)); push(uint8_t(kSentinel)); push(uint8_t(p | FU));
            if (how == Call::KernalIrq) { push(a); push(x); push(y); }
        }
        uint8_t s0 = 0xFF;
        pc = to;
        rec = &call;
        uint64_t start = cycles;
        bool ok = true;
        for (;;) {
            if (cycles - start > limit) {
                error = what + " ran past " + std::to_string(limit) + " cycles (at " + hex4(pc) + "): it does not return";
                ok = false;
                break;
            }
            if (how != Call::Jsr && isExit(pc)) break;
            if (!known[pc] && pc != kSentinel) {
                error = what + " jumps to " + hex4(pc) + ", where nothing was loaded (a C64 ROM routine? not emulated)";
                ok = false;
                break;
            }
            uint8_t op = mem[pc];
            uint16_t at = pc;
            step();
            if (jammed) {
                error = what + ": the JAM opcode " + hex2(op) + " at " + hex4(at);
                ok = false;
                break;
            }
            if ((op == 0x60 || op == 0x40) && pc == kSentinel && s == s0) break;
        }
        call.cycles = uint32_t(cycles - start);
        rec = nullptr;
        return ok;
    }
};

} // namespace

bool runPsid(const PsidFile& tune, int song, int calls, SidRun& out, std::string& error) {
    out = SidRun();
    if (tune.mus()) { error = "Compute!'s Sidplayer MUS data needs its player: not supported"; return false; }
    if (tune.basic()) { error = "an RSID in C64 BASIC needs a real C64: not supported"; return false; }
    if (tune.rsid() && tune.init == 0) { error = "an RSID without an init address needs a real C64: not supported"; return false; }
    if (song < 1 || song > tune.songs) {
        error = "song " + std::to_string(song) + ": the tune has " + std::to_string(tune.songs);
        return false;
    }
    if (tune.data.empty() || tune.load + tune.data.size() > 0x10000) { error = "no data, or past $FFFF"; return false; }

    auto m = std::make_unique<C64>();
    C64& c = *m;
    c.ntsc = tune.ntsc();
    c.sid2 = tune.sid2;
    c.sid3 = tune.sid3;
    out.clock = c.ntsc ? kNtscClock : kPalClock;
    for (size_t i = 0; i < tune.data.size(); i++) c.poke(uint16_t(tune.load + i), tune.data[i], 1);

    // the KERNAL, where the tune is not
    for (uint16_t at = 0xFF81; at <= 0xFFF3; at += 3) c.stub(at, {0x60});
    c.stub(0xFF48, {0x48, 0x8A, 0x48, 0x98, 0x48, 0xBA, 0xBD, 0x04, 0x01, 0x29, 0x10,
                    0xF0, 0x03, 0x6C, 0x16, 0x03, 0x6C, 0x14, 0x03});
    c.stub(0xEA81, {0x68, 0xA8, 0x68, 0xAA, 0x68, 0x40});
    c.stub(0xFEBC, {0x68, 0xA8, 0x68, 0xAA, 0x68, 0x40});
    c.stub(0xEA31, {0x4C, 0x81, 0xEA});
    c.stub(0xFE66, {0x4C, 0x81, 0xEA});   // BRK: as the IRQ exit
    c.stub(0xFE43, {0x40});               // NMI: RTI
    c.stub(0xFFFA, {0x43, 0xFE, 0xE2, 0xFC, 0x48, 0xFF});
    c.stub(0x0314, {0x31, 0xEA, 0x66, 0xFE, 0x47, 0xFE});
    c.stub(0x02A6, {uint8_t(c.ntsc ? 0 : 1)});
    // RAM's vectors stubs as above are "known"; the rest of the low pages too (data, never code)
    for (int i = 0; i < 0x400; i++) if (!c.known[i]) c.known[i] = 1;

    // the bank: as sidplay sets it for a PSID
    uint16_t hi = tune.init > tune.play ? tune.init : tune.play;
    uint8_t bank = 0x37;
    if (hi >= 0xE000) bank = 0x35;
    else if (hi >= 0xD000) bank = 0x34;
    else if (hi >= 0xA000) bank = 0x36;
    c.poke(0, 0x2F, 1);
    c.poke(1, tune.rsid() ? 0x37 : bank, 1);

    // the timers as the KERNAL leaves them; the raster or CIA interrupt per the speed bit
    bool ciaSong = !tune.rsid() && tune.songUsesCia(song);
    c.latchA[0] = c.ntsc ? 0x4295 : 0x4025;
    c.cia[0][0x0E] = 0x11;
    c.ciaMask[0] = ciaSong || tune.rsid() ? 0x01 : 0x00;
    c.vic[0x1A] = ciaSong || tune.rsid() ? 0x00 : 0x01;
    c.vic[0x11] = 0x1B;
    c.vic[0x12] = 0x37;

    // init
    c.a = uint8_t(song - 1);
    c.x = c.y = 0;
    c.p = 0x24;
    std::string what = tune.rsid() ? "the RSID's init" : "init";
    if (!c.run(tune.init, C64::Call::Jsr, kSidInitCycleLimit, out.init, what + " (" + hex4(tune.init) + ")", error)) {
        if (tune.rsid()) error += "; an RSID that does not return needs a real C64: not supported";
        return false;
    }
    out.initRegs = c.sidRegs;

    // the play routine and its rate
    bool cia;
    if (tune.play != 0) {
        cia = ciaSong;
    } else {
        bool ciaOn = (c.cia[0][0x0E] & 1) && (c.ciaMask[0] & 1);
        bool vicOn = c.vic[0x1A] & 1;
        cia = ciaOn != vicOn ? ciaOn : ciaSong;
        out.irq = true;
    }
    out.cia = cia;
    if (cia) {
        out.ciaLatch = c.latchA[0];
        out.callsPerSecond = out.clock / (double(c.latchA[0]) + 1);
    } else
        out.callsPerSecond = c.ntsc ? 60 : 50;

    // where the interrupt goes: the hardware vector when the KERNAL is banked out and init set it; else $0314
    auto handler = [&](uint16_t& to, C64::Call& how) -> bool {
        uint16_t hw = c.vec(0xFFFE), sw = c.vec(0x0314);
        bool hwSet = !(hw == 0xFF48 && c.isStub(0xFF48));
        bool swSet = !c.isExit(sw);
        if (hwSet && !c.kernalVisible()) { to = hw; how = C64::Call::Irq; return true; }
        if (swSet) { to = sw; how = C64::Call::KernalIrq; return true; }
        if (hwSet) { to = hw; how = C64::Call::Irq; return true; }
        return false;
    };

    uint64_t period = uint64_t(out.clock / out.callsPerSecond + 0.5);
    uint64_t t0 = (c.cycles / period + 1) * period;
    for (int i = 0; i < calls; i++) {
        uint16_t to = tune.play;
        C64::Call how = C64::Call::Jsr;
        if (tune.play == 0 && !handler(to, how)) {
            error = "no play routine: play is 0 and " + std::string(i ? "the tune's handler" : "init") +
                    " left no interrupt handler ($0314 and $FFFE are the KERNAL's)";
            return false;
        }
        if (i == 0) out.playAddress = to;
        c.cycles = t0 + uint64_t(i) * period;
        c.p = 0x24;
        out.calls.emplace_back();
        if (!c.run(to, how, kSidPlayCycleLimit, out.calls.back(), "play call " + std::to_string(i + 1) + " (" + hex4(to) + ")", error))
            return false;
    }
    return true;
}
