#include <iostream>
#include "Logger.hpp"
#include <fstream>
#include <sstream>
#include "MiniJson.hpp"
#include <vector>
#include <string>
#include <iomanip>
#include <algorithm>
#include <map>
#include <cstdio>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <cctype>
#include "O45Reader.hpp"
#include "O45OverlayFile.hpp"
#include "O45Linker.hpp"
#include "O45FormatUtil.hpp"
#include "AssemblerOpcodeDatabase.hpp"
#include "AssemblerTypes.hpp"
#include "BasicTokenizer.hpp"
#include "BasicEmitter.hpp"
#include "Version.hpp"

// An ifstream opens a directory but throws on the first read.
static bool isDirectory(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

// TODO: Add binary diff mode (-D) for comparing two .prg/.o45 files.
// Shows byte-level differences at matching symbol addresses, highlighting
// relocation mismatches (e.g., __sp_base or __zp_scratch resolved differently
// between direct-compile and linked builds). Usage:
//   objdump45 -D direct.prg linked.prg
// For each shared symbol, show side-by-side disassembly of the first N
// instructions with differing bytes highlighted.

// ─── Helpers shared with nm45 ───────────────────────────────────────────────

static const char* segmentName(uint8_t seg) {
    switch (seg) {
        case SEG_EXTERNAL: return "EXTERN";
        case SEG_ABSOLUTE: return "ABS";
        case SEG_TEXT: return "TEXT";
        case SEG_DATA: return "DATA";
        case SEG_BSS:  return "BSS";
        case SEG_ZP:   return "ZP";
        default:       return "???";
    }
}

static char segmentLetter(uint8_t seg) {
    switch (seg) {
        case SEG_TEXT: return 'T';
        case SEG_DATA: return 'D';
        case SEG_BSS:  return 'B';
        case SEG_ZP:   return 'Z';
        default:       return 'A';
    }
}

static const char* relocTypeName(uint8_t rtype) {
    // Phase 78: Check for immediate relocation types first (stored in low bits)
    if ((rtype & ~O45_RSEG_MASK) == 0) {
        switch (rtype) {
            case R_IMM8:  return "R_IMM8";
            case R_IMM16: return "R_IMM16";
            default: break;
        }
    }

    switch (rtype & O45_RTYPE_MASK) {
        case R_LOW:      return "R_LOW";
        case R_HIGH:     return "R_HIGH";
        case R_LOWX:     return "R_LOWX";
        case R_WORD:     return "R_WORD";
        case R_LINEAR24: return "R_LINEAR24";
        case R_LINEAR32: return "R_LINEAR32";
        case R_SEGADR:   return "R_SEGADR";
        default:         return "R_???";
    }
}

static const char* cpuName(uint8_t id) {
    switch (id) {
        case 0x00: return "6502";
        case 0x01: return "6502";
        case 0x02: return "65C02";
        case 0x03: return "65CE02";
        case 0x04: return "65816";
        case 0x45: return "45GS02";
        default:   return "unknown";
    }
}

// ─── Reverse opcode table for disassembly ───────────────────────────────────

struct OpcodeInfo {
    std::string mnemonic;
    AddressingMode mode;
    bool undocumented = false;  // a 6510 opcode outside the documented set
};

// The CPU objdump45 decodes for (#144): the 45GS02 (the default), the 65CE02
// (no quad or flat-memory prefixes; $5C is AUG, $EA NOP), the 6502 (the
// documented opcodes only) and the 6510 (with the NMOS undocumented ones)
enum class DisCpu { CPU45GS02, CPU65CE02, CPU6502, CPU6510 };

static bool parseDisCpu(std::string name, DisCpu& cpu) {
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    if (name == "45gs02" || name == "4510" || name == "m65") cpu = DisCpu::CPU45GS02;
    else if (name == "65ce02") cpu = DisCpu::CPU65CE02;
    else if (name == "6502") cpu = DisCpu::CPU6502;
    else if (name == "6510" || name == "6502x") cpu = DisCpu::CPU6510;
    else return false;
    return true;
}

static const char* disCpuName(DisCpu cpu) {
    switch (cpu) {
        case DisCpu::CPU65CE02: return "65ce02";
        case DisCpu::CPU6502:   return "6502";
        case DisCpu::CPU6510:   return "6510";
        default:                return "45gs02";
    }
}

// Returns the number of operand bytes for a given addressing mode
static int operandSize(AddressingMode mode) {
    switch (mode) {
        case AddressingMode::IMPLIED:
        case AddressingMode::ACCUMULATOR:
            return 0;
        case AddressingMode::IMMEDIATE:
        case AddressingMode::BASE_PAGE:
        case AddressingMode::BASE_PAGE_X:
        case AddressingMode::BASE_PAGE_Y:
        case AddressingMode::BASE_PAGE_X_INDIRECT:
        case AddressingMode::BASE_PAGE_INDIRECT_Y:
        case AddressingMode::BASE_PAGE_INDIRECT_Z:
        case AddressingMode::BASE_PAGE_INDIRECT_SP_Y:
        case AddressingMode::FLAT_INDIRECT_Z:
        case AddressingMode::RELATIVE:
        case AddressingMode::STACK_RELATIVE:
            return 1;
        case AddressingMode::IMMEDIATE16:
        case AddressingMode::ABSOLUTE:
        case AddressingMode::ABSOLUTE_X:
        case AddressingMode::ABSOLUTE_Y:
        case AddressingMode::ABSOLUTE_INDIRECT:
        case AddressingMode::ABSOLUTE_X_INDIRECT:
        case AddressingMode::RELATIVE16:
        case AddressingMode::STACK_RELATIVE_INDIRECT_Y:
            return 2;
        case AddressingMode::BASE_PAGE_RELATIVE:
            return 2; // zp byte + rel byte
        case AddressingMode::LINEAR_ABSOLUTE:
        case AddressingMode::LINEAR_ABSOLUTE_X:
        case AddressingMode::LINEAR_ABSOLUTE_Y:
            return 3;
        default:
            return 0;
    }
}

// Build reverse lookup: opcode byte → (mnemonic, mode)
// For duplicate opcodes (e.g. BASE_PAGE_INDIRECT_Z vs FLAT_INDIRECT_Z),
// we prefer the more common form.
static std::map<uint8_t, OpcodeInfo> buildReverseOpcodeTable() {
    std::map<uint8_t, OpcodeInfo> table;
    const auto& opcodeMap = AssemblerOpcodeDatabase::getOpcodeMap();

    // First pass: populate all entries
    for (const auto& entry : opcodeMap) {
        uint8_t opcode = entry.second;
        const std::string& mnemonic = entry.first.first;
        AddressingMode mode = entry.first.second;

        // Skip FLAT_INDIRECT_Z if we already have BASE_PAGE_INDIRECT_Z for same opcode
        // (they share opcodes; prefer the (zp),Z syntax)
        auto it = table.find(opcode);
        if (it != table.end()) {
            // Prefer non-flat form, and prefer non-Q form (ldq/stq share opcodes with lda/sta)
            if (mode == AddressingMode::FLAT_INDIRECT_Z) continue;
            if (mnemonic == "ldq" || mnemonic == "stq") continue;
        }
        table[opcode] = {mnemonic, mode};
    }

    return table;
}

// The NMOS 6502's undocumented opcodes (the 6510's: 105, with the 151
// documented ones all 256), named as in the "NMOS 6510 Unintended Opcodes"
// reference (groepaz): the read-modify-write combinations, the immediate
// ones, the unstable stores, JAM and the NOPs
static void addUndocumented6510(std::map<uint8_t, OpcodeInfo>& table) {
    using M = AddressingMode;
    auto add = [&](uint8_t op, const char* mn, M mode) { table[op] = {mn, mode, true}; };
    // SLO RLA SRE RRA DCP ISC: zp, zp,x, abs, abs,x, abs,y, (zp,x), (zp),y
    const struct { const char* mn; uint8_t base; } rmw[] = {
        {"slo", 0x00}, {"rla", 0x20}, {"sre", 0x40}, {"rra", 0x60}, {"dcp", 0xC0}, {"isc", 0xE0}};
    for (const auto& r : rmw) {
        add(r.base + 0x07, r.mn, M::BASE_PAGE);
        add(r.base + 0x17, r.mn, M::BASE_PAGE_X);
        add(r.base + 0x0F, r.mn, M::ABSOLUTE);
        add(r.base + 0x1F, r.mn, M::ABSOLUTE_X);
        add(r.base + 0x1B, r.mn, M::ABSOLUTE_Y);
        add(r.base + 0x03, r.mn, M::BASE_PAGE_X_INDIRECT);
        add(r.base + 0x13, r.mn, M::BASE_PAGE_INDIRECT_Y);
    }
    add(0x87, "sax", M::BASE_PAGE); add(0x97, "sax", M::BASE_PAGE_Y);
    add(0x8F, "sax", M::ABSOLUTE);  add(0x83, "sax", M::BASE_PAGE_X_INDIRECT);
    add(0xA7, "lax", M::BASE_PAGE); add(0xB7, "lax", M::BASE_PAGE_Y);
    add(0xAF, "lax", M::ABSOLUTE);  add(0xBF, "lax", M::ABSOLUTE_Y);
    add(0xA3, "lax", M::BASE_PAGE_X_INDIRECT); add(0xB3, "lax", M::BASE_PAGE_INDIRECT_Y);
    add(0x0B, "anc", M::IMMEDIATE); add(0x2B, "anc", M::IMMEDIATE);
    add(0x4B, "alr", M::IMMEDIATE); add(0x6B, "arr", M::IMMEDIATE);
    add(0xCB, "sbx", M::IMMEDIATE); add(0xEB, "sbc", M::IMMEDIATE);
    add(0x8B, "ane", M::IMMEDIATE); add(0xAB, "lxa", M::IMMEDIATE);
    add(0x9F, "sha", M::ABSOLUTE_Y); add(0x93, "sha", M::BASE_PAGE_INDIRECT_Y);
    add(0x9E, "shx", M::ABSOLUTE_Y); add(0x9C, "shy", M::ABSOLUTE_X);
    add(0x9B, "tas", M::ABSOLUTE_Y); add(0xBB, "las", M::ABSOLUTE_Y);
    for (uint8_t op : {0x02, 0x12, 0x22, 0x32, 0x42, 0x52, 0x62, 0x72, 0x92, 0xB2, 0xD2, 0xF2})
        add(op, "jam", M::IMPLIED);
    for (uint8_t op : {0x1A, 0x3A, 0x5A, 0x7A, 0xDA, 0xFA}) add(op, "nop", M::IMPLIED);
    for (uint8_t op : {0x80, 0x82, 0x89, 0xC2, 0xE2}) add(op, "nop", M::IMMEDIATE);
    for (uint8_t op : {0x04, 0x44, 0x64}) add(op, "nop", M::BASE_PAGE);
    for (uint8_t op : {0x14, 0x34, 0x54, 0x74, 0xD4, 0xF4}) add(op, "nop", M::BASE_PAGE_X);
    add(0x0C, "nop", M::ABSOLUTE);
    for (uint8_t op : {0x1C, 0x3C, 0x5C, 0x7C, 0xDC, 0xFC}) add(op, "nop", M::ABSOLUTE_X);
}

// The opcode table of a CPU
static std::map<uint8_t, OpcodeInfo> buildOpcodeTable(DisCpu cpu) {
    std::map<uint8_t, OpcodeInfo> table = buildReverseOpcodeTable();
    if (cpu == DisCpu::CPU45GS02) return table;
    if (cpu == DisCpu::CPU65CE02) {
        // the 4510's MAP is the 65CE02's AUG (a 4-byte no-op), its EOM a NOP
        table[0x5C] = {"aug", AddressingMode::LINEAR_ABSOLUTE};
        table[0xEA] = {"nop", AddressingMode::IMPLIED};
        return table;
    }
    // the 6502: the 45GS02's opcodes that are the 6502's documented ones (ca45 --cpu 6502)
    std::map<uint8_t, OpcodeInfo> nmos;
    for (const auto& e : AssemblerOpcodeDatabase::getOpcodeMap()) {
        if (!AssemblerOpcodeDatabase::isOn6502(e.first.first, e.first.second)) continue;
        std::string mn = e.first.first;
        std::transform(mn.begin(), mn.end(), mn.begin(), ::tolower);
        if (mn == "eom") mn = "nop";
        nmos[e.second] = {mn, e.first.second};
    }
    if (cpu == DisCpu::CPU6510) addUndocumented6510(nmos);
    return nmos;
}

// Format an operand based on addressing mode and raw bytes
static std::string formatOperand(AddressingMode mode, const uint8_t* bytes, int nbytes,
                                  uint32_t pc) {
    char buf[64];
    (void)nbytes;
    switch (mode) {
        case AddressingMode::IMPLIED:
            return "";
        case AddressingMode::ACCUMULATOR:
            return "a";
        case AddressingMode::IMMEDIATE:
            snprintf(buf, sizeof(buf), "#$%02x", bytes[0]);
            return buf;
        case AddressingMode::IMMEDIATE16:
            snprintf(buf, sizeof(buf), "#$%04x", bytes[0] | (bytes[1] << 8));
            return buf;
        case AddressingMode::BASE_PAGE:
            snprintf(buf, sizeof(buf), "$%02x", bytes[0]);
            return buf;
        case AddressingMode::BASE_PAGE_X:
            snprintf(buf, sizeof(buf), "$%02x,x", bytes[0]);
            return buf;
        case AddressingMode::BASE_PAGE_Y:
            snprintf(buf, sizeof(buf), "$%02x,y", bytes[0]);
            return buf;
        case AddressingMode::ABSOLUTE:
            snprintf(buf, sizeof(buf), "$%04x", bytes[0] | (bytes[1] << 8));
            return buf;
        case AddressingMode::ABSOLUTE_X:
            snprintf(buf, sizeof(buf), "$%04x,x", bytes[0] | (bytes[1] << 8));
            return buf;
        case AddressingMode::ABSOLUTE_Y:
            snprintf(buf, sizeof(buf), "$%04x,y", bytes[0] | (bytes[1] << 8));
            return buf;
        case AddressingMode::ABSOLUTE_INDIRECT:
            snprintf(buf, sizeof(buf), "($%04x)", bytes[0] | (bytes[1] << 8));
            return buf;
        case AddressingMode::ABSOLUTE_X_INDIRECT:
            snprintf(buf, sizeof(buf), "($%04x,x)", bytes[0] | (bytes[1] << 8));
            return buf;
        case AddressingMode::BASE_PAGE_X_INDIRECT:
            snprintf(buf, sizeof(buf), "($%02x,x)", bytes[0]);
            return buf;
        case AddressingMode::BASE_PAGE_INDIRECT_Y:
            snprintf(buf, sizeof(buf), "($%02x),y", bytes[0]);
            return buf;
        case AddressingMode::BASE_PAGE_INDIRECT_Z:
            snprintf(buf, sizeof(buf), "($%02x),z", bytes[0]);
            return buf;
        case AddressingMode::FLAT_INDIRECT_Z:
            snprintf(buf, sizeof(buf), "[$%02x],z", bytes[0]);
            return buf;
        case AddressingMode::BASE_PAGE_INDIRECT_SP_Y:
            snprintf(buf, sizeof(buf), "($%02x,sp),y", bytes[0]);
            return buf;
        case AddressingMode::RELATIVE: {
            int8_t offset = (int8_t)bytes[0];
            uint32_t target = pc + 2 + offset;
            snprintf(buf, sizeof(buf), "$%04x", target & 0xFFFF);
            return buf;
        }
        case AddressingMode::RELATIVE16: {
            int16_t offset = (int16_t)(bytes[0] | (bytes[1] << 8));
            uint32_t target = pc + 2 + offset;
            snprintf(buf, sizeof(buf), "$%04x", target & 0xFFFF);
            return buf;
        }
        case AddressingMode::BASE_PAGE_RELATIVE: {
            int8_t rel = (int8_t)bytes[1];
            uint32_t target = pc + 3 + rel;
            snprintf(buf, sizeof(buf), "$%02x,$%04x", bytes[0], target & 0xFFFF);
            return buf;
        }
        case AddressingMode::STACK_RELATIVE:
            snprintf(buf, sizeof(buf), "$%02x,sp", bytes[0]);
            return buf;
        case AddressingMode::STACK_RELATIVE_INDIRECT_Y:
            snprintf(buf, sizeof(buf), "($%02x,sp),y", bytes[0]);
            return buf;
        case AddressingMode::LINEAR_ABSOLUTE:
            snprintf(buf, sizeof(buf), "$%06x",
                     bytes[0] | (bytes[1] << 8) | (bytes[2] << 16));
            return buf;
        case AddressingMode::LINEAR_ABSOLUTE_X:
            snprintf(buf, sizeof(buf), "$%06x,x",
                     bytes[0] | (bytes[1] << 8) | (bytes[2] << 16));
            return buf;
        case AddressingMode::LINEAR_ABSOLUTE_Y:
            snprintf(buf, sizeof(buf), "$%06x,y",
                     bytes[0] | (bytes[1] << 8) | (bytes[2] << 16));
            return buf;
        default:
            return "";
    }
}

// ─── Display functions ──────────────────────────────────────────────────────

static void printFileHeader(const O45File& obj, const std::string& filename) {
    bool is32 = (obj.mode & O45_MODE_SIZE32) != 0;
    std::cout << std::endl;
    std::cout << filename << ":     file format "
              << (is32 ? "o45-32" : "o65-16") << std::endl;
    printf("architecture: %s, ", cpuName(obj.cpuId));
    std::string flags;
    if (obj.mode & O45_MODE_SIZE32)  flags += ", SIZE32";
    if (obj.mode & O45_MODE_CPUEXT)  flags += ", CPUEXT";
    if (obj.mode & O45_MODE_BSSZERO) flags += ", BSSZERO";
    if (obj.mode & O45_MODE_PAGED)   flags += ", PAGED";
    if (obj.mode & O45_MODE_CHAIN)   flags += ", CHAIN";
    if (obj.mode & O45_MODE_SIMPLE)  flags += ", SIMPLE";
    printf("flags $%04x", obj.mode);
    if (!flags.empty()) std::cout << " (" << flags.substr(2) << ")";
    std::cout << std::endl;

    if (!obj.options.empty()) {
        for (const auto& opt : obj.options) {
            const char* name = "USER";
            switch (opt.type) {
                case OPT_FNAME:   name = "FNAME"; break;
                case OPT_OS:      name = "OS"; break;
                case OPT_ASM:     name = "ASM"; break;
                case OPT_AUTHOR:  name = "AUTHOR"; break;
                case OPT_CREATED: name = "CREATED"; break;
                case OPT_LINEINFO: name = "LINEINFO"; break;
                case OPT_DEBUG_SYMBOLS: name = "DEBUG_SYMBOLS"; break;
                case OPT_DEBUG_TYPES: name = "DEBUG_TYPES"; break;
                case OPT_DEBUG_VARS: name = "DEBUG_VARS"; break;
            }
            printf("  option: %s", name);
            bool isText = true;
            for (size_t i = 0; i < opt.data.size(); i++) {
                uint8_t c = opt.data[i];
                if (c == 0 && i == opt.data.size() - 1) continue;
                if (c < 0x20 || c > 0x7E) { isText = false; break; }
            }
            if (isText && !opt.data.empty()) {
                std::string s(opt.data.begin(), opt.data.end());
                if (!s.empty() && s.back() == '\0') s.pop_back();
                std::cout << " \"" << s << "\"";
            }
            std::cout << std::endl;
        }
    }
}

static void printSectionHeaders(const O45File& obj, const std::string& filename) {
    std::cout << std::endl;
    std::cout << filename << ":" << std::endl;
    std::cout << "Sections:" << std::endl;
    printf("Idx %-8s %-10s %-10s %-10s %-6s\n",
           "Name", "Size", "VMA", "Type", "Relocs");

    auto relocCount = [](const std::vector<uint8_t>& raw) -> size_t {
        return O45RelocDecoder::decode(raw).size();
    };

    int idx = 0;
    if (obj.tlen > 0) {
        printf("%3d %-8s %08X   %08X   %-10s %zu\n",
               idx++, "TEXT", obj.tlen, obj.tbase, "CODE",
               relocCount(obj.textRelocs));
    }
    if (obj.dlen > 0) {
        printf("%3d %-8s %08X   %08X   %-10s %zu\n",
               idx++, "DATA", obj.dlen, obj.dbase, "DATA",
               relocCount(obj.dataRelocs));
    }
    if (obj.blen > 0) {
        printf("%3d %-8s %08X   %08X   %-10s %d\n",
               idx++, "BSS", obj.blen, obj.bbase, "NOBITS", 0);
    }
    if (obj.zlen > 0) {
        printf("%3d %-8s %08X   %08X   %-10s %d\n",
               idx++, "ZP", obj.zlen, obj.zbase, "NOBITS", 0);
    }
    // Per-function/data sections (OPT_SECTIONS) ln45 keeps or drops as a whole
    if (!obj.sections.empty()) {
        static const char* segName[] = {"?", "?", "TEXT", "DATA", "BSS", "ZP"};
        std::cout << "Link sections (" << obj.sections.size() << "; the rest of each segment is kept with any of them):" << std::endl;
        printf("Idx %-5s %-10s %-10s %-5s %s\n", "Seg", "Offset", "Size", "Align", "Name");
        for (size_t i = 0; i < obj.sections.size(); i++) {
            const auto& sc = obj.sections[i];
            printf("%3zu %-5s %08X   %08X   %-5u %s%s\n", i, sc.segId <= 5 ? segName[sc.segId] : "?",
                   sc.offset, sc.length, 1u << sc.alignLog2, sc.name.c_str(), sc.keep ? " [keep]" : "");
        }
    }
}

static void printSymbolTable(const O45File& obj, const std::string& filename) {
    std::cout << std::endl;
    std::cout << "SYMBOL TABLE (" << filename << "):" << std::endl;

    for (const auto& imp : obj.imports) {
        printf("         U %s\n", imp.name.c_str());
    }
    for (const auto& exp : obj.exports) {
        char letter = exp.isWeak() ? 'W' : segmentLetter(exp.segmentId());
        printf("%08X %c %s", exp.offset, letter, exp.name.c_str());
        if (exp.hasFuncAttr) {
            printf("  %s", formatFuncAttr(exp.funcAttr).c_str());
        }
        printf("\n");
    }

    if (obj.imports.empty() && obj.exports.empty()) {
        std::cout << "no symbols" << std::endl;
    }
}

static void printRelocations(const O45File& obj, const std::string& filename) {
    auto printRelocs = [&](const std::vector<uint8_t>& raw, const char* segLabel) {
        auto relocs = O45RelocDecoder::decode(raw);
        if (relocs.empty()) return;
        std::cout << std::endl;
        printf("RELOCATION RECORDS FOR [%s] (%s):\n", segLabel, filename.c_str());
        printf("%-10s %-12s %-8s %s\n", "OFFSET", "TYPE", "SEGMENT", "SYMBOL");
        for (const auto& r : relocs) {
            printf("%08X   %-12s %-8s",
                   r.offset, relocTypeName((uint8_t)r.type),
                   segmentName((uint8_t)r.segment));
            if (r.segment == SEG_EXTERNAL) {
                if (r.symbolIndex < obj.imports.size()) {
                    std::cout << " " << obj.imports[r.symbolIndex].name;
                } else {
                    printf(" [index %u]", r.symbolIndex);
                }
            }
            if (o45RelocHasExtra(r.type)) {
                printf("  extra=$%02x", r.extra);
            }
            std::cout << std::endl;
        }
    };

    printRelocs(obj.textRelocs, "TEXT");
    printRelocs(obj.dataRelocs, "DATA");
}

static void hexDump(const std::vector<uint8_t>& body, uint32_t base,
                    const std::string& label) {
    if (body.empty()) return;
    std::cout << std::endl;
    std::cout << "Contents of section " << label << ":" << std::endl;
    for (size_t i = 0; i < body.size(); i += 16) {
        printf(" %04x ", (unsigned)(base + i));
        for (size_t j = 0; j < 16; j++) {
            if (i + j < body.size()) printf("%02x", body[i + j]);
            else printf("  ");
            if (j % 4 == 3) printf(" ");
        }
        printf(" ");
        for (size_t j = 0; j < 16 && i + j < body.size(); j++) {
            uint8_t c = body[i + j];
            printf("%c", (c >= 0x20 && c < 0x7F) ? c : '.');
        }
        printf("\n");
    }
}

static void printSectionContents(const O45File& obj) {
    hexDump(obj.textBody, obj.tbase, "TEXT");
    hexDump(obj.dataBody, obj.dbase, "DATA");
}

// ─── BASIC SYS stub detection ───────────────────────────────────────────────

// Detect a BASIC SYS stub at the start of a PRG body.
// Format: next-line-ptr(2) + line-number(2) + $9E(SYS) + ASCII digits + $00 + $00 $00
// Returns the stub size in bytes (0 if no stub detected), and fills sysAddr
// with the parsed SYS target address.
static size_t detectBasicStub(const std::vector<uint8_t>& body, uint32_t base,
                               uint32_t& sysAddr) {
    // Minimum stub: 2(next) + 2(line#) + 1($9E) + 1(digit) + 1(NUL) + 2(end) = 9
    if (body.size() < 9) return 0;
    // BASIC SYS token must be at offset 4
    if (body[4] != 0x9E) return 0;

    // Extract ASCII digits after the SYS token
    size_t i = 5;
    // Skip optional space after SYS token
    if (i < body.size() && body[i] == 0x20) i++;
    size_t digitStart = i;
    while (i < body.size() && body[i] >= '0' && body[i] <= '9') i++;
    if (i == digitStart) return 0;  // no digits found

    // Must be followed by $00 (end of BASIC line)
    if (i >= body.size() || body[i] != 0x00) return 0;
    i++;
    // Must be followed by $00 $00 (end of BASIC program)
    if (i + 1 >= body.size() || body[i] != 0x00 || body[i + 1] != 0x00) return 0;
    size_t stubSize = i + 2;

    // Verify next-line pointer is consistent (points to end-of-program marker)
    uint16_t nextLine = body[0] | (body[1] << 8);
    if (nextLine != base + stubSize - 2) return 0;

    // Parse the SYS address
    std::string digits(body.begin() + digitStart, body.begin() + digitStart + (i - 1 - digitStart));
    try {
        sysAddr = (uint32_t)std::stoul(digits);
    } catch (...) {
        return 0;
    }
    return stubSize;
}

// ─── Disassembly ────────────────────────────────────────────────────────────

// One section's labels: the exports defined in it (not those of the other
// sections or absolute ones, which would land at their numbers in it, #354)
static std::map<uint32_t, std::string> sectionSymbols(const O45File& obj, uint8_t seg) {
    std::map<uint32_t, std::string> syms;
    uint32_t base = seg == SEG_TEXT ? obj.tbase : obj.dbase;
    for (const auto& exp : obj.exports)
        if (exp.segmentId() == seg && !syms.count(base + exp.offset)) syms[base + exp.offset] = exp.name;
    return syms;
}

// What each relocated operand refers to, by the address of its first byte:
// the relocation's own symbol (an import, or the export of the target
// segment at or below the placeholder's address), not whatever symbol has
// the placeholder's value (#354)
static std::map<uint32_t, std::string> relocationNames(const O45File& obj, const std::vector<uint8_t>& raw,
                                                       const std::vector<uint8_t>& body, uint32_t base) {
    std::map<uint32_t, std::string> names;
    auto word = [&](uint32_t o) -> uint32_t { return o + 1 < body.size() ? body[o] | (body[o + 1] << 8) : 0; };
    auto plus = [](std::string n, int32_t k) {
        if (k == 0) return n;
        char b[16];
        snprintf(b, sizeof b, k > 0 ? "+%d" : "%d", k);
        return n + b;
    };
    for (const auto& r : O45RelocDecoder::decode(raw)) {
        if (r.offset >= body.size()) continue;
        // the value the placeholder holds (a full one where the type keeps it)
        uint32_t v;
        const char* part = "";   // (" lo" / " hi": a byte of it)
        switch (r.type) {
            case R_WORD: v = word(r.offset); break;
            case R_LOW: v = body[r.offset]; part = " lo"; break;
            case R_LOWX: v = body[r.offset] | (r.extra << 8); part = " lo"; break;
            case R_HIGH: v = (body[r.offset] << 8) | r.extra; part = " hi"; break;
            case R_LINEAR24: case R_LINEAR32: case R_SEGADR:
                v = word(r.offset) | (r.offset + 2 < body.size() ? body[r.offset + 2] << 16 : 0); break;
            default: continue;
        }
        std::string n;
        if (r.segment == SEG_EXTERNAL) {
            if (r.symbolIndex >= obj.imports.size()) continue;
            // (a byte keeps only part of the addend: named without it)
            n = r.type == R_LOW ? obj.imports[r.symbolIndex].name : plus(obj.imports[r.symbolIndex].name, (int16_t)v);
        } else {
            uint32_t sb = 0;
            const char* sn = "?";
            switch (r.segment) {
                case SEG_TEXT: sb = obj.tbase; sn = "TEXT"; break;
                case SEG_DATA: sb = obj.dbase; sn = "DATA"; break;
                case SEG_BSS:  sb = obj.bbase; sn = "BSS"; break;
                case SEG_ZP:   sb = obj.zbase; sn = "ZP"; break;
                default: continue;
            }
            if (r.type == R_LOW) {
                names[base + r.offset] = std::string("<") + sn + "> lo";   // (the low byte of an address in it)
                continue;
            }
            // (the export at or before it, plus the offset: a static has no
            // symbol, so one shows as the export before it)
            int64_t off = (int64_t)v - sb, best = -1;
            for (const auto& e : obj.exports)
                if (e.segmentId() == r.segment && (int64_t)e.offset <= off &&
                    ((int64_t)e.offset > best || ((int64_t)e.offset == best && e.name < n))) {
                    best = e.offset;
                    n = e.name;
                }
            if (best < 0) {
                char b[24];
                snprintf(b, sizeof b, "%s+$%x", sn, (unsigned)off);
                n = b;
            } else {
                n = plus(n, (int32_t)(off - best));
            }
        }
        names[base + r.offset] = "<" + n + ">" + part;
    }
    return names;
}

// Parse symbols from a linker map file (ln45 -M output).
// Looks for the "Symbols (by address)" section and parses lines like:
//   $200D  _main                          main_jmp.o45  [uses:- clob:- ...]
// Parse TEXT segment end address from map file Memory Layout section
static uint32_t loadMapTextEnd(const std::string& mapFile) {
    std::ifstream in(mapFile);
    if (!in.is_open()) return 0;
    std::string line;
    while (std::getline(in, line)) {
        // Look for: "  TEXT   $XXXX - $YYYY  (N bytes)"
        if (line.find("TEXT") != std::string::npos && line.find('$') != std::string::npos) {
            size_t dash = line.find('-');
            if (dash == std::string::npos) continue;
            size_t dollar = line.find('$', dash);
            if (dollar == std::string::npos) continue;
            try {
                uint32_t endAddr = (uint32_t)std::stoul(line.substr(dollar + 1), nullptr, 16);
                return endAddr + 1; // end is inclusive, return exclusive
            } catch (...) {}
        }
    }
    return 0;
}

static std::map<uint32_t, std::string> loadMapSymbols(const std::string& mapFile) {
    std::map<uint32_t, std::string> syms;
    std::ifstream in(mapFile);
    if (!in.is_open()) {
        std::cerr << "objdump45: warning: cannot open map file: " << mapFile << std::endl;
        return syms;
    }

    bool inSymbols = false;
    std::string line;
    while (std::getline(in, line)) {
        if (line.find("Symbols (by address)") != std::string::npos) {
            inSymbols = true;
            std::getline(in, line); // skip "----" underline
            continue;
        }
        if (inSymbols) {
            // Empty line or next section header ends the symbol block
            if (line.empty() || (!line.empty() && line[0] != ' ')) {
                break;
            }
            // Parse: "  $XXXX  name  ..."
            std::istringstream iss(line);
            std::string addrStr, name;
            iss >> addrStr >> name;
            if (addrStr.empty() || addrStr[0] != '$' || name.empty()) continue;
            try {
                uint32_t addr = (uint32_t)std::stoul(addrStr.substr(1), nullptr, 16);
                syms[addr] = name;
            } catch (...) {
                continue;
            }
        }
    }
    return syms;
}

// Source line info: address → (file, line)
struct SourceLineEntry {
    std::string file;
    int line;
};
using SourceLineMap = std::map<uint32_t, SourceLineEntry>;

// The resident functions (as symbols) and source lines of ln45 -g's debug
// file (<program>.dbg, #202): what the map gives, when there is no map
static bool loadDebugFile(const std::string& path, std::map<uint32_t, std::string>& symbols,
                          std::map<uint32_t, struct SourceLineEntry>& lines);

// Parse "Source Lines" section from a linker map file
static SourceLineMap loadMapSourceLines(const std::string& mapFile) {
    SourceLineMap lines;
    std::ifstream in(mapFile);
    if (!in.is_open()) return lines;

    bool inSection = false;
    std::string line;
    while (std::getline(in, line)) {
        if (line.find("Source Lines") != std::string::npos) {
            inSection = true;
            std::getline(in, line); // skip "----" underline
            continue;
        }
        if (inSection) {
            if (line.empty() || line[0] != ' ') break;
            // Parse: "  $XXXX  file.c:NN"
            std::istringstream iss(line);
            std::string addrStr, fileLine;
            iss >> addrStr >> fileLine;
            if (addrStr.empty() || addrStr[0] != '$' || fileLine.empty()) continue;
            auto colon = fileLine.rfind(':');
            if (colon == std::string::npos) continue;
            try {
                uint32_t addr = (uint32_t)std::stoul(addrStr.substr(1), nullptr, 16);
                std::string file = fileLine.substr(0, colon);
                int lineNum = std::stoi(fileLine.substr(colon + 1));
                lines[addr] = {file, lineNum};
            } catch (...) { continue; }
        }
    }
    return lines;
}

// -g: an object's debug information (#202): its source files and line
// table, the struct / union layouts and the variables with their locations
static void printDebugInfo(const O45File& obj, const std::string& filename) {
    std::cout << std::endl << filename << ":" << std::endl << "Debug information:" << std::endl;
    std::cout << "  Source files:" << std::endl;
    for (size_t i = 0; i < obj.lineFiles.size(); i++) printf("    %zu  %s\n", i, obj.lineFiles[i].c_str());
    std::cout << "  Lines (text offset  file:line):" << std::endl;
    for (const auto& li : obj.lineInfos)
        printf("    %04x  %s:%u\n", li.textOffset,
               li.fileIndex < obj.lineFiles.size() ? obj.lineFiles[li.fileIndex].c_str() : "?", li.line);
    std::cout << "  Types:" << std::endl;
    for (const auto& t : obj.debugTypes) {
        std::istringstream ws(t);
        std::string kind, name, size, m;
        ws >> kind >> name >> size;
        printf("    %s %s (%s bytes)\n", kind.c_str(), name.c_str(), size.c_str());
        while (ws >> m) {
            size_t at = m.find('@'), colon = m.find(':', at);
            if (at == std::string::npos || colon == std::string::npos) continue;
            printf("      %-16s +%-8s %s\n", m.substr(0, at).c_str(), m.substr(at + 1, colon - at - 1).c_str(),
                   m.substr(colon + 1).c_str());
        }
    }
    std::cout << "  Functions (label  location  length):" << std::endl;
    for (const auto& v : obj.debugVars) {
        std::istringstream ws(v);
        std::string kind, name, loc, len;
        ws >> kind >> name >> loc >> len;
        if (kind == "func") printf("    %-26s %-16s %s\n", name.c_str(), loc.c_str(), len.c_str());
    }
    std::cout << "  Variables (kind  scope  name  type  location):" << std::endl;
    for (const auto& v : obj.debugVars) {
        std::istringstream ws(v);
        std::string kind, scope, name, type, loc;
        ws >> kind >> scope >> name >> type >> loc;
        if (kind == "func") continue;
        printf("    %-7s %-18s %-14s %-16s %s\n", kind.c_str(), scope.c_str(), name.c_str(), type.c_str(), loc.c_str());
    }
}

static bool loadDebugFile(const std::string& path, std::map<uint32_t, std::string>& symbols,
                          std::map<uint32_t, SourceLineEntry>& lines) {
    std::ifstream in(path);
    if (!in) return false;
    std::stringstream ss;
    ss << in.rdbuf();
    std::string text = ss.str();
    Json root;
    if (!Json::parse(text, root) || root.kind != Json::OBJ) {
        std::cerr << "objdump45: " << path << ": not a debug file" << std::endl;
        return false;
    }
    std::vector<std::string> files;
    if (const Json* f = root.at("files"))
        for (const auto& v : f->arr) files.push_back(v.str);
    if (const Json* l = root.at("lines"))
        for (const auto& e : l->arr) {
            if (e.arr.size() < 3 || (e.arr.size() > 3 && e.arr[3].kind != Json::NUL)) continue;   // (an overlay's)
            uint32_t a = (uint32_t)e.arr[0].num;
            size_t fi = (size_t)e.arr[1].num;
            if (!lines.count(a)) lines[a] = {fi < files.size() ? files[fi] : "?", (int)e.arr[2].num};
        }
    if (const Json* fs = root.at("functions"))
        for (const auto& f : fs->arr) {
            const Json *n = f.at("name"), *st = f.at("start"), *ov = f.at("overlay");
            if (!n || !st || (ov && ov->kind != Json::NUL)) continue;
            if (!symbols.count((uint32_t)st->num)) symbols[(uint32_t)st->num] = n->str;
        }
    return true;
}

// Build source line map from .o45 line info
static SourceLineMap buildSourceLineMap(const O45File& obj) {
    SourceLineMap lines;
    for (const auto& li : obj.lineInfos) {
        std::string file;
        if (li.fileIndex < obj.lineFiles.size())
            file = obj.lineFiles[li.fileIndex];
        uint32_t addr = obj.tbase + li.textOffset;
        if (lines.find(addr) == lines.end())
            lines[addr] = {file, (int)li.line};
    }
    return lines;
}

// Cache for reading source file lines on demand
static std::map<std::string, std::vector<std::string>> sourceFileCache_;

static std::string getSourceLine(const std::string& file, int lineNum) {
    auto it = sourceFileCache_.find(file);
    if (it == sourceFileCache_.end()) {
        std::vector<std::string> fileLines;
        std::ifstream in(file);
        if (in.is_open()) {
            std::string l;
            while (std::getline(in, l)) fileLines.push_back(l);
        }
        it = sourceFileCache_.emplace(file, std::move(fileLines)).first;
    }
    if (lineNum >= 1 && lineNum <= (int)it->second.size())
        return it->second[lineNum - 1];
    return "";
}

// ─── Symbol files (-S, #144) ────────────────────────────────────────────────
// The names a symbol file gives addresses (ROM routines, I/O registers): an
// operand that is one of them is shown by its name (jsr CHROUT ; $ffd2), and
// a disassembled address that has one is labelled with it. The program's own
// symbols (an object's, a map's, a debug file's) come first.
static std::map<uint32_t, std::string> g_fileSyms;

// "$addr NAME [comment]" (also 0xaddr, or plain hex), VICE's "al C:addr .name",
// or "NAME = $addr"; # and ; start comment lines. false: cannot be read
static bool loadSymbolFile(const std::string& path, std::map<uint32_t, std::string>& out, std::string& error) {
    std::ifstream in(path);
    if (!in) { error = "cannot read " + path; return false; }
    auto number = [](std::string t, uint32_t& v) -> bool {
        int radix = 16;
        if (!t.empty() && t[0] == '$') t = t.substr(1);
        else if (t.size() > 2 && t[0] == '0' && (t[1] == 'x' || t[1] == 'X')) t = t.substr(2);
        else if (t.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos) return false;
        if (t.empty() || t.size() > 8 || t.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos) return false;
        v = (uint32_t)std::stoul(t, nullptr, radix);
        return true;
    };
    auto isName = [](const std::string& n) {
        if (n.empty() || !(std::isalpha((unsigned char)n[0]) || n[0] == '_' || n[0] == '.')) return false;
        for (char c : n) if (!(std::isalnum((unsigned char)c) || c == '_' || c == '.')) return false;
        return true;
    };
    std::string line;
    int lineNo = 0;
    while (std::getline(in, line)) {
        lineNo++;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        size_t f = line.find_first_not_of(" \t");
        if (f == std::string::npos || line[f] == '#' || line[f] == ';') continue;
        std::istringstream ls(line);
        std::string a, b, c;
        ls >> a >> b;
        uint32_t v;
        if (a == "al" && ls >> c) {                       // VICE: al C:ffd2 .chrout
            size_t colon = b.find(':');
            std::string addr = colon == std::string::npos ? b : b.substr(colon + 1);
            if (!c.empty() && c[0] == '.') c = c.substr(1);
            if (number(addr, v) && isName(c)) { out[v] = c; continue; }
        } else if (b == "=" || (b.size() > 1 && b[0] == '=') || (a.find('=') != std::string::npos)) {
            std::string rest = line.substr(f);           // NAME = $addr
            size_t eq = rest.find('=');
            std::string n = rest.substr(0, eq), val = rest.substr(eq + 1);
            n.erase(n.find_last_not_of(" \t") + 1);
            std::istringstream vs(val);
            std::string vt;
            vs >> vt;
            if (isName(n) && number(vt, v)) { out[v] = n; continue; }
        } else if (number(a, v) && isName(b)) {          // $addr NAME [comment]
            out[v] = b;
            continue;
        }
        error = path + ":" + std::to_string(lineNo) + ": not a symbol line (\"$addr NAME\", \"al C:addr .name\" or \"NAME = $addr\")";
        return false;
    }
    return true;
}

// -S NAME without a file of that name: lib/symbols/NAME.sym beside the tools
// (the build tree, or an installed one's lib/cc45/symbols)
static std::string shippedSymbolFile(const std::string& name) {
    if (name.find('/') != std::string::npos) return "";
    char buf[4096];
    ssize_t n = readlink("/proc/self/exe", buf, sizeof buf - 1);
    if (n <= 0) return "";
    buf[n] = 0;
    std::string bin = buf;
    bin = bin.substr(0, bin.find_last_of('/') + 1);
    for (const char* d : {"../lib/symbols/", "../lib/cc45/symbols/"}) {
        std::string p = bin + d + name + ".sym";
        if (std::ifstream(p)) return p;
    }
    return "";
}

static void disassembleSection(const std::vector<uint8_t>& body, uint32_t base,
                               const char* sectionName,
                               const std::map<uint8_t, OpcodeInfo>& opcodeTable,
                               const std::map<uint32_t, std::string>& symbols,
                               const std::string& filename,
                               const SourceLineMap& sourceLines = {},
                               const std::map<uint32_t, std::string>* relocNames = nullptr,
                               DisCpu cpu = DisCpu::CPU45GS02, const char* note = nullptr) {
    if (body.empty()) return;
    std::cout << std::endl;
    printf("Disassembly of section %s (%s):\n", sectionName, filename.c_str());
    if (note) printf("; %s\n", note);
    LOG_INFO("objdump45", sectionName << ": decoded for the " << disCpuName(cpu));

    int lastSourceLine = -1;
    std::string lastSourceFile;

    size_t i = 0;
    while (i < body.size()) {
        uint32_t pc = base + (uint32_t)i;

        // Print label if a symbol exists at this address (else a symbol file's name)
        auto symIt = symbols.find(pc);
        if (symIt != symbols.end()) {
            std::cout << std::endl;
            printf("%08x <%s>:\n", pc, symIt->second.c_str());
        } else if (g_fileSyms.count(pc)) {
            std::cout << std::endl;
            printf("%08x <%s>:\n", pc, g_fileSyms.at(pc).c_str());
        }

        // Print source line comment when location changes (range-based lookup)
        if (!sourceLines.empty()) {
            auto slIt = sourceLines.upper_bound(pc);
            if (slIt != sourceLines.begin()) {
                --slIt; // most recent entry at or before pc
                if (slIt->second.line != lastSourceLine || slIt->second.file != lastSourceFile) {
                    lastSourceFile = slIt->second.file;
                    lastSourceLine = slIt->second.line;
                    std::string srcText = getSourceLine(lastSourceFile, lastSourceLine);
                    size_t start = srcText.find_first_not_of(" \t");
                    if (start != std::string::npos) srcText = srcText.substr(start);
                    printf("; %s:%d  %s\n", lastSourceFile.c_str(), lastSourceLine, srcText.c_str());
                }
            }
        }

        // the 45GS02's 32-bit instructions: NEG NEG ($42 $42) before one
        // that has a Q form (ldq, stq, adcq, cpq, aslq, ...)
        size_t pre = 0;
        std::string qname;
        if (cpu == DisCpu::CPU45GS02 && i + 2 < body.size() && body[i] == 0x42 && body[i + 1] == 0x42) {
            auto qit = opcodeTable.find(body[i + 2]);
            if (qit != opcodeTable.end()) {
                std::string m = qit->second.mnemonic;
                std::transform(m.begin(), m.end(), m.begin(), ::tolower);
                static const std::map<std::string, std::string> qform = {
                    {"lda", "ldq"}, {"sta", "stq"}, {"cmp", "cpq"}, {"ora", "orq"}, {"inc", "inq"}, {"dec", "deq"},
                    {"adc", "adcq"}, {"sbc", "sbcq"}, {"and", "andq"}, {"eor", "eorq"}, {"bit", "bitq"},
                    {"asl", "aslq"}, {"lsr", "lsrq"}, {"rol", "rolq"}, {"ror", "rorq"}, {"asr", "asrq"}};
                auto q = qform.find(m);
                if (q != qform.end()) {
                    pre = 2;
                    qname = q->second;
                }
            }
        }
        uint8_t opcode = body[i + pre];
        auto it = opcodeTable.find(opcode);

        if (it == opcodeTable.end()) {
            // Unknown opcode — emit as .byte
            printf("    %04x:\t%02x          \t.byte\t$%02x\n", pc, opcode, opcode);
            i++;
            continue;
        }

        const OpcodeInfo& info = it->second;
        int opSize = operandSize(info.mode);
        int instrSize = (int)pre + 1 + opSize;

        // Check if we have enough bytes
        if (i + instrSize > body.size()) {
            // Not enough bytes — dump remainder as .byte
            for (size_t j = i; j < body.size(); j++) {
                printf("    %04x:\t%02x          \t.byte\t$%02x\n",
                       (unsigned)(base + j), body[j], body[j]);
            }
            break;
        }

        // Format hex bytes column (the instruction's bytes, padded)
        char hexCol[24];
        {
            std::string h;
            for (int b = 0; b < instrSize; b++) {
                char x[4];
                snprintf(x, sizeof(x), b ? " %02x" : "%02x", body[i + b]);
                h += x;
            }
            if (instrSize < 4) while (h.size() < 12) h += ' ';   // (the widths before)
            snprintf(hexCol, sizeof(hexCol), "%s", h.c_str());
        }

        size_t o = i + pre;     // the instruction after any prefix
        const uint8_t* operandBytes = (opSize > 0) ? &body[o + 1] : nullptr;
        std::string operand = formatOperand(info.mode, operandBytes, opSize, pc);

        // Annotate operands with symbol names where the address matches (in
        // an object: a relocated operand by its relocation, any other by none)
        std::string annotation;
        if (relocNames && opSize > 0 && info.mode != AddressingMode::RELATIVE &&
            info.mode != AddressingMode::RELATIVE16 && info.mode != AddressingMode::BASE_PAGE_RELATIVE) {
            for (int b = 0; b < opSize && annotation.empty(); b++) {
                auto rn = relocNames->find(pc + (uint32_t)pre + 1 + b);
                if (rn != relocNames->end()) annotation = "\t; " + rn->second;
            }
        } else if (info.mode == AddressingMode::RELATIVE ||
            info.mode == AddressingMode::RELATIVE16) {
            uint32_t target;
            if (info.mode == AddressingMode::RELATIVE) {
                int8_t offset = (int8_t)body[o + 1];
                target = pc + (uint32_t)pre + 2 + offset;
            } else {
                int16_t offset = (int16_t)(body[o + 1] | (body[o + 2] << 8));
                target = pc + (uint32_t)pre + 2 + offset;     // as formatOperand: from pc + 2
            }
            target &= 0xFFFF;
            auto tgtSym = symbols.find(target);
            if (tgtSym != symbols.end()) {
                annotation = "\t; <" + tgtSym->second + ">";
            }
        } else if (info.mode == AddressingMode::ABSOLUTE ||
                   info.mode == AddressingMode::ABSOLUTE_X ||
                   info.mode == AddressingMode::ABSOLUTE_Y ||
                   info.mode == AddressingMode::ABSOLUTE_INDIRECT ||
                   info.mode == AddressingMode::ABSOLUTE_X_INDIRECT) {
            uint32_t target = body[o + 1] | (body[o + 2] << 8);
            auto tgtSym = symbols.find(target);
            if (tgtSym != symbols.end()) {
                annotation = "\t; <" + tgtSym->second + ">";
            }
        } else if (info.mode == AddressingMode::IMMEDIATE16) {
            uint32_t val = body[o + 1] | (body[o + 2] << 8);
            auto tgtSym = symbols.find(val);
            if (tgtSym != symbols.end()) {
                annotation = "\t; <" + tgtSym->second + ">";
            }
        } else if (info.mode == AddressingMode::BASE_PAGE_RELATIVE) {
            int8_t rel = (int8_t)body[o + 2];
            uint32_t target = (pc + 3 + rel) & 0xFFFF;
            auto tgtSym = symbols.find(target);
            if (tgtSym != symbols.end()) {
                annotation = "\t; <" + tgtSym->second + ">";
            }
        }

        // a symbol file's name for the address the operand names (not an
        // immediate), when the program has none there: jsr CHROUT ; $ffd2
        if (!g_fileSyms.empty() && annotation.empty() && opSize > 0 && info.mode != AddressingMode::IMMEDIATE &&
            info.mode != AddressingMode::IMMEDIATE16 && info.mode != AddressingMode::BASE_PAGE_RELATIVE) {
            uint32_t addr;
            char hex[8];
            if (info.mode == AddressingMode::RELATIVE || info.mode == AddressingMode::RELATIVE16) {
                int off = info.mode == AddressingMode::RELATIVE ? (int8_t)body[o + 1]
                                                                : (int16_t)(body[o + 1] | (body[o + 2] << 8));
                addr = (pc + (uint32_t)pre + 2 + off) & 0xFFFF;
                snprintf(hex, sizeof hex, "$%04x", addr);
            } else if (opSize == 1) {
                addr = body[o + 1];
                snprintf(hex, sizeof hex, "$%02x", addr);
            } else {
                addr = body[o + 1] | (body[o + 2] << 8);
                snprintf(hex, sizeof hex, "$%04x", addr);
            }
            auto fs = g_fileSyms.find(addr);
            size_t at = operand.find(hex);
            if (fs != g_fileSyms.end() && !symbols.count(addr) && at != std::string::npos) {
                operand = operand.substr(0, at) + fs->second + operand.substr(at + strlen(hex));
                annotation = std::string("\t; ") + hex;
            }
        }
        if (info.undocumented)
            annotation += annotation.empty() ? "\t; undocumented" : " (undocumented)";
        printf("    %04x:\t%s\t%s\t%s%s\n",
               pc, hexCol, pre ? qname.c_str() : info.mnemonic.c_str(), operand.c_str(),
               annotation.c_str());

        i += instrSize;
    }
}

// ─── Main ───────────────────────────────────────────────────────────────────

// ─── Raw binary / PRG support ───────────────────────────────────────────────

static bool hasExtension(const std::string& filename, const std::string& ext) {
    if (filename.size() < ext.size()) return false;
    std::string tail = filename.substr(filename.size() - ext.size());
    for (auto& c : tail) c = std::tolower(c);
    return tail == ext;
}

// ─── BASIC Program Dumping ──────────────────────────────────────────────────

// Convert raw BASIC program bytes to BasicToken objects
static std::vector<BasicToken> bytesToTokens(const std::vector<uint8_t>& bytes, const BasicTokenizer& tokenizer) {
    std::vector<BasicToken> tokens;
    const auto& keywords = tokenizer.getKeywords();
    const auto& escapeKeywords = tokenizer.getEscapeKeywords();

    for (size_t i = 0; i < bytes.size(); i++) {
        uint8_t byte = bytes[i];
        BasicToken token;

        if (byte == 0) break;  // End of line

        if (byte >= 0x80) {
            // Keyword token
            if ((byte == 0xFE || byte == 0xCE) && i + 1 < bytes.size()) {
                // Escape keyword (two-byte sequence: $FE or BASIC 65's $CE functions)
                uint8_t escapeByte = bytes[i + 1];
                token.type = BasicToken::KEYWORD;
                token.tokenByte = byte;
                token.escapeByte = escapeByte;
                // Find keyword name
                for (const auto& [name, code] : byte == 0xFE ? escapeKeywords : tokenizer.getCeKeywords()) {
                    if (code == escapeByte) {
                        token.value = name;
                        break;
                    }
                }
                if (byte == 0xFE && (escapeByte == 0x52 || escapeByte == 0x53)) token.value = escapeByte == 0x52 ? "<<" : ">>";
                i++;  // Skip next byte since we consumed it
            } else {
                // Single-byte keyword
                token.type = BasicToken::KEYWORD;
                token.tokenByte = byte;
                // Find keyword name
                for (const auto& [name, code] : keywords) {
                    if (code == byte) {
                        token.value = name;
                        break;
                    }
                }
            }
        } else if (byte == '"') {
            // String token - read until closing quote
            token.type = BasicToken::STRING;
            token.value = "\"";
            i++;
            while (i < bytes.size() && bytes[i] != '"') {
                token.value += (char)bytes[i];
                i++;
            }
            if (i < bytes.size()) token.value += '"';
        } else if (std::isdigit(byte)) {
            // Number token
            token.type = BasicToken::NUMBER;
            token.value = (char)byte;
            while (i + 1 < bytes.size() && std::isdigit(bytes[i + 1])) {
                i++;
                token.value += (char)bytes[i];
            }
        } else if (std::isalpha(byte) || byte == '_') {
            // Identifier token
            token.type = BasicToken::IDENTIFIER;
            token.value = (char)byte;
            while (i + 1 < bytes.size() && (std::isalnum(bytes[i + 1]) || bytes[i + 1] == '_')) {
                i++;
                token.value += (char)bytes[i];
            }
        } else {
            // Operator or other character
            token.type = BasicToken::OPERATOR;
            token.value = (char)byte;
        }

        tokens.push_back(token);
    }

    return tokens;
}

static bool readBasicPrg(const std::string& filename, std::vector<BasicLine>& lines) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) return false;

    uint8_t loadLow, loadHigh;
    file.read((char*)&loadLow, 1);
    file.read((char*)&loadHigh, 1);
    if (!file) return false;

    uint16_t loadAddr = loadLow | (loadHigh << 8);
    uint16_t currentAddr = loadAddr;

    BasicTokenizer tokenizer;

    while (file) {
        uint8_t nextLow, nextHigh;
        file.read((char*)&nextLow, 1);
        file.read((char*)&nextHigh, 1);
        if (!file || (nextLow == 0 && nextHigh == 0)) break;

        uint16_t nextAddr = nextLow | (nextHigh << 8);

        uint8_t lineLow, lineHigh;
        file.read((char*)&lineLow, 1);
        file.read((char*)&lineHigh, 1);
        if (!file) break;

        uint16_t lineNum = lineLow | (lineHigh << 8);

        std::vector<uint8_t> rawBytes;
        uint8_t byte;
        while (file.read((char*)&byte, 1)) {
            if (byte == 0) break;
            rawBytes.push_back(byte);
        }

        BasicLine line;
        line.lineNumber = lineNum;
        line.tokens = bytesToTokens(rawBytes, tokenizer);
        lines.push_back(line);

        currentAddr = nextAddr;
    }

    return true;
}

