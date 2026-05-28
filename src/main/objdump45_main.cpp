#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>
#include <algorithm>
#include <map>
#include <cstdio>
#include "O45Reader.hpp"
#include "O45Linker.hpp"
#include "O45FormatUtil.hpp"
#include "AssemblerOpcodeDatabase.hpp"
#include "AssemblerTypes.hpp"
#include "Version.hpp"

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
    switch (rtype & O45_RTYPE_MASK) {
        case R_LOW:      return "R_LOW";
        case R_HIGH:     return "R_HIGH";
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
};

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
            snprintf(buf, sizeof(buf), "#$%02X", bytes[0]);
            return buf;
        case AddressingMode::IMMEDIATE16:
            snprintf(buf, sizeof(buf), "#$%04X", bytes[0] | (bytes[1] << 8));
            return buf;
        case AddressingMode::BASE_PAGE:
            snprintf(buf, sizeof(buf), "$%02X", bytes[0]);
            return buf;
        case AddressingMode::BASE_PAGE_X:
            snprintf(buf, sizeof(buf), "$%02X,x", bytes[0]);
            return buf;
        case AddressingMode::BASE_PAGE_Y:
            snprintf(buf, sizeof(buf), "$%02X,y", bytes[0]);
            return buf;
        case AddressingMode::ABSOLUTE:
            snprintf(buf, sizeof(buf), "$%04X", bytes[0] | (bytes[1] << 8));
            return buf;
        case AddressingMode::ABSOLUTE_X:
            snprintf(buf, sizeof(buf), "$%04X,x", bytes[0] | (bytes[1] << 8));
            return buf;
        case AddressingMode::ABSOLUTE_Y:
            snprintf(buf, sizeof(buf), "$%04X,y", bytes[0] | (bytes[1] << 8));
            return buf;
        case AddressingMode::ABSOLUTE_INDIRECT:
            snprintf(buf, sizeof(buf), "($%04X)", bytes[0] | (bytes[1] << 8));
            return buf;
        case AddressingMode::ABSOLUTE_X_INDIRECT:
            snprintf(buf, sizeof(buf), "($%04X,x)", bytes[0] | (bytes[1] << 8));
            return buf;
        case AddressingMode::BASE_PAGE_X_INDIRECT:
            snprintf(buf, sizeof(buf), "($%02X,x)", bytes[0]);
            return buf;
        case AddressingMode::BASE_PAGE_INDIRECT_Y:
            snprintf(buf, sizeof(buf), "($%02X),y", bytes[0]);
            return buf;
        case AddressingMode::BASE_PAGE_INDIRECT_Z:
            snprintf(buf, sizeof(buf), "($%02X),z", bytes[0]);
            return buf;
        case AddressingMode::FLAT_INDIRECT_Z:
            snprintf(buf, sizeof(buf), "[$%02X],z", bytes[0]);
            return buf;
        case AddressingMode::BASE_PAGE_INDIRECT_SP_Y:
            snprintf(buf, sizeof(buf), "($%02X,sp),y", bytes[0]);
            return buf;
        case AddressingMode::RELATIVE: {
            int8_t offset = (int8_t)bytes[0];
            uint32_t target = pc + 2 + offset;
            snprintf(buf, sizeof(buf), "$%04X", target & 0xFFFF);
            return buf;
        }
        case AddressingMode::RELATIVE16: {
            int16_t offset = (int16_t)(bytes[0] | (bytes[1] << 8));
            uint32_t target = pc + 3 + offset;
            snprintf(buf, sizeof(buf), "$%04X", target & 0xFFFF);
            return buf;
        }
        case AddressingMode::BASE_PAGE_RELATIVE: {
            int8_t rel = (int8_t)bytes[1];
            uint32_t target = pc + 3 + rel;
            snprintf(buf, sizeof(buf), "$%02X,$%04X", bytes[0], target & 0xFFFF);
            return buf;
        }
        case AddressingMode::STACK_RELATIVE:
            snprintf(buf, sizeof(buf), "$%02X,s", bytes[0]);
            return buf;
        case AddressingMode::STACK_RELATIVE_INDIRECT_Y:
            snprintf(buf, sizeof(buf), "($%02X,s),y", bytes[0]);
            return buf;
        case AddressingMode::LINEAR_ABSOLUTE:
            snprintf(buf, sizeof(buf), "$%06X",
                     bytes[0] | (bytes[1] << 8) | (bytes[2] << 16));
            return buf;
        case AddressingMode::LINEAR_ABSOLUTE_X:
            snprintf(buf, sizeof(buf), "$%06X,x",
                     bytes[0] | (bytes[1] << 8) | (bytes[2] << 16));
            return buf;
        case AddressingMode::LINEAR_ABSOLUTE_Y:
            snprintf(buf, sizeof(buf), "$%06X,y",
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
    printf("flags $%04X", obj.mode);
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
            printf("  %s", formatFuncAttr(exp.funcAttr, obj.zbase).c_str());
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
            if (r.type == R_HIGH) {
                printf("  extra=$%02X", r.extra);
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

// Build symbol map: address → name (for branch/call target annotation)
// Adds the appropriate segment base to each export offset.
static std::map<uint32_t, std::string> buildSymbolMap(const O45File& obj) {
    std::map<uint32_t, std::string> syms;
    for (const auto& exp : obj.exports) {
        uint32_t base = 0;
        switch (exp.segmentId()) {
            case SEG_TEXT: base = obj.tbase; break;
            case SEG_DATA: base = obj.dbase; break;
            case SEG_BSS:  base = obj.bbase; break;
            case SEG_ZP:   base = obj.zbase; break;
        }
        syms[base + exp.offset] = exp.name;
    }
    return syms;
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

static void disassembleSection(const std::vector<uint8_t>& body, uint32_t base,
                               const char* sectionName,
                               const std::map<uint8_t, OpcodeInfo>& opcodeTable,
                               const std::map<uint32_t, std::string>& symbols,
                               const std::string& filename,
                               const SourceLineMap& sourceLines = {}) {
    if (body.empty()) return;
    std::cout << std::endl;
    printf("Disassembly of section %s (%s):\n", sectionName, filename.c_str());

    int lastSourceLine = -1;
    std::string lastSourceFile;

    size_t i = 0;
    while (i < body.size()) {
        uint32_t pc = base + (uint32_t)i;

        // Print label if a symbol exists at this address
        auto symIt = symbols.find(pc);
        if (symIt != symbols.end()) {
            std::cout << std::endl;
            printf("%08x <%s>:\n", pc, symIt->second.c_str());
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

        uint8_t opcode = body[i];
        auto it = opcodeTable.find(opcode);

        if (it == opcodeTable.end()) {
            // Unknown opcode — emit as .byte
            printf("    %04x:\t%02x          \t.byte\t$%02X\n", pc, opcode, opcode);
            i++;
            continue;
        }

        const OpcodeInfo& info = it->second;
        int opSize = operandSize(info.mode);
        int instrSize = 1 + opSize;

        // Check if we have enough bytes
        if (i + instrSize > body.size()) {
            // Not enough bytes — dump remainder as .byte
            for (size_t j = i; j < body.size(); j++) {
                printf("    %04x:\t%02x          \t.byte\t$%02X\n",
                       (unsigned)(base + j), body[j], body[j]);
            }
            break;
        }

        // Format hex bytes column (up to 4 bytes shown)
        char hexCol[20];
        switch (instrSize) {
            case 1:
                snprintf(hexCol, sizeof(hexCol), "%02x          ", opcode);
                break;
            case 2:
                snprintf(hexCol, sizeof(hexCol), "%02x %02x       ",
                         opcode, body[i + 1]);
                break;
            case 3:
                snprintf(hexCol, sizeof(hexCol), "%02x %02x %02x    ",
                         opcode, body[i + 1], body[i + 2]);
                break;
            case 4:
                snprintf(hexCol, sizeof(hexCol), "%02x %02x %02x %02x",
                         opcode, body[i + 1], body[i + 2], body[i + 3]);
                break;
            default:
                snprintf(hexCol, sizeof(hexCol), "%02x          ", opcode);
                break;
        }

        const uint8_t* operandBytes = (opSize > 0) ? &body[i + 1] : nullptr;
        std::string operand = formatOperand(info.mode, operandBytes, opSize, pc);

        // Annotate operands with symbol names where the address matches
        std::string annotation;
        if (info.mode == AddressingMode::RELATIVE ||
            info.mode == AddressingMode::RELATIVE16) {
            uint32_t target;
            if (info.mode == AddressingMode::RELATIVE) {
                int8_t offset = (int8_t)body[i + 1];
                target = pc + 2 + offset;
            } else {
                int16_t offset = (int16_t)(body[i + 1] | (body[i + 2] << 8));
                target = pc + 3 + offset;
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
            uint32_t target = body[i + 1] | (body[i + 2] << 8);
            auto tgtSym = symbols.find(target);
            if (tgtSym != symbols.end()) {
                annotation = "\t; <" + tgtSym->second + ">";
            }
        } else if (info.mode == AddressingMode::IMMEDIATE16) {
            uint32_t val = body[i + 1] | (body[i + 2] << 8);
            auto tgtSym = symbols.find(val);
            if (tgtSym != symbols.end()) {
                annotation = "\t; <" + tgtSym->second + ">";
            }
        } else if (info.mode == AddressingMode::BASE_PAGE_RELATIVE) {
            int8_t rel = (int8_t)body[i + 2];
            uint32_t target = (pc + 3 + rel) & 0xFFFF;
            auto tgtSym = symbols.find(target);
            if (tgtSym != symbols.end()) {
                annotation = "\t; <" + tgtSym->second + ">";
            }
        }

        printf("    %04x:\t%s\t%s\t%s%s\n",
               pc, hexCol, info.mnemonic.c_str(), operand.c_str(),
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
    std::cout << "  -a       Display all information" << std::endl;
    std::cout << "  -b ADDR  Set base address for raw binary files (default: $0000)" << std::endl;
    std::cout << "  -m FILE  Load symbols from linker map file (ln45 -M output)" << std::endl;
    std::cout << "  -V       Display version" << std::endl;
    std::cout << "  -?       Display this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Supported formats:" << std::endl;
    std::cout << "  .o45/.o65  Relocatable object files (full header/symbol/reloc support)" << std::endl;
    std::cout << "  .prg       PRG files (2-byte load address header, auto-detected)" << std::endl;
    std::cout << "  .bin       Raw binary files (use -b to set base address)" << std::endl;
}

int main(int argc, char** argv) {
    bool showFileHeader = false;
    bool showSections = false;
    bool showSymbols = false;
    bool showRelocs = false;
    bool showContents = false;
    bool showDisasm = false;
    bool showAll = false;
    uint32_t baseAddr = 0;
    bool baseAddrSet = false;
    std::string mapFile;
    std::vector<std::string> files;

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
                    case 'a': showAll = true; break;
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
        showFileHeader = showSections = showSymbols = showRelocs = showContents = showDisasm = true;
    }

    // If no display options given, default to showing file header
    if (!showFileHeader && !showSections && !showSymbols && !showRelocs &&
        !showContents && !showDisasm) {
        std::cerr << "objdump45: no display option specified (use -f, -h, -t, -r, -s, -d, or -a)" << std::endl;
        printUsage(argv[0]);
        return 1;
    }

    if (files.empty()) {
        std::cerr << "objdump45: no input files" << std::endl;
        printUsage(argv[0]);
        return 1;
    }

    // Build reverse opcode table once
    auto opcodeTable = buildReverseOpcodeTable();

    int exitCode = 0;

    for (const auto& filename : files) {
        std::ifstream in(filename, std::ios::binary);
        if (!in.is_open()) {
            std::cerr << "objdump45: " << filename << ": cannot open" << std::endl;
            exitCode = 1;
            continue;
        }

        std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)),
                                   std::istreambuf_iterator<char>());
        in.close();

        // Try .o45/.o65 first
        O45File obj;
        std::string err;
        bool isObject = O45Reader::read(data, obj, err);

        if (isObject) {
            // ── .o45 / .o65 object file ──
            if (showFileHeader) printFileHeader(obj, filename);
            if (showSections)   printSectionHeaders(obj, filename);
            if (showSymbols)    printSymbolTable(obj, filename);
            if (showRelocs)     printRelocations(obj, filename);
            if (showContents)   printSectionContents(obj);
            if (showDisasm) {
                auto symbols = buildSymbolMap(obj);
                auto srcLines = buildSourceLineMap(obj);
                disassembleSection(obj.textBody, obj.tbase, "TEXT", opcodeTable,
                                 symbols, filename, srcLines);
                disassembleSection(obj.dataBody, obj.dbase, "DATA", opcodeTable,
                                 symbols, filename);
            }
        } else {
            // ── Raw binary or PRG ──
            bool isPrg = hasExtension(filename, ".prg");
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
            } else if (hasExtension(filename, ".bin") || baseAddrSet) {
                body = data;
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
                printf("base address: $%04X, size: %zu bytes\n", base, body.size());
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
                    prgSymbols = loadMapSymbols(effectiveMap);
                    srcLines = loadMapSourceLines(effectiveMap);
                    textEnd = loadMapTextEnd(effectiveMap);
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
                        printf("\t; %u SYS %u ($%04X)\n", lineNum, sysTarget, sysTarget);
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
                disassembleSection(codeBody, codeBase,
                                 stubSkip ? "code" : (isPrg ? "PRG" : "BIN"),
                                 opcodeTable, prgSymbols, filename, srcLines);
            }
            // -h, -t, -r silently produce no output for raw binaries
        }
    }

    return exitCode;
}
