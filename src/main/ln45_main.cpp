#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "O45Reader.hpp"
#include "O45Linker.hpp"
#include "O45Archive.hpp"
#include "Version.hpp"

static void printUsage(const char* progName) {
    std::cout << "Usage: " << progName << " [options] <file1.o45> [file2.o45 ...] [-l lib.lib ...]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -o <file>      Output filename (default: a.out, or a.prg if -prg)" << std::endl;
    std::cout << "  -prg           Output as PRG with 2-byte load address header" << std::endl;
    std::cout << "  -basic         Output as PRG with BASIC SYS stub (loads at $2001, code at $200D)" << std::endl;
    std::cout << "  -t <addr>      Set text segment base address (hex, e.g., -t 2000)" << std::endl;
    std::cout << "  -d <addr>      Set data segment base address (hex; default: after text)" << std::endl;
    std::cout << "  -b <addr>      Set BSS segment base address (hex; default: after data)" << std::endl;
    std::cout << "  -z <addr>      Set zero page base address (hex; default: 02)" << std::endl;
    std::cout << "  -l <lib.lib>   Link against a library archive (selective linking)" << std::endl;
    std::cout << "  -m             Print linker map (symbol addresses) after linking" << std::endl;
    std::cout << "  -M <file>      Write detailed linker map file (segments, objects, symbols)" << std::endl;
    std::cout << "  -?             Display this help message" << std::endl;
}

static uint32_t parseAddr(const std::string& s) {
    return (uint32_t)std::stoul(s, nullptr, 16);
}