static void dumpBasicProgram(const std::string& filename, bool showHex) {
    std::vector<BasicLine> lines;
    if (!readBasicPrg(filename, lines)) {
        std::cerr << "objdump45: Cannot read BASIC program: " << filename << std::endl;
        return;
    }

    printf("\nBASIC Program: %s (%zu lines)\n", filename.c_str(), lines.size());
    printf("─────────────────────────────────\n");
    printf("Format: LineNum [TOKENS...]\n\n");

    for (const auto& line : lines) {
        if (showHex) {
            printf("%5u [HEX] ", line.lineNumber);
            for (const auto& token : line.tokens) {
                if ((token.tokenByte == 0xFE || token.tokenByte == 0xCE) && token.escapeByte != 0) {
                    printf("%02X%02X ", token.tokenByte, token.escapeByte);
                } else if (token.tokenByte != 0) {
                    printf("%02X ", token.tokenByte);
                } else {
                    // Non-keyword token - just show first char if available
                    if (!token.value.empty()) {
                        printf("%02X ", (uint8_t)token.value[0]);
                    }
                }
            }
            printf("\n");
        }

        printf("%5u ", line.lineNumber);
        printf(" ");

        for (const auto& token : line.tokens) {
            // Print token value or raw representation
            if (!token.value.empty()) {
                printf("%s ", token.value.c_str());
            } else if ((token.tokenByte == 0xFE || token.tokenByte == 0xCE) && token.escapeByte != 0) {
                printf("[%02X%02X] ", token.tokenByte, token.escapeByte);
            } else if (token.tokenByte != 0) {
                printf("[%02X] ", token.tokenByte);
            }
        }

        printf("\n");
    }
}

