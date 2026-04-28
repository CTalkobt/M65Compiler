#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>
#include <algorithm>
#include <cstdio>
#include "O45Reader.hpp"
#include "O45Linker.hpp"

static char segmentLetter(uint8_t seg) {
    switch (seg) {
        case SEG_TEXT: return 'T';
        case SEG_DATA: return 'D';
        case SEG_BSS:  return 'B';
        case SEG_ZP:   return 'Z';
        default:       return 'A';
    }
}

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

static void printUsage(const char* progName) {
    std::cout << "Usage: " << progName << " [options] <file.o45> [...]" << std::endl;
    std::cout << "Symbol options:" << std::endl;
    std::cout << "  -u       Display only undefined (imported) symbols" << std::endl;
    std::cout << "  -g       Display only global (exported) symbols" << std::endl;
    std::cout << "  -n       Sort by address instead of name" << std::endl;
    std::cout << "  -p       No sorting (display in file order)" << std::endl;
    std::cout << "  -R       Reverse sort order" << std::endl;
    std::cout << "  -A       Prepend filename to each symbol line" << std::endl;
    std::cout << "Inspection options:" << std::endl;
    std::cout << "  -h       Display file header (mode, CPU, segment bases/lengths)" << std::endl;
    std::cout << "  -x       Hex dump of segment bodies" << std::endl;
    std::cout << "  -r       Display relocation table entries" << std::endl;
    std::cout << "  -s       Display segment sizes summary" << std::endl;
    std::cout << "  -a       Display all (header + relocations + symbols)" << std::endl;
    std::cout << "  -?       Display this help message" << std::endl;
}

static void printHeader(const O45File& obj, const std::string& filename) {
    bool is32 = (obj.mode & O45_MODE_SIZE32) != 0;
    std::cout << "File:    " << filename << std::endl;
    std::cout << "Format:  " << (is32 ? ".o45 (32-bit)" : ".o65 (16-bit)") << std::endl;
    printf("CPU:     %s ($%02X)\n", cpuName(obj.cpuId), obj.cpuId);
    printf("Mode:    $%04X", obj.mode);
    std::string flags;
    if (obj.mode & O45_MODE_SIZE32)  flags += " SIZE32";
    if (obj.mode & O45_MODE_CPUEXT)  flags += " CPUEXT";
    if (obj.mode & O45_MODE_BSSZERO) flags += " BSSZERO";
    if (obj.mode & O45_MODE_PAGED)   flags += " PAGED";
    if (obj.mode & O45_MODE_CHAIN)   flags += " CHAIN";
    if (obj.mode & O45_MODE_SIMPLE)  flags += " SIMPLE";
    if (!flags.empty()) std::cout << " (" << flags.substr(1) << ")";
    std::cout << std::endl;
    printf("Text:    base=$%04X  len=%u\n", obj.tbase, obj.tlen);
    printf("Data:    base=$%04X  len=%u\n", obj.dbase, obj.dlen);
    printf("BSS:     base=$%04X  len=%u\n", obj.bbase, obj.blen);
    printf("ZP:      base=$%04X  len=%u\n", obj.zbase, obj.zlen);

    if (!obj.options.empty()) {
        std::cout << "Options:" << std::endl;
        for (const auto& opt : obj.options) {
            printf("  type=$%02X", opt.type);
            switch (opt.type) {
                case OPT_FNAME:   std::cout << " FNAME"; break;
                case OPT_OS:      std::cout << " OS"; break;
                case OPT_ASM:     std::cout << " ASM"; break;
                case OPT_AUTHOR:  std::cout << " AUTHOR"; break;
                case OPT_CREATED: std::cout << " CREATED"; break;
                default:          std::cout << " USER"; break;
            }
            // Try to print as string if it looks like text
            bool isText = true;
            for (size_t i = 0; i < opt.data.size(); i++) {
                uint8_t c = opt.data[i];
                if (c == 0 && i == opt.data.size() - 1) continue; // trailing NUL ok
                if (c < 0x20 || c > 0x7E) { isText = false; break; }
            }
            if (isText && !opt.data.empty()) {
                std::string s(opt.data.begin(), opt.data.end());
                if (!s.empty() && s.back() == '\0') s.pop_back();
                std::cout << " \"" << s << "\"";
            } else {
                std::cout << " [";
                for (size_t i = 0; i < opt.data.size(); i++) {
                    if (i > 0) std::cout << " ";
                    printf("%02X", opt.data[i]);
                }
                std::cout << "]";
            }
            std::cout << std::endl;
        }
    }
}

static void hexDump(const std::vector<uint8_t>& body, uint32_t base, const std::string& label) {
    if (body.empty()) return;
    std::cout << label << " (" << body.size() << " bytes):" << std::endl;
    for (size_t i = 0; i < body.size(); i += 16) {
        printf("  %04X: ", (unsigned)(base + i));
        for (size_t j = 0; j < 16; j++) {
            if (i + j < body.size()) printf("%02X ", body[i + j]);
            else printf("   ");
            if (j == 7) printf(" ");
        }
        printf(" |");
        for (size_t j = 0; j < 16 && i + j < body.size(); j++) {
            uint8_t c = body[i + j];
            printf("%c", (c >= 0x20 && c < 0x7F) ? c : '.');
        }
        printf("|\n");
    }
}

static void printRelocs(const std::vector<uint8_t>& raw, const O45File& obj,
                        const std::string& segLabel) {
    auto relocs = O45RelocDecoder::decode(raw);
    if (relocs.empty()) return;

    std::cout << segLabel << " relocations (" << relocs.size() << "):" << std::endl;
    for (const auto& r : relocs) {
        printf("  %04X  %-10s  %-6s",
               r.offset, relocTypeName((uint8_t)r.type), segmentName((uint8_t)r.segment));
        if (r.segment == SEG_EXTERNAL) {
            if (r.symbolIndex < obj.imports.size()) {
                std::cout << "  " << obj.imports[r.symbolIndex].name;
            } else {
                printf("  [index %u]", r.symbolIndex);
            }
        }
        if (r.type == R_HIGH) {
            printf("  extra=$%02X", r.extra);
        }
        std::cout << std::endl;
    }
}