int main(int argc, char** argv) {
    std::string outputFile;
    bool outputSet = false;
    bool isPrg = false;
    bool isBasic = false;
    bool printMap = false;
    std::string mapFile;
    uint32_t textBase = 0x2000; // sensible MEGA65 default
    bool textBaseSet = false;
    uint32_t dataBase = 0; bool dataBaseSet = false;
    uint32_t bssBase = 0;  bool bssBaseSet = false;
    uint32_t zpBase = 0;   bool zpBaseSet = false;
    std::vector<std::string> inputFiles;
    std::vector<std::string> libFiles;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-V" || arg == "--version") { std::cout << suiteVersionString("ln45") << std::endl; return 0; }
        else if (arg == "-?" || arg == "--help") { printUsage(argv[0]); return 0; }
        else if (arg == "-o" && i + 1 < argc) { outputFile = argv[++i]; outputSet = true; }
        else if (arg == "-prg") { isPrg = true; }
        else if (arg == "-basic") { isBasic = true; isPrg = true; }
        else if (arg == "-m") { printMap = true; }
        else if (arg == "-M" && i + 1 < argc) { mapFile = argv[++i]; }
        else if (arg == "-l" && i + 1 < argc) { libFiles.push_back(argv[++i]); }
        else if (arg == "-t" && i + 1 < argc) { textBase = parseAddr(argv[++i]); textBaseSet = true; }
        else if (arg == "-d" && i + 1 < argc) { dataBase = parseAddr(argv[++i]); dataBaseSet = true; }
        else if (arg == "-b" && i + 1 < argc) { bssBase = parseAddr(argv[++i]); bssBaseSet = true; }
        else if (arg == "-z" && i + 1 < argc) { zpBase = parseAddr(argv[++i]); zpBaseSet = true; }
        else if (arg.length() > 4 && arg.substr(arg.length() - 4) == ".lib") { libFiles.push_back(arg); }
        else inputFiles.push_back(arg);
    }

    if (inputFiles.empty()) {
        std::cerr << "ln45: no input files" << std::endl;
        printUsage(argv[0]);
        return 1;
    }

    // Auto-detect PRG from output filename
    if (!isPrg && outputSet && outputFile.length() >= 4 &&
        outputFile.substr(outputFile.length() - 4) == ".prg") {
        isPrg = true;
    }

    if (!outputSet) {
        outputFile = isPrg ? "a.prg" : "a.out";
    }

    // If -basic, code starts at $200D (after 12-byte BASIC stub at $2001 on MEGA65)
    if (isBasic && !textBaseSet) {
        textBase = 0x200D;
    } else if (isPrg && !textBaseSet) {
        textBase = 0x2000;
    }

    // Load all input objects
    O45Linker linker;
    linker.setTextBase(textBase);
    if (dataBaseSet) linker.setDataBase(dataBase);
    if (bssBaseSet)  linker.setBssBase(bssBase);
    if (zpBaseSet)   linker.setZpBase(zpBase);

    for (const auto& filename : inputFiles) {
        std::ifstream in(filename, std::ios::binary);
        if (!in.is_open()) {
            std::cerr << "ln45: cannot open " << filename << std::endl;
            return 1;
        }
        std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)),
                                   std::istreambuf_iterator<char>());
        in.close();

        O45File obj;
        std::string err;
        if (!O45Reader::read(data, obj, err)) {
            std::cerr << "ln45: " << filename << ": " << err << std::endl;
            return 1;
        }

        linker.addObject(filename, obj);
    }

    // Load libraries
    for (const auto& libname : libFiles) {
        std::ifstream in(libname, std::ios::binary);
        if (!in.is_open()) {
            std::cerr << "ln45: cannot open library " << libname << std::endl;
            return 1;
        }
        std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)),
                                   std::istreambuf_iterator<char>());
        in.close();

        Ar45Archive lib;
        std::string err;
        if (!Ar45Reader::read(data, lib, err)) {
            std::cerr << "ln45: " << libname << ": " << err << std::endl;
            return 1;
        }

        linker.addLibrary(libname, lib);
    }

    // Link (for -basic, we add the header ourselves)
    std::string linkErr;
    auto binary = linker.link(linkErr, isPrg && !isBasic);
    if (binary.empty()) {
        std::cerr << "ln45: link error: " << linkErr << std::endl;
        return 1;
    }

    // Prepend BASIC SYS stub if requested
    if (isBasic) {
        // MEGA65 BASIC starts at $2001 (C64 uses $0801)
        uint16_t basicBase = 0x2001;

        // Build: load address + BASIC stub (12 bytes) + binary
        std::vector<uint8_t> basicPrg;

        // 2-byte PRG load address
        basicPrg.push_back((uint8_t)(basicBase & 0xFF));
        basicPrg.push_back((uint8_t)(basicBase >> 8));

        // BASIC stub: 10 SYS <textBase>
        std::string addrStr = std::to_string(textBase);
        // No padding — real BASIC SYS tokens don't pad

        uint16_t nextLine = basicBase + 2 + 2 + 1 + (uint16_t)addrStr.length() + 1;
        basicPrg.push_back((uint8_t)(nextLine & 0xFF));
        basicPrg.push_back((uint8_t)(nextLine >> 8));
        basicPrg.push_back(0x0A); basicPrg.push_back(0x00); // line number 10
        basicPrg.push_back(0x9E); // SYS token
        for (char c : addrStr) {
            basicPrg.push_back((uint8_t)c);
        }
        basicPrg.push_back(0x00); // end of BASIC line
        basicPrg.push_back(0x00); basicPrg.push_back(0x00); // end of BASIC program

        // Append the linked binary
        basicPrg.insert(basicPrg.end(), binary.begin(), binary.end());
        binary = std::move(basicPrg);
    }

    // Write output
    std::ofstream out(outputFile, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "ln45: cannot write " << outputFile << std::endl;
        return 1;
    }
    out.write(reinterpret_cast<const char*>(binary.data()), binary.size());
    out.close();

    std::cout << "Linked " << inputFiles.size() << " object"
              << (inputFiles.size() > 1 ? "s" : "")
              << " to " << outputFile << " (" << binary.size() << " bytes)" << std::endl;

    // Print map if requested
    if (printMap) {
        std::cout << std::endl << "Symbol map:" << std::endl;
        for (const auto& [name, addr] : linker.getSymbolMap()) {
            printf("  %08x %s\n", addr, name.c_str());
        }
    }

    // Write detailed map file if requested
    if (!mapFile.empty()) {
        std::ofstream mf(mapFile);
        if (!mf.is_open()) {
            std::cerr << "ln45: cannot write map file " << mapFile << std::endl;
            return 1;
        }
        linker.writeMap(mf);
        mf.close();
        std::cout << "Map file: " << mapFile << std::endl;
    }

    return 0;
}