static bool diffBinaries(const std::string& file1, const std::string& file2) {
    for (const auto& f : {file1, file2}) {
        if (isDirectory(f)) {
            std::cerr << "objdump45: " << f << ": is a directory" << std::endl;
            return false;
        }
    }
    std::ifstream f1(file1, std::ios::binary), f2(file2, std::ios::binary);
    if (!f1 || !f2) {
        std::cerr << "objdump45: cannot open files for comparison" << std::endl;
        return false;
    }

    std::vector<uint8_t> data1((std::istreambuf_iterator<char>(f1)), std::istreambuf_iterator<char>()),
                         data2((std::istreambuf_iterator<char>(f2)), std::istreambuf_iterator<char>());
    f1.close(); f2.close();

    printf("Binary Diff: %s vs %s\n", file1.c_str(), file2.c_str());
    printf("Size: %zu bytes vs %zu bytes\n\n", data1.size(), data2.size());

    if (data1.empty() && data2.empty()) {
        printf("Both files are empty (identical)\n");
        return true;
    }

    int diffCount = 0;
    uint32_t lastDiffAddr = -1;
    for (size_t i = 0; i < std::min(data1.size(), data2.size()); i++) {
        if (data1[i] != data2[i]) {
            if (diffCount == 0 || i > lastDiffAddr + 16) {
                if (diffCount > 0) printf("\n");
                printf("Offset $%04zX: ", i);
            }
            printf("[%02X→%02X] ", data1[i], data2[i]);
            lastDiffAddr = i;
            diffCount++;
            if (diffCount > 0 && (diffCount % 8 == 0)) printf("\n            ");
        }
    }

    if (data1.size() != data2.size()) {
        printf("\nSize difference: %s is %zu bytes, %s is %zu bytes\n",
               file1.c_str(), data1.size(), file2.c_str(), data2.size());
    }

    if (diffCount == 0) {
        printf("Files are identical\n");
    } else {
        printf("\n\nTotal differences: %d bytes\n", diffCount);
    }
    return true;
}