static void printSizes(const O45File& obj, const std::string& filename) {
    uint32_t total = obj.tlen + obj.dlen + obj.blen + obj.zlen;
    printf("%8u %8u %8u %8u %8u  %s\n",
           obj.tlen, obj.dlen, obj.blen, obj.zlen, total, filename.c_str());
}

int main(int argc, char** argv) {
    bool onlyUndefined = false;
    bool onlyGlobal = false;
    bool sortByAddress = false;
    bool noSort = false;
    bool reverseSort = false;
    bool prependFilename = false;
    bool showHeader = false;
    bool showRelocs = false;
    bool showSizes = false;
    bool showHex = false;
    bool showAll = false;
    bool showSymbols = true; // default behavior
    std::vector<std::string> files;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-?" || arg == "--help") { printUsage(argv[0]); return 0; }
        else if (arg == "-u") onlyUndefined = true;
        else if (arg == "-g") onlyGlobal = true;
        else if (arg == "-n") sortByAddress = true;
        else if (arg == "-p") noSort = true;
        else if (arg == "-R") reverseSort = true;
        else if (arg == "-A" || arg == "-o") prependFilename = true;
        else if (arg == "-h") { showHeader = true; showSymbols = false; }
        else if (arg == "-r") { showRelocs = true; showSymbols = false; }
        else if (arg == "-s") { showSizes = true; showSymbols = false; }
        else if (arg == "-x") { showHex = true; showSymbols = false; }
        else if (arg == "-a") { showAll = true; showHeader = true; showRelocs = true; showHex = true; showSymbols = true; }
        else files.push_back(arg);
    }

    if (files.empty()) {
        std::cerr << "nm45: no input files" << std::endl;
        printUsage(argv[0]);
        return 1;
    }

    bool multiFile = files.size() > 1;
    int exitCode = 0;

    // Print sizes header once
    if (showSizes && !showAll) {
        printf("%8s %8s %8s %8s %8s  %s\n", "text", "data", "bss", "zp", "total", "filename");
    }

    for (const auto& filename : files) {
        std::ifstream in(filename, std::ios::binary);
        if (!in.is_open()) {
            std::cerr << "nm45: " << filename << ": cannot open" << std::endl;
            exitCode = 1;
            continue;
        }

        std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)),
                                   std::istreambuf_iterator<char>());
        in.close();

        O45File obj;
        std::string err;
        if (!O45Reader::read(data, obj, err)) {
            std::cerr << "nm45: " << filename << ": " << err << std::endl;
            exitCode = 1;
            continue;
        }

        // Sizes mode — compact one-line-per-file output
        if (showSizes && !showAll) {
            printSizes(obj, filename);
            continue;
        }

        if (multiFile && !prependFilename) {
            std::cout << std::endl << filename << ":" << std::endl;
        }

        // Header
        if (showHeader) {
            printHeader(obj, filename);
            std::cout << std::endl;
        }

        // Hex dump
        if (showHex) {
            hexDump(obj.textBody, obj.tbase, "Text body");
            hexDump(obj.dataBody, obj.dbase, "Data body");
            std::cout << std::endl;
        }

        // Relocations
        if (showRelocs) {
            printRelocs(obj.textRelocs, obj, "Text");
            printRelocs(obj.dataRelocs, obj, "Data");
            if (!obj.textRelocs.empty() || !obj.dataRelocs.empty())
                std::cout << std::endl;
        }

        // Sizes (within -a mode)
        if (showAll) {
            printf("Sizes:   text=%u  data=%u  bss=%u  zp=%u  total=%u\n\n",
                   obj.tlen, obj.dlen, obj.blen, obj.zlen,
                   obj.tlen + obj.dlen + obj.blen + obj.zlen);
        }

        // Symbols
        if (showSymbols) {
            struct SymEntry {
                std::string name;
                char type;
                uint32_t offset;
                bool isUndefined;
            };
            std::vector<SymEntry> syms;

            if (!onlyGlobal) {
                for (const auto& imp : obj.imports) {
                    syms.push_back({imp.name, 'U', 0, true});
                }
            }

            if (!onlyUndefined) {
                for (const auto& exp : obj.exports) {
                    char letter = exp.isWeak() ? 'W' : segmentLetter(exp.segmentId());
                    syms.push_back({exp.name, letter, exp.offset, false});
                }
            }

            if (!noSort) {
                if (sortByAddress) {
                    std::sort(syms.begin(), syms.end(), [](const SymEntry& a, const SymEntry& b) {
                        if (a.isUndefined != b.isUndefined) return b.isUndefined;
                        if (a.offset != b.offset) return a.offset < b.offset;
                        return a.name < b.name;
                    });
                } else {
                    std::sort(syms.begin(), syms.end(),
                              [](const SymEntry& a, const SymEntry& b) { return a.name < b.name; });
                }
                if (reverseSort) std::reverse(syms.begin(), syms.end());
            }

            std::string prefix = prependFilename ? (filename + ":") : "";

            for (const auto& s : syms) {
                if (s.isUndefined) {
                    std::cout << prefix << "         U " << s.name << std::endl;
                } else {
                    std::cout << prefix
                              << std::setfill('0') << std::setw(8) << std::hex << s.offset
                              << " " << s.type << " " << s.name << std::endl;
                }
            }
        }
    }

    return exitCode;
}
