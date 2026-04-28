#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "O45Reader.hpp"
#include "O45Linker.hpp"
#include "O45Archive.hpp"

static void printUsage(const char* progName) {
    std::cout << "Usage: " << progName << " [options] <file1.o45> [file2.o45 ...] [-l lib.lib ...]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -o <file>      Output filename (default: a.out, or a.prg if -prg)" << std::endl;
    std::cout << "  -prg           Output as PRG with 2-byte load address header" << std::endl;
    std::cout << "  -t <addr>      Set text segment base address (hex, e.g., -t 2000)" << std::endl;
    std::cout << "  -d <addr>      Set data segment base address (hex; default: after text)" << std::endl;
    std::cout << "  -b <addr>      Set BSS segment base address (hex; default: after data)" << std::endl;
    std::cout << "  -z <addr>      Set zero page base address (hex; default: 02)" << std::endl;
    std::cout << "  -l <lib.lib>   Link against a library archive (selective linking)" << std::endl;
    std::cout << "  -m             Print linker map (symbol addresses) after linking" << std::endl;
    std::cout << "  -?             Display this help message" << std::endl;
}

static uint32_t parseAddr(const std::string& s) {
    return (uint32_t)std::stoul(s, nullptr, 16);
}

int main(int argc, char** argv) {
    std::string outputFile;
    bool outputSet = false;
    bool isPrg = false;
    bool printMap = false;
    uint32_t textBase = 0x2000; // sensible MEGA65 default
    bool textBaseSet = false;
    uint32_t dataBase = 0; bool dataBaseSet = false;
    uint32_t bssBase = 0;  bool bssBaseSet = false;
    uint32_t zpBase = 0;   bool zpBaseSet = false;
    std::vector<std::string> inputFiles;
    std::vector<std::string> libFiles;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-?" || arg == "--help") { printUsage(argv[0]); return 0; }
        else if (arg == "-o" && i + 1 < argc) { outputFile = argv[++i]; outputSet = true; }
        else if (arg == "-prg") { isPrg = true; }
        else if (arg == "-m") { printMap = true; }
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

    // If -prg but no explicit text base, use $2000
    if (isPrg && !textBaseSet) {
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

    // Link
    std::string linkErr;
    auto binary = linker.link(linkErr, isPrg);
    if (binary.empty()) {
        std::cerr << "ln45: link error: " << linkErr << std::endl;
        return 1;
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

    return 0;
}
