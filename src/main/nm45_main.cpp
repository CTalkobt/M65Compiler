#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>
#include <algorithm>
#include "O45Reader.hpp"

static char segmentLetter(uint8_t seg) {
    switch (seg) {
        case SEG_TEXT: return 'T';
        case SEG_DATA: return 'D';
        case SEG_BSS:  return 'B';
        case SEG_ZP:   return 'Z';
        default:       return 'A'; // absolute
    }
}

static void printUsage(const char* progName) {
    std::cout << "Usage: " << progName << " [options] <file.o45> [...]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -A       Prepend filename to each output line" << std::endl;
    std::cout << "  -g       Display only global (exported) symbols" << std::endl;
    std::cout << "  -n       Sort by address instead of name" << std::endl;
    std::cout << "  -p       No sorting (display in file order)" << std::endl;
    std::cout << "  -r       Reverse sort order" << std::endl;
    std::cout << "  -u       Display only undefined (imported) symbols" << std::endl;
    std::cout << "  -?       Display this help message" << std::endl;
}

int main(int argc, char** argv) {
    bool onlyUndefined = false;
    bool onlyGlobal = false;
    bool sortByAddress = false;
    bool noSort = false;
    bool reverseSort = false;
    bool prependFilename = false;
    std::vector<std::string> files;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-?" || arg == "--help") { printUsage(argv[0]); return 0; }
        else if (arg == "-u") onlyUndefined = true;
        else if (arg == "-g") onlyGlobal = true;
        else if (arg == "-n") sortByAddress = true;
        else if (arg == "-p") noSort = true;
        else if (arg == "-r") reverseSort = true;
        else if (arg == "-A" || arg == "-o") prependFilename = true;
        else files.push_back(arg);
    }

    if (files.empty()) {
        std::cerr << "nm45: no input files" << std::endl;
        printUsage(argv[0]);
        return 1;
    }

    bool multiFile = files.size() > 1;
    int exitCode = 0;

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

        if (multiFile && !prependFilename) {
            std::cout << std::endl << filename << ":" << std::endl;
        }

        // Collect all symbols into a unified list for sorting
        struct SymEntry {
            std::string name;
            char type;       // 'U', 'T', 'D', 'B', 'Z', 'A'
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

        // Sort
        if (!noSort) {
            if (sortByAddress) {
                std::sort(syms.begin(), syms.end(), [](const SymEntry& a, const SymEntry& b) {
                    if (a.isUndefined != b.isUndefined) return b.isUndefined; // defined first
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

    return exitCode;
}
