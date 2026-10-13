#include "LinkerLog.hpp"
#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <string>
#include <unistd.h>
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
    std::cout << "  -L <dir>       Add library search path" << std::endl;
    std::cout << "  -l <lib.lib>   Link against a library archive (selective linking)" << std::endl;
    std::cout << "  -m             Print linker map (symbol addresses) after linking" << std::endl;
    std::cout << "  -M <file>      Write detailed linker map file (segments, objects, symbols)" << std::endl;
    std::cout << "  --no-thunks    Error on calling convention mismatches (no thunk generation)" << std::endl;
    std::cout << "  --no-sac-overlay  Give every static-allocation function its own storage" << std::endl;
    std::cout << "  --no-gc        Keep every library member pulled in, also those only dropped code" << std::endl;
    std::cout << "                 needed or whose weak definition lost (default: drop them)" << std::endl;
    std::cout << "  --print-gc     List the library members dropped" << std::endl;
    std::cout << "  --gc-inputs    Drop the unreached sections of the input objects too (roots: the" << std::endl;
    std::cout << "                 startup code, .keep sections, -u symbols; cc45 -ffunction-sections)" << std::endl;
    std::cout << "  -u <sym>       Keep <sym> and what it reaches" << std::endl;
    std::cout << "  --bss-fill=V   Byte crt0 fills BSS with at startup (default $00; $FF, 0xFF or decimal)" << std::endl;
    std::cout << "  --stack-size=N The CPU stack's share below __stack_top; malloc's heap ends there" << std::endl;
    std::cout << "                 (default 4096; $1000, 0x1000 or decimal)" << std::endl;
    std::cout << "  -Wthunk        Warn (but still generate thunks) on convention mismatches" << std::endl;
    std::cout << "  -v             Also print the cross-module analysis and informational warnings" << std::endl;
    std::cout << "                 (unused global symbols, SAC overlay)" << std::endl;
    std::cout << "  -vv            As -v, plus a trace of every relocation" << std::endl;
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
    int bssFill = -1;
    int stackSize = -1;
    bool gcObjects = true, printGc = false, gcInputs = false;
    std::vector<std::string> keepSyms;
    std::string mapFile;
    uint32_t textBase = 0x2000; // sensible MEGA65 default
    bool textBaseSet = false;
    uint32_t dataBase = 0; bool dataBaseSet = false;
    uint32_t bssBase = 0;  bool bssBaseSet = false;
    uint32_t zpBase = 0;   bool zpBaseSet = false;
    std::vector<std::string> inputFiles;
    std::vector<std::string> libFiles;
    std::vector<std::string> libSearchPaths;
    O45Linker::ThunkMode thunkMode = O45Linker::THUNK_AUTO;
    bool sacOverlay = true;

    // Add paths from CC45_LIB environment variable
    if (const char* envLib = std::getenv("CC45_LIB")) {
        std::string s(envLib);
        size_t pos = 0, found;
        while ((found = s.find(':', pos)) != std::string::npos) {
            if (found > pos) libSearchPaths.push_back(s.substr(pos, found - pos));
            pos = found + 1;
        }
        if (pos < s.size()) libSearchPaths.push_back(s.substr(pos));
    }

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-V" || arg == "--version") { std::cout << suiteVersionString("ln45") << std::endl; return 0; }
        else if (arg == "-?" || arg == "--help") { printUsage(argv[0]); return 0; }
        else if (arg == "-o" && i + 1 < argc) { outputFile = argv[++i]; outputSet = true; }
        else if (arg == "-prg") { isPrg = true; }
        else if (arg == "-basic") { isBasic = true; isPrg = true; }
        else if (arg == "-m") { printMap = true; }
        else if (arg == "-M" && i + 1 < argc) { mapFile = argv[++i]; }
        else if (arg == "-L" && i + 1 < argc) { libSearchPaths.push_back(argv[++i]); }
        else if (arg == "-l" && i + 1 < argc) { libFiles.push_back(argv[++i]); }
        else if (arg == "-t" && i + 1 < argc) { textBase = parseAddr(argv[++i]); textBaseSet = true; }
        else if (arg == "-d" && i + 1 < argc) { dataBase = parseAddr(argv[++i]); dataBaseSet = true; }
        else if (arg == "-b" && i + 1 < argc) { bssBase = parseAddr(argv[++i]); bssBaseSet = true; }
        else if (arg == "-z" && i + 1 < argc) { zpBase = parseAddr(argv[++i]); zpBaseSet = true; }
        else if (arg == "--no-thunks") { thunkMode = O45Linker::THUNK_ERROR; }
        else if (arg == "--no-sac-overlay") { sacOverlay = false; }
        else if (arg == "--no-gc") { gcObjects = false; }
        else if (arg == "--gc-inputs") { gcInputs = true; }
        else if (arg == "-u" && i + 1 < argc) { keepSyms.push_back(argv[++i]); }
        else if (arg == "--print-gc") { printGc = true; }
        else if (arg == "-v") { g_ln45Verbosity = std::max(g_ln45Verbosity, 1); }
        else if (arg == "-vv") { g_ln45Verbosity = 2; }
        else if (arg.rfind("--stack-size=", 0) == 0) {
            std::string v = arg.substr(13);
            int base = 10;
            if (!v.empty() && v[0] == '$') { v = v.substr(1); base = 16; }
            else if (v.size() > 2 && v[0] == '0' && (v[1] == 'x' || v[1] == 'X')) { v = v.substr(2); base = 16; }
            unsigned long sv = 0;
            try { size_t used = 0; sv = std::stoul(v, &used, base); if (used != v.size() || sv > 0xFFFF) throw 0; }
            catch (...) { std::cerr << "ln45: error: invalid --stack-size value '" << arg.substr(13) << "' (0-65535, $1000, 0x1000)" << std::endl; return 1; }
            stackSize = (int)sv;
        }
        else if (arg.rfind("--bss-fill=", 0) == 0) {
            std::string v = arg.substr(11);
            int base = 10;
            if (!v.empty() && v[0] == '$') { v = v.substr(1); base = 16; }
            else if (v.size() > 2 && v[0] == '0' && (v[1] == 'x' || v[1] == 'X')) { v = v.substr(2); base = 16; }
            unsigned long fv = 0;
            try { size_t used = 0; fv = std::stoul(v, &used, base); if (used != v.size() || fv > 255) throw 0; }
            catch (...) { std::cerr << "ln45: error: invalid --bss-fill value '" << arg.substr(11) << "' (a byte: 0-255, $FF, 0xFF)" << std::endl; return 1; }
            bssFill = (int)fv;
        }
        else if (arg == "-Wthunk") { thunkMode = O45Linker::THUNK_WARN; }
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
    linker.setThunkMode(thunkMode);
    linker.setSacOverlay(sacOverlay);
    linker.setGcObjects(gcObjects);
    linker.setGcInputs(gcInputs);
    for (const auto& k : keepSyms) linker.addKeepSymbol(k);
    if (bssFill >= 0) linker.setBssFill((uint8_t)bssFill);
    if (stackSize >= 0) linker.setStackSize((uint16_t)stackSize);

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

    // Last, the standard library directories next to this executable: the
    // build tree's bin/../lib/build, an installed tree's bin/../lib/cc45
    {
        char buf[4096];
        ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
        if (len > 0) {
            buf[len] = '\0';
            std::string self(buf);
            size_t sep = self.find_last_of('/');
            if (sep != std::string::npos) {
                std::string base = self.substr(0, sep + 1);
                libSearchPaths.push_back(base + "../lib/build");
                libSearchPaths.push_back(base + "../lib/cc45");
            }
        }
    }

    // Load libraries
    for (const auto& libname : libFiles) {
        // Try the path as given first, then search CC45_LIB, -L and the
        // standard library directories
        std::string resolvedLib = libname;
        {
            std::ifstream test(resolvedLib, std::ios::binary);
            if (!test.is_open()) {
                bool found = false;
                for (const auto& dir : libSearchPaths) {
                    std::string candidate = dir + "/" + libname;
                    std::ifstream t(candidate, std::ios::binary);
                    if (t.is_open()) { resolvedLib = candidate; found = true; break; }
                }
                if (!found) {
                    std::cerr << "ln45: cannot find library " << libname << std::endl;
                    return 1;
                }
            }
        }
        std::ifstream in(resolvedLib, std::ios::binary);
        if (!in.is_open()) {
            std::cerr << "ln45: cannot open library " << resolvedLib << std::endl;
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

    // Link (we always add the header ourselves in ln45_main)
    std::string linkErr;
    auto binary = linker.link(linkErr, false);
    if (binary.empty()) {
        std::cerr << "ln45: link error: " << linkErr << std::endl;
        return 1;
    }

    if (printGc || g_ln45Verbosity >= 1)
    {
        for (const auto& n : linker.discardedObjects())
            std::cerr << "ln45: dropped " << n << " (nothing live references it)" << std::endl;
        for (const auto& n : linker.discardedSectionNames())
            std::cerr << "ln45: dropped section " << n << std::endl;
    }

    // The 16-bit CPU stack grows down from __stack_top towards the end of the
    // program (BSS and the private base page): make sure there is room
    {
        const auto& syms = linker.getSymbolMap();
        auto st = syms.find("__stack_top");
        if (st != syms.end() && st->second != 0) {
            uint32_t end = 0;
            auto be = syms.find("__bss_end");
            if (be != syms.end()) end = be->second;
            auto zp = syms.find("__zp_page");
            if (zp != syms.end() && zp->second + 0x100 > end) end = zp->second + 0x100;
            if (end > 0 && end > st->second) {
                std::cerr << "ln45: error: program ends at $" << std::hex << std::uppercase << end
                          << ", above the CPU stack top $" << st->second << std::dec
                          << " (cc45 -fstack-top=ADDR or -fno-stack16)" << std::endl;
                return 1;
            }
            if (end > 0 && st->second + 1 - end < 0x200) {
                std::cerr << "ln45: warning: only " << (st->second + 1 - end)
                          << " bytes left for the CPU stack between the program end ($"
                          << std::hex << std::uppercase << end << ") and $" << st->second
                          << std::dec << std::endl;
            }
        }
        // malloc's automatic heap runs from the program end to __stack_size
        // below __stack_top ($C000 with the 8-bit stack): say when a program
        // that uses it leaves (almost) nothing for it
        auto hp = syms.find("__heap_ptr");
        auto hs = syms.find("__heap_start"), he = syms.find("__heap_end");
        bool autoHeap = (hs == syms.end() || hs->second == 0) && (he == syms.end() || he->second == 0);
        if (hp != syms.end() && autoHeap) {
            uint32_t top = st != syms.end() ? st->second : 0;
            auto ss = syms.find("__stack_size");
            uint32_t size = ss != syms.end() ? ss->second : 0x1000;
            uint32_t heapEnd = top ? (top + 1 > size ? top + 1 - size : 0) : 0xC000;
            uint32_t end = 0;
            auto be = syms.find("__bss_end");
            if (be != syms.end()) end = be->second;
            auto zp = syms.find("__zp_page");
            if (zp != syms.end() && zp->second + 0x100 > end) end = zp->second + 0x100;
            end = (end + 1) & ~1u;
            if (end + 4 > heapEnd) {
                std::cerr << "ln45: warning: no heap: the program ends at $" << std::hex << std::uppercase << end
                          << ", at or above the heap's end $" << heapEnd << std::dec;
                if (top) std::cerr << " (" << size << " bytes below the CPU stack top for the stack)";
                std::cerr << "; malloc returns NULL. A smaller stack share (--stack-size, cc45 -fstack-size)"
                             " or a heap elsewhere (__heap_start/__heap_end) makes room" << std::endl;
            } else if (heapEnd - end < 0x400) {
                std::cerr << "ln45: warning: only " << (heapEnd - end) << " bytes for the heap between the program end ($"
                          << std::hex << std::uppercase << end << ") and $" << heapEnd << std::dec << std::endl;
            }
        }
    }

    // Prepend PRG load address header and optional BASIC SYS stub
    if (isPrg) {
        std::vector<uint8_t> prgData;

        if (isBasic) {
            // MEGA65 BASIC starts at $2001 (C64 uses $0801)
            uint16_t basicBase = 0x2001;

            // 2-byte PRG load address for BASIC
            prgData.push_back((uint8_t)(basicBase & 0xFF));
            prgData.push_back((uint8_t)(basicBase >> 8));

            // BASIC stub: 10 SYS <textBase>
            std::string addrStr = std::to_string(textBase);
            // No padding — real BASIC SYS tokens don't pad

            uint16_t nextLine = basicBase + 2 + 2 + 1 + (uint16_t)addrStr.length() + 1;
            prgData.push_back((uint8_t)(nextLine & 0xFF));
            prgData.push_back((uint8_t)(nextLine >> 8));
            prgData.push_back(0x0A); prgData.push_back(0x00); // line number 10
            prgData.push_back(0x9E); // SYS token
            for (char c : addrStr) {
                prgData.push_back((uint8_t)c);
            }
            prgData.push_back(0x00); // end of BASIC line
            prgData.push_back(0x00); prgData.push_back(0x00); // end of BASIC program

            // The image is linked for its addresses (the code at textBase,
            // $200D right after the stub unless -t moves it; data below it
            // with -d): pad the file up to its start so it loads there
            uint32_t stubEnd = basicBase + (uint32_t)(prgData.size() - 2);
            uint32_t imageBase = linker.imageBase();
            if (imageBase < stubEnd) {
                std::cerr << "ln45: error: " << (imageBase < textBase ? "-d $" : "-t $")
                          << std::hex << std::uppercase << imageBase
                          << " overlaps the BASIC stub at $2001-$" << (stubEnd - 1) << std::dec
                          << " (with -basic the program must start at or after $" << std::hex << stubEnd
                          << std::dec << ")" << std::endl;
                return 1;
            }
            prgData.insert(prgData.end(), imageBase - stubEnd, 0x00);
        } else {
            // Plain PRG: just 2-byte load address header (the image's
            // start), no BASIC stub
            prgData.push_back((uint8_t)(linker.imageBase() & 0xFF));
            prgData.push_back((uint8_t)(linker.imageBase() >> 8));
        }

        // Append the linked binary
        prgData.insert(prgData.end(), binary.begin(), binary.end());
        binary = std::move(prgData);
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