static void printUsage(const char* progName) {
    std::cout << "Usage: " << progName << " [options] <file> [...]" << std::endl;
    std::cout << "Display information from .o45/.o65 object files, .prg, or raw binaries." << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -f       Display file header" << std::endl;
    std::cout << "  -h       Display section headers" << std::endl;
    std::cout << "  -t       Display symbol table" << std::endl;
    std::cout << "  -r       Display relocation entries" << std::endl;
    std::cout << "  -s       Display full contents of all sections (hex dump)" << std::endl;
    std::cout << "  -d       Disassemble executable sections" << std::endl;
    std::cout << "  -g       Display debug information (source lines, types, variables)" << std::endl;
    std::cout << "  -a       Display all information" << std::endl;
    std::cout << "  -D       Binary diff mode (requires two files)" << std::endl;
    std::cout << "  -bas     Dump BASIC program (for .prg files only)" << std::endl;
    std::cout << "  -x       Show hex dump (with -bas option)" << std::endl;
    std::cout << "  -b ADDR  Set base address for raw binary files (default: $0000)" << std::endl;
    std::cout << "  -m FILE  Load symbols from linker map file (ln45 -M output)" << std::endl;
    std::cout << "  -S FILE  Name ROM routines and I/O registers from a symbol file ($addr NAME lines, VICE's" << std::endl;
    std::cout << "           al C:addr .name, or NAME = $addr; again for more): jsr CHROUT ; $ffd2. The shipped" << std::endl;
    std::cout << "           sets: -S c64, -S mega65 (default: the CPU's, a $0801 PRG's c64); -S none: none" << std::endl;
    std::cout << "  --cpu=CPU  Decode for 45gs02 (the default), 65ce02, 6502 (documented opcodes; others as" << std::endl;
    std::cout << "           .byte) or 6510 (with the undocumented ones, marked); a .prg loading at $0801 is 6510" << std::endl;
    std::cout << "  -V       Display version" << std::endl;
    std::cout << "  -?       Display this help message" << std::endl;
    std::cout << "  -v ... -vvvv  Log to stderr ([objdump45:INFO] ...): each file's format, the map or" << std::endl;
    std::cout << "           debug file used for symbols; -v0 off" << std::endl;
    std::cout << std::endl;
    std::cout << "Supported formats:" << std::endl;
    std::cout << "  .o45/.o65  Relocatable object files (full header/symbol/reloc support)" << std::endl;
    std::cout << "  .prg       PRG files (2-byte load address header, auto-detected, -bas for BASIC dump)" << std::endl;
    std::cout << "  .bin       Raw binary files (use -b to set base address)" << std::endl;
    std::cout << "  .ovd .ov1  Code overlay files (ln45; also a version 1 .ovl): a section per overlay, at its run address" << std::endl;
}

int main(int argc, char** argv) {
    bool showFileHeader = false;
    bool showSections = false;
    bool showSymbols = false;
    bool showRelocs = false;
    bool showContents = false;
    bool showDisasm = false;
    bool showAll = false;
    bool showDebug = false;
    bool showBasic = false;
    bool showHex = false;
    bool showDiff = false;
    uint32_t baseAddr = 0;
    bool baseAddrSet = false;
    std::string mapFile;
    DisCpu cpu = DisCpu::CPU45GS02;
    bool cpuSet = false;
    std::vector<std::string> symFiles;   // -S (none: no symbol file)
    bool symNone = false;
    std::vector<std::string> files;
    int verbosity = 0;
    Logger::setTool("objdump45");

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-V" || arg == "--version") {
            std::cout << suiteVersionString("objdump45") << std::endl;
            return 0;
        } else if (arg == "-?" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-b") {
            if (i + 1 >= argc) {
                std::cerr << "objdump45: -b requires an address argument" << std::endl;
                return 1;
            }
            std::string addrStr = argv[++i];
            // Accept $XXXX or 0xXXXX or plain decimal
            if (!addrStr.empty() && addrStr[0] == '$') {
                baseAddr = (uint32_t)std::stoul(addrStr.substr(1), nullptr, 16);
            } else {
                baseAddr = (uint32_t)std::stoul(addrStr, nullptr, 0);
            }
            baseAddrSet = true;
        } else if (arg == "-m") {
            if (i + 1 >= argc) {
                std::cerr << "objdump45: -m requires a map file argument" << std::endl;
                return 1;
            }
            mapFile = argv[++i];
        } else if (arg == "-S") {
            if (i + 1 >= argc) {
                std::cerr << "objdump45: -S requires a symbol file (a file, c64, mega65 or none)" << std::endl;
                return 1;
            }
            std::string f = argv[++i];
            if (f == "none") symNone = true;
            else symFiles.push_back(f);
        } else if (arg == "--cpu" || arg.rfind("--cpu=", 0) == 0) {
            std::string name;
            if (arg == "--cpu") {
                if (i + 1 >= argc) {
                    std::cerr << "objdump45: --cpu requires a CPU (45gs02, 65ce02, 6502, 6510)" << std::endl;
                    return 1;
                }
                name = argv[++i];
            } else {
                name = arg.substr(6);
            }
            if (!parseDisCpu(name, cpu)) {
                std::cerr << "objdump45: unknown CPU '" << name << "' (45gs02, 65ce02, 6502, 6510)" << std::endl;
                return 1;
            }
            cpuSet = true;
        } else if (arg == "-bas") {
            showBasic = true;
        } else if (arg == "-D") {
            showDiff = true;
        } else if (arg == "-x") {
            showHex = true;
        } else if (Logger::parseFlag(arg, verbosity)) {
            Logger::setLevel(verbosity);
        } else if (arg[0] == '-' && arg.size() > 1) {
            // Parse combined flags like -fdh
            for (size_t j = 1; j < arg.size(); j++) {
                switch (arg[j]) {
                    case 'f': showFileHeader = true; break;
                    case 'h': showSections = true; break;
                    case 't': showSymbols = true; break;
                    case 'r': showRelocs = true; break;
                    case 's': showContents = true; break;
                    case 'd': showDisasm = true; break;
                    case 'g': showDebug = true; break;
                    case 'a': showAll = true; break;
                    case 'x': showHex = true; break;
                    default:
                        std::cerr << "objdump45: unknown option '-" << arg[j] << "'" << std::endl;
                        return 1;
                }
            }
        } else {
            files.push_back(arg);
        }
    }

    if (showAll) {
        showFileHeader = showSections = showSymbols = showRelocs = showContents = showDisasm = showDebug = true;
    }

    // Handle diff mode - requires exactly 2 files
    if (showDiff) {
        if (files.size() != 2) {
            std::cerr << "objdump45: -D requires exactly 2 files" << std::endl;
            return 1;
        }
        return diffBinaries(files[0], files[1]) ? 0 : 1;
    }

    // If no display options given, default to showing file header (unless -bas is specified)
    if (!showFileHeader && !showSections && !showSymbols && !showRelocs &&
        !showContents && !showDisasm && !showBasic && !showDebug) {
        std::cerr << "objdump45: no display option specified (use -f, -h, -t, -r, -s, -d, -a, or -bas)" << std::endl;
        printUsage(argv[0]);
        return 1;
    }

    if (files.empty()) {
        std::cerr << "objdump45: no input files" << std::endl;
        printUsage(argv[0]);
        return 1;
    }

    // The opcode table of the CPU (--cpu); a C64 PRG's own when no --cpu is given
    auto opcodeTable = buildOpcodeTable(cpu);
    std::map<uint8_t, OpcodeInfo> table6510;

    // Symbol files (-S, #144): the ones named (a file, or a shipped set: c64,
    // mega65), else the CPU's shipped set (6502/6510: c64, else mega65; a
    // $0801 PRG's: c64); -S none: none
    auto loadSyms = [&](const std::string& name, std::map<uint32_t, std::string>& into, bool quiet) -> bool {
        std::string path = std::ifstream(name) ? name : shippedSymbolFile(name);
        if (path.empty()) {
            if (!quiet) std::cerr << "objdump45: -S " << name << ": no such file, and no shipped set of that name (c64, mega65)" << std::endl;
            return false;
        }
        std::string err;
        if (!loadSymbolFile(path, into, err)) { std::cerr << "objdump45: " << err << std::endl; return false; }
        LOG_INFO("objdump45", "symbols from " << path << " (" << into.size() << ")");
        return true;
    };
    std::map<uint32_t, std::string> c64Syms;
    bool c64Loaded = false;
    if (!symFiles.empty()) {
        for (const auto& f : symFiles)
            if (!loadSyms(f, g_fileSyms, false)) return 1;
    } else if (!symNone) {
        loadSyms(cpu == DisCpu::CPU6502 || cpu == DisCpu::CPU6510 ? "c64" : "mega65", g_fileSyms, true);
    }

    int exitCode = 0;

    for (const auto& filename : files) {
        if (isDirectory(filename)) {
            std::cerr << "objdump45: " << filename << ": is a directory" << std::endl;
            exitCode = 1;
            continue;
        }
        std::ifstream in(filename, std::ios::binary);
        if (!in.is_open()) {
            std::cerr << "objdump45: " << filename << ": cannot open" << std::endl;
            exitCode = 1;
            continue;
        }

        std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)),
                                   std::istreambuf_iterator<char>());
        in.close();

        // A code overlay file (ln45 <program>.ovd / .ov1, or a version 1 .ovl): a section per overlay,
        // at the address it runs at
        if (O45OverlayFile::is(filename, data)) {
            O45OverlayFile ov;
            std::string oerr;
            if (!O45OverlayFile::open(filename, data, ov, oerr)) {
                std::cerr << "objdump45: " << filename << ": " << oerr << std::endl;
                exitCode = 1;
                continue;
            }
            LOG_INFO("objdump45", filename << ": code overlay file (" << (ov.mapped ? "mapped" : "copied") << "), "
                                  << ov.overlays.size() << " overlay(s)");
            if (showFileHeader) {
                std::cout << std::endl;
                std::cout << filename << ":     file format code overlays (" << (ov.mapped ? "mapped" : "copied") << ")" << std::endl;
                printf("run address: $%04x, loaded at: $%05x, %zu overlays, %zu symbols\n",
                       ov.region, ov.loadAddress, ov.overlays.size(), ov.symbols.size());
            }
            if (showSections) {
                std::cout << std::endl << filename << ":" << std::endl << "Sections:" << std::endl;
                printf("Idx Name             Size       VMA        LMA\n");
                for (size_t i = 0; i < ov.overlays.size(); i++)
                    printf("%3zu %-16s %08x   %08x   %08x\n", i + 1, ov.overlays[i].name.c_str(),
                           ov.overlays[i].length, ov.region, ov.overlays[i].at);
            }
            if (showSymbols) {
                std::cout << std::endl << "SYMBOL TABLE:" << std::endl;
                for (const auto& s : ov.symbols)
                    printf("%08x g     %-16s %s\n", s.address,
                           s.overlay >= 1 && s.overlay <= (int)ov.overlays.size() ? ov.overlays[s.overlay - 1].name.c_str() : "?",
                           s.name.c_str());
            }
            for (size_t i = 0; i < ov.overlays.size(); i++) {
                const auto& o = ov.overlays[i];
                std::vector<uint8_t> body = ov.code(i);
                std::string label = "overlay " + o.name;
                if (body.empty() && o.length && (showContents || showDisasm)) {
                    std::cerr << "objdump45: " << filename << ": overlay " << o.name << ": its code file (.ov"
                              << o.file << ") was not found" << std::endl;
                    exitCode = 1;
                    continue;
                }
                if (showContents) hexDump(body, ov.region, label);
                if (showDisasm) {
                    std::map<uint32_t, std::string> syms;
                    for (const auto& s : ov.symbols)
                        if (s.overlay == (int)i + 1 && !syms.count(s.address)) syms[s.address] = s.name;
                    disassembleSection(body, ov.region, label.c_str(), opcodeTable, syms, filename, {}, nullptr, cpu);
                }
            }
            continue;
        }

        // Try .o45/.o65 first
        O45File obj;
        std::string err;
        bool isObject = O45Reader::read(data, obj, err);

        if (isObject) {
            // ── .o45 / .o65 object file ──
            LOG_INFO("objdump45", filename << ": object, text " << obj.tlen << " bytes at $" << std::hex << obj.tbase
                                  << ", data " << std::dec << obj.dlen << " bytes");
            if (showFileHeader) printFileHeader(obj, filename);
            if (showSections)   printSectionHeaders(obj, filename);
            if (showSymbols)    printSymbolTable(obj, filename);
            if (showRelocs)     printRelocations(obj, filename);
            if (showContents)   printSectionContents(obj);
            if (showDebug)      printDebugInfo(obj, filename);
            if (showDisasm) {
                auto srcLines = buildSourceLineMap(obj);
                auto textNames = relocationNames(obj, obj.textRelocs, obj.textBody, obj.tbase);
                auto dataNames = relocationNames(obj, obj.dataRelocs, obj.dataBody, obj.dbase);
                disassembleSection(obj.textBody, obj.tbase, "TEXT", opcodeTable,
                                 sectionSymbols(obj, SEG_TEXT), filename, srcLines, &textNames, cpu);
                disassembleSection(obj.dataBody, obj.dbase, "DATA", opcodeTable,
                                 sectionSymbols(obj, SEG_DATA), filename, {}, &dataNames, cpu);
            }
        } else {
            // ── Raw binary or PRG ──
            bool isPrg = hasExtension(filename, ".prg");
            LOG_DEBUG("objdump45", filename << ": not an object (" << err << ")");

            // Check for BASIC dump mode
            if (showBasic && isPrg) {
                dumpBasicProgram(filename, showHex);
                continue;
            }

            uint32_t base = baseAddr;
            std::vector<uint8_t> body;

            if (isPrg) {
                if (data.size() < 2) {
                    std::cerr << "objdump45: " << filename << ": PRG file too small" << std::endl;
                    exitCode = 1;
                    continue;
                }
                uint16_t loadAddr = data[0] | (data[1] << 8);
                base = baseAddrSet ? baseAddr : loadAddr;
                body.assign(data.begin() + 2, data.end());
                LOG_INFO("objdump45", filename << ": PRG, " << body.size() << " bytes loaded at $" << std::hex << loadAddr);
            } else if (hasExtension(filename, ".bin") || baseAddrSet) {
                body = data;
                LOG_INFO("objdump45", filename << ": raw binary, " << body.size() << " bytes at $" << std::hex << base);
            } else {
                // Not a recognized format and no -b flag
                std::cerr << "objdump45: " << filename << ": " << err << std::endl;
                std::cerr << "  (use -b ADDR for raw binary files, or .prg/.bin extension)" << std::endl;
                exitCode = 1;
                continue;
            }

            if (showFileHeader) {
                std::cout << std::endl;
                std::cout << filename << ":     file format "
                          << (isPrg ? "prg" : "raw binary") << std::endl;
                printf("base address: $%04x, size: %zu bytes\n", base, body.size());
            }
            if (showContents) {
                hexDump(body, base, isPrg ? "PRG" : "BIN");
            }
            if (showDisasm) {
                std::map<uint32_t, std::string> prgSymbols;
                std::string effectiveMap = mapFile;
                // Auto-discover map file: foo.prg → foo.map
                if (effectiveMap.empty() && isPrg) {
                    std::string autoMap = filename.substr(0, filename.size() - 4) + ".map";
                    std::ifstream probe(autoMap);
                    if (probe.good()) {
                        effectiveMap = autoMap;
                    }
                }
                SourceLineMap srcLines;
                uint32_t textEnd = 0;
                if (!effectiveMap.empty()) {
                    LOG_INFO("objdump45", "symbols from the map " << effectiveMap);
                    prgSymbols = loadMapSymbols(effectiveMap);
                    srcLines = loadMapSourceLines(effectiveMap);
                    textEnd = loadMapTextEnd(effectiveMap);
                } else if (isPrg) {
                    // no map: the debug file (ln45 -g), foo.prg -> foo.dbg
                    LOG_INFO("objdump45", "symbols from the debug file " << filename.substr(0, filename.size() - 4) << ".dbg, if there");
                    loadDebugFile(filename.substr(0, filename.size() - 4) + ".dbg", prgSymbols, srcLines);
                }
                // Detect and render BASIC SYS stub before disassembly
                size_t stubSkip = 0;
                if (isPrg) {
                    uint32_t sysTarget = 0;
                    size_t stubSize = detectBasicStub(body, base, sysTarget);
                    if (stubSize > 0) {
                        stubSkip = stubSize;
                        std::cout << std::endl;
                        printf("Disassembly of section PRG (%s):\n", filename.c_str());
                        printf("\n%08x <BASIC stub>:\n", base);
                        // Show raw bytes
                        printf("    %04x:\t", base);
                        for (size_t b = 0; b < stubSize; b++)
                            printf("%02x ", body[b]);
                        uint16_t lineNum = body[2] | (body[3] << 8);
                        printf("\t; %u SYS %u ($%04x)\n", lineNum, sysTarget, sysTarget);
                    }
                }

                // Limit disassembly to TEXT segment if map provides boundary
                size_t codeLen = body.size() - stubSkip;
                if (textEnd > 0 && textEnd > base + stubSkip) {
                    size_t textSize = textEnd - (base + stubSkip);
                    if (textSize < codeLen) codeLen = textSize;
                }
                std::vector<uint8_t> codeBody(body.begin() + stubSkip, body.begin() + stubSkip + codeLen);
                uint32_t codeBase = base + (uint32_t)stubSkip;
                // a C64 program (loaded at $0801) is 6510 code unless --cpu says otherwise
                bool c64 = isPrg && !cpuSet && !baseAddrSet && base == 0x0801;
                if (c64 && table6510.empty()) table6510 = buildOpcodeTable(DisCpu::CPU6510);
                // (its symbols: the C64's, unless -S named some)
                std::map<uint32_t, std::string> kept;
                bool swap = c64 && symFiles.empty() && !symNone;
                if (swap) {
                    if (!c64Loaded) { loadSyms("c64", c64Syms, true); c64Loaded = true; }
                    kept.swap(g_fileSyms);
                    g_fileSyms = c64Syms;
                }
                disassembleSection(codeBody, codeBase,
                                 stubSkip ? "code" : (isPrg ? "PRG" : "BIN"),
                                 c64 ? table6510 : opcodeTable, prgSymbols, filename, srcLines, nullptr,
                                 c64 ? DisCpu::CPU6510 : cpu,
                                 c64 ? "loaded at $0801: decoded as 6510 (C64) code; --cpu=45gs02 for MEGA65 code" : nullptr);
                if (swap) g_fileSyms.swap(kept);
            }
            // -h, -t, -r silently produce no output for raw binaries
        }
    }

    return exitCode;
}
