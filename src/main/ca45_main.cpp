#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "AssemblerLexer.hpp"
#include "AssemblerParser.hpp"
#include "Preprocessor.hpp"
#include "AssemblerGenerator.hpp"
#include "M65Emitter.hpp"
#include "O45Emitter.hpp"
#include "Version.hpp"
#include "Diagnostic.hpp"

static void printHelpGeneral() {
    std::cout << "ca45 — 45GS02 Assembler for MEGA65\n\n";
    std::cout << "Usage: ca45 [options] <input_file.s>\n\n";
    std::cout << "Options by category:\n";
    std::cout << "  Input/Output:    -c, -o, -L, -l\n";
    std::cout << "  Optimization:    -O, -P, --experimental\n";
    std::cout << "  Debugging:       -v, -vv, -Roptimizer, -Rmachstate\n";
    std::cout << "  Preprocessor:    -D, -I\n";
    std::cout << "  Diagnostics:     -Woverflow, -Wunderflow\n";
    std::cout << "  General:         -?, -V\n\n";
    std::cout << "Use --help=<section> for detailed help. Example: ca45 --help=input-output\n";
    std::cout << "Available sections: input-output, optimization, debugging, preprocessor, diagnostics\n";
}

static void printHelpInputOutput() {
    std::cout << "Input/Output Options:\n\n";
    std::cout << "  -c                 Produce relocatable .o45 object file instead of binary\n";
    std::cout << "                     Default: produces flat binary (.bin)\n\n";
    std::cout << "  -o <filename>      Specify output filename\n";
    std::cout << "                     Default: out.bin (or out.o45 with -c)\n";
    std::cout << "                     If filename ends in .prg, a 2-byte load address header is added\n";
    std::cout << "                     Example: ca45 -o program.prg input.s45\n\n";
    std::cout << "  -L <filename>      Generate assembly listing file\n";
    std::cout << "                     Useful for inspecting generated code and symbol table\n";
    std::cout << "                     Example: ca45 -L listing.txt -l2 input.s45\n\n";
    std::cout << "  -l <level>         Listing level (requires -L)\n";
    std::cout << "                     1 = Binary (default): hex dump with addresses\n";
    std::cout << "                     2 = Expanded Assembly: full expanded instructions\n\n";
    std::cout << "  --dry-run          Validate assembly without generating output\n";
    std::cout << "                     Useful for checking syntax and finding errors\n";
    std::cout << "                     Reports symbol count, instruction count, and size estimates\n";
    std::cout << "                     Example: ca45 --dry-run input.s45\n\n";
    std::cout << "  --source-map <file> Generate source-to-address mapping file\n";
    std::cout << "                     Maps binary addresses back to source lines for debugging\n";
    std::cout << "                     Shows address, source location, and original source code\n";
    std::cout << "                     Example: ca45 --source-map map.txt input.s45\n\n";
    std::cout << "  --emulator          Launch emulator with assembled output\n";
    std::cout << "                     Automatically detects available emulator (VICE, etc.)\n";
    std::cout << "                     Output file must be .prg format for emulator loading\n";
    std::cout << "                     Example: ca45 --emulator -o program.prg input.s45\n\n";
    std::cout << "  --emulator-path <path>\n";
    std::cout << "                     Path to emulator executable (overrides auto-detection)\n";
    std::cout << "                     Example: ca45 --emulator --emulator-path /usr/bin/x64 input.s45\n";
}

static void printHelpOptimization() {
    std::cout << "Optimization Options:\n\n";
    std::cout << "  -O<level>          Optimization level (default: -O2)\n";
    std::cout << "                     0 = none (no optimization)\n";
    std::cout << "                     1 = basic (redundant load/store elimination)\n";
    std::cout << "                     2 = default (add branch optimization, tail calls)\n";
    std::cout << "                     3 = aggressive (experimental optimizations)\n";
    std::cout << "                     Example: ca45 -O3 input.s45\n\n";
    std::cout << "  -P<OptName>        Enable individual optimization (after -O level)\n";
    std::cout << "  -PNo<OptName>      Disable individual optimization\n";
    std::cout << "                     Options: RedundantLoad, DeadStore, TailCall, JmpBra,\n";
    std::cout << "                     CmpElimination, BranchInvert, JSRRelocate, etc.\n";
    std::cout << "                     Example: ca45 -O2 -PNoRedundantLoad input.s45\n\n";
    std::cout << "  --experimental     Enable experimental optimizations\n";
    std::cout << "                     WARNING: HIGHLY UNSTABLE, likely to break code\n";
}

static void printHelpDebugging() {
    std::cout << "Debugging Options:\n\n";
    std::cout << "  -v                 Enable verbose output (show assembly phases)\n";
    std::cout << "                     Displays preprocessing, lexing, parsing steps\n\n";
    std::cout << "  -vv                Extra verbose output (token dumps)\n";
    std::cout << "                     Shows every token during lexing phase\n\n";
    std::cout << "  -Roptimizer        Report optimizer actions to stderr\n";
    std::cout << "                     Shows which optimizations are applied\n";
    std::cout << "                     Example: ca45 -Roptimizer input.s45 2>&1 | grep -i elim\n\n";
    std::cout << "  -Rmachstate        Trace MachineState register/flag tracking\n";
    std::cout << "                     Low-level debugging of optimizer state\n";
}

static void printHelpPreprocessor() {
    std::cout << "Preprocessor Options:\n\n";
    std::cout << "  -D<name>=<value>   Define a symbol (visible to assembler)\n";
    std::cout << "                     Example: ca45 -DMY_CONST=42 input.s45\n";
    std::cout << "                     Hex values: -DADDR=$2000, binary: -DMASK=%11110000\n\n";
    std::cout << "  -D<name>           Define a symbol with value 1\n";
    std::cout << "                     Example: ca45 -DDEBUG_MODE input.s45\n\n";
    std::cout << "  -I<path>           Add include search path\n";
    std::cout << "                     Searched in order for .include directives\n";
    std::cout << "                     Example: ca45 -Ilib/ -Iinclude/ input.s45\n";
    std::cout << "                     Can be used multiple times\n\n";
    std::cout << "  CC45_INCLUDE       Environment variable for include paths (colon-separated)\n";
    std::cout << "                     Example: CC45_INCLUDE=lib/:include/ ca45 input.s45\n";
}

static void printHelpDiagnostics() {
    std::cout << "Diagnostic Warnings:\n\n";
    std::cout << "  -Woverflow         Warn when immediate or address values overflow\n";
    std::cout << "                     Default: silent (values silently truncate)\n";
    std::cout << "                     Shows: original value and truncated result\n";
    std::cout << "                     Example: ca45 -Woverflow input.s45\n\n";
    std::cout << "  -Wunderflow        Warn when negative values are used in addresses\n";
    std::cout << "                     Default: silent\n";
    std::cout << "                     Example: ca45 -Wunderflow input.s45\n";
}

static bool launchEmulatorWithBinary(const std::string& binaryPath, const std::string& customEmulatorPath) {
    // Phase 3.3: Launch emulator with assembled binary
    std::string emuPath = customEmulatorPath;

    // If no custom path, try common emulator locations
    if (emuPath.empty()) {
        const char* candidates[] = {
            "x64",           // VICE C64 emulator
            "x64sc",         // VICE C64 (SuperCPU mode)
            "mega65",        // MEGA65 emulator
            "mega65_emu",    // Alternative MEGA65 name
            "/usr/bin/x64",  // Linux typical path
            "/usr/local/bin/x64"
        };
        for (const char* candidate : candidates) {
            // Simple existence check - would need to use stat() for robust check
            std::string cmd = std::string("which ") + candidate + " >/dev/null 2>&1";
            if (system(cmd.c_str()) == 0) {
                emuPath = candidate;
                break;
            }
        }
    }

    if (emuPath.empty()) {
        std::cerr << "Warning: No emulator found. Set path with --emulator-path <path>" << std::endl;
        return false;
    }

    // Build command to launch emulator with binary
    std::string cmd = emuPath + " \"" + binaryPath + "\" 2>/dev/null &";
    int result = system(cmd.c_str());

    if (result == 0) {
        std::cout << "Launching emulator: " << emuPath << std::endl;
        return true;
    }
    return false;
}

static void writeSourceMap(const std::string& filename, const AssemblerParser& parser, const std::string& source) {
    std::ofstream out(filename);
    if (!out.is_open()) return;

    std::vector<std::string> sourceLines;
    std::stringstream ss(source);
    std::string line;
    while (std::getline(ss, line)) sourceLines.push_back(line);

    out << "Source Map — Binary Address to Source Line Mapping\n";
    out << "=====================================================\n\n";
    out << std::hex;
    out << std::setfill('0');

    for (const auto& stmt : parser.statements) {
        if (stmt->deleted || stmt->bytes.empty()) continue;

        // Address
        out << std::setw(8) << stmt->address << "  ";

        // Line number
        if (stmt->line > 0) {
            out << std::dec << "Line " << stmt->line << "  " << std::hex;
        } else {
            out << "(no line info)  ";
        }

        // Source line text
        if (stmt->line > 0 && stmt->line <= (int)sourceLines.size()) {
            std::string srcLine = sourceLines[stmt->line - 1];
            // Trim leading whitespace for display
            size_t start = srcLine.find_first_not_of(" \t");
            if (start != std::string::npos) {
                srcLine = srcLine.substr(start);
            }
            // Limit line length for readability
            if (srcLine.length() > 60) {
                srcLine = srcLine.substr(0, 57) + "...";
            }
            out << srcLine;
        }
        out << "\n";
    }

    out << "\n\nSymbol Table\n";
    out << "============\n";
    std::map<std::string, Symbol> sortedSymbols = parser.getSymbolTable();
    for (const auto& [name, sym] : sortedSymbols) {
        out << std::setw(32) << std::left << std::setfill(' ') << name << " ";
        out << std::hex << std::setw(8) << std::right << std::setfill('0') << sym.value;
        if (sym.isConstant) out << " (CONST)";
        else if (sym.isVariable) out << " (VAR)";
        else if (sym.isAddress) out << " (ADDR)";
        out << "\n";
    }
}

static void writeListing(const std::string& filename, const AssemblerParser& parser, const std::string& source) {
    std::ofstream out(filename);
    if (!out.is_open()) return;

    std::vector<std::string> sourceLines;
    std::stringstream ss(source);
    std::string line;
    while (std::getline(ss, line)) sourceLines.push_back(line);

    out << "ca45 Assembly Listing\n";
    out << "====================\n\n";

    for (const auto& stmt : parser.statements) {
        if (stmt->deleted) continue;
        
        // Address
        if (stmt->type == AssemblerParser::Statement::DIRECTIVE && (stmt->dir.name == "segment" || stmt->dir.name == "code" || stmt->dir.name == "data" || stmt->dir.name == "bss")) {
            out << "\nSegment: " << stmt->segmentName << "\n";
        }

        out << std::hex << std::setw(8) << std::setfill('0') << stmt->address << ": ";

        // Bytes (up to 4 bytes per line to match common listing formats)
        size_t byteCount = stmt->bytes.size();
        for (size_t i = 0; i < 4; ++i) {
            if (i < byteCount) {
                out << std::hex << std::setw(2) << std::setfill('0') << (int)stmt->bytes[i] << " ";
            } else {
                out << "   ";
            }
        }

        // Source line
        if (stmt->line > 0 && stmt->line <= (int)sourceLines.size()) {
            out << "  " << sourceLines[stmt->line - 1];
        }
        out << "\n";

        // Extra bytes
        if (byteCount > 4) {
            for (size_t i = 4; i < byteCount; i += 4) {
                out << "          "; // indentation (8 hex chars + ": ")
                for (size_t j = 0; j < 4; ++j) {
                    if (i + j < byteCount) {
                        out << std::hex << std::setw(2) << std::setfill('0') << (int)stmt->bytes[i + j] << " ";
                    } else {
                        out << "   ";
                    }
                }
                out << "\n";
            }
        }
    }

    out << "\nSymbol Table\n";
    out << "============\n";
    // Sort symbols by name for consistent output
    std::map<std::string, Symbol> sortedSymbols = parser.getSymbolTable();
    for (const auto& [name, sym] : sortedSymbols) {
        out << std::setw(32) << std::left << std::setfill(' ') << name << " ";
        out << std::hex << std::setw(8) << std::right << std::setfill('0') << sym.value;
        if (sym.isConstant) out << " (CONST)";
        else if (sym.isVariable) out << " (VAR)";
        else if (sym.isAddress) out << " (ADDR)";
        out << "\n";
    }
}

int main(int argc, char** argv) {
    std::string input_file;
    std::string output_file;
    std::string listing_file;
    bool outputSet = false;
    bool relocMode = false;
    bool verboseOptimizer = false;
    bool traceMachState = false;
    bool enableExperimental = false;
    bool warnOverflow = false;   // Phase 1.3: warn on value overflows (default: off)
    bool warnUnderflow = false;  // Phase 1.3: warn on negative values (default: off)
    bool dryRun = false;         // Phase 3.1: validate without generating output
    std::string sourceMapFile;   // Phase 3.2: source map output file
    bool launchEmulator = false; // Phase 3.3: launch emulator after assembly
    std::string emulatorPath;    // Phase 3.3: path to emulator executable
    int optimizationLevel = 2;  // Default to -O2
    OptimizationFlags optFlags = OptimizationFlags::fromLevel(2);  // Default to -O2
    int verboseLevel = 0;
    int listingLevel = 1;
    std::map<std::string, uint32_t> predefinedSymbols;
    std::map<std::string, std::string> initialSymbols;
    std::vector<std::string> includePaths;

    // Add paths from CC45_INCLUDE environment variable
    if (const char* envInc = std::getenv("CC45_INCLUDE")) {
        std::string s(envInc);
        size_t pos = 0, found;
        while ((found = s.find(':', pos)) != std::string::npos) {
            if (found > pos) includePaths.push_back(s.substr(pos, found - pos));
            pos = found + 1;
        }
        if (pos < s.size()) includePaths.push_back(s.substr(pos));
    }

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-V" || arg == "--version") {
            std::cout << suiteVersionString("ca45") << std::endl;
            return 0;
        } else if (arg == "-?" || arg == "--help") {
            printHelpGeneral();
            return 0;
        } else if (arg.substr(0, 7) == "--help=") {
            std::string section = arg.substr(7);
            if (section == "input-output" || section == "io") {
                printHelpInputOutput();
            } else if (section == "optimization" || section == "opt") {
                printHelpOptimization();
            } else if (section == "debugging" || section == "debug") {
                printHelpDebugging();
            } else if (section == "preprocessor" || section == "prep") {
                printHelpPreprocessor();
            } else if (section == "diagnostics" || section == "diag" || section == "warnings") {
                printHelpDiagnostics();
            } else {
                std::cerr << "Unknown help section: " << section << std::endl;
                std::cerr << "Available sections: input-output, optimization, debugging, preprocessor, diagnostics" << std::endl;
                return 1;
            }
            return 0;
        } else if (arg == "-c") {
            relocMode = true;
        } else if (arg == "-o" && i + 1 < argc) {
            output_file = argv[++i];
            outputSet = true;
        } else if (arg == "-L" && i + 1 < argc) {
            listing_file = argv[++i];
        } else if (arg == "-l" && i + 1 < argc) {
            listingLevel = std::stoi(argv[++i]);
        } else if (arg == "-Roptimizer") {
            verboseOptimizer = true;
        } else if (arg == "-Rmachstate") {
            traceMachState = true;
        } else if (arg == "--experimental") {
            enableExperimental = true;
        } else if (arg.substr(0, 2) == "-O") {
            std::string levelStr = arg.substr(2);
            if (!levelStr.empty() && levelStr[0] >= '0' && levelStr[0] <= '3') {
                optimizationLevel = levelStr[0] - '0';
            } else {
                optimizationLevel = 2;  // -O defaults to O2
            }
            optFlags = OptimizationFlags::fromLevel(optimizationLevel);
        } else if (arg.substr(0, 2) == "-P") {
            // Named optimization flags: -P<Name> to enable, -PNo<Name> to disable
            std::string flagName = arg.substr(2);
            bool enable = true;
            if (flagName.substr(0, 2) == "No") {
                enable = false;
                flagName = flagName.substr(2);
            }
            // IR-level optimizations
            if (flagName == "StrengthReduction") optFlags.strengthReduction = enable;
            else if (flagName == "AlgebraicSimplify") optFlags.algebraicSimplify = enable;
            else if (flagName == "TypeNarrowing") optFlags.typeNarrowing = enable;
            else if (flagName == "BranchFold") optFlags.branchFold = enable;
            else if (flagName == "CSE") optFlags.cse = enable;
            else if (flagName == "LICM") optFlags.licm = enable;
            else if (flagName == "CopyChains") optFlags.copyChains = enable;
            else if (flagName == "AddrElemFusion") optFlags.addrElemFusion = enable;
            // Assembler-level optimizations
            else if (flagName == "JSRRelocate") optFlags.jsrRelocate = enable;
            else if (flagName == "TailCall") optFlags.tailCall = enable;
            else if (flagName == "BranchInvert") optFlags.branchInvert = enable;
            else if (flagName == "JmpBra") optFlags.jmpBra = enable;
            else if (flagName == "NoOpBra") optFlags.noOpBra = enable;
            else if (flagName == "CmpElimination") optFlags.cmpElimination = enable;
            else if (flagName == "RedundantLoad") optFlags.redundantLoad = enable;
            else if (flagName == "DeadStore") optFlags.deadStore = enable;
            else if (flagName == "TailDedup") optFlags.tailDedup = enable;
            else if (flagName == "PreserveXSP") optFlags.preserveXSP = enable;
            else if (flagName == "SeqExtract") optFlags.seqExtract = enable;
            else if (flagName == "StoreLoadPair") optFlags.storeLoadPair = enable;
            else if (flagName == "FCmpOpt") optFlags.fcmpOpt = enable;
            else if (flagName == "TSXRedundant") optFlags.tsxRedundant = enable;
        } else if (arg == "-Woverflow" || arg == "--warn-overflow") {
            warnOverflow = true;  // Phase 1.3: enable overflow warnings
        } else if (arg == "-Wunderflow" || arg == "--warn-underflow") {
            warnUnderflow = true;  // Phase 1.3: enable underflow warnings
        } else if (arg == "--dry-run") {
            dryRun = true;  // Phase 3.1: validate without generating output
        } else if (arg == "--source-map" && i + 1 < argc) {
            sourceMapFile = argv[++i];  // Phase 3.2: generate source map
        } else if (arg == "--emulator") {
            launchEmulator = true;  // Phase 3.3: launch emulator after assembly
        } else if (arg == "--emulator-path" && i + 1 < argc) {
            emulatorPath = argv[++i];  // Phase 3.3: custom emulator path
        } else if (arg == "-vv") {
            verboseLevel = 2;
        } else if (arg == "-v") {
            verboseLevel = 1;
        } else if (arg.substr(0, 2) == "-I") {
            includePaths.push_back(arg.substr(2));
        } else if (arg.substr(0, 2) == "-D") {
            std::string define = arg.substr(2);
            size_t eq = define.find('=');
            if (eq != std::string::npos) {
                std::string name = define.substr(0, eq);
                std::string valStr = define.substr(eq + 1);
                initialSymbols[name] = valStr;
                uint32_t val = 0;
                if (valStr.substr(0, 1) == "$") val = std::stoul(valStr.substr(1), nullptr, 16);
                else if (valStr.substr(0, 1) == "%") val = std::stoul(valStr.substr(1), nullptr, 2);
                else val = std::stoul(valStr);
                predefinedSymbols[name] = val;
            } else {
                initialSymbols[define] = "1";
                predefinedSymbols[define] = 1;
            }
        } else {
            input_file = arg;
        }
    }

    if (!outputSet) {
        output_file = relocMode ? "out.o45" : "out.bin";
    }

    if (predefinedSymbols.find("cc45.zeroPageStart") == predefinedSymbols.end()) {
        predefinedSymbols["cc45.zeroPageStart"] = 0x02;
    }
    if (predefinedSymbols.find("__sp_base") == predefinedSymbols.end()) {
        predefinedSymbols["__sp_base"] = 0x0101;
    }

    if (input_file.empty()) {
        std::cerr << "Usage: ca45 [options] <input_file.s>" << std::endl;
        std::cerr << "Use -? for a list of options." << std::endl;
        return 1;
    }

    std::ifstream file(input_file);
    if (!file.is_open()) {
        std::cerr << "Failed to open input file: " << input_file << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string sourceRaw = buffer.str();

    if (verboseLevel >= 1) {
        std::cout << "Preprocessing " << input_file << "..." << std::endl;
    }

    Preprocessor preprocessor(false);
    std::string source;
    try {
        source = preprocessor.process(sourceRaw, initialSymbols, includePaths, input_file);
    } catch (const std::exception& e) {
        std::cerr << formatDiagnostic(input_file, 1, 1, Severity::Error, e.what()) << std::endl;
        return 1;
    }

    if (verboseLevel >= 1) {
        std::cout << "Lexing " << input_file << "..." << std::endl;
    }

    AssemblerLexer lexer(source);
    std::vector<AssemblerToken> tokens = lexer.tokenize();

    if (verboseLevel >= 2) {
        for (const auto& token : tokens) {
            std::cout << "Token: " << token.typeToString() << " (" << token.value << ") at " << token.line << ":" << token.column << std::endl;
        }
    }

    AssemblerParser parser(tokens, predefinedSymbols);
    parser.setSourceFile(input_file);
    parser.verboseOptimizer = verboseOptimizer;
    parser.traceMachState = traceMachState;
    parser.enableExperimental = enableExperimental;
    parser.warnOverflow = warnOverflow;    // Phase 1.3
    parser.warnUnderflow = warnUnderflow;  // Phase 1.3
    parser.optimizationLevel = optimizationLevel;
    parser.optFlags = optFlags;
    try {
        parser.pass1();

        if (parser.hasErrors()) {
            for (const auto& err : parser.getErrors()) {
                std::cerr << err << std::endl;
            }
            return 1;
        }

        // Phase 3.1: Dry-run mode - validate without generating output
        if (dryRun) {
            parser.pass2(false);  // Run optimizer and resolve addresses
            auto symbols = parser.getSymbolTable();
            int totalInstructions = 0;
            uint32_t totalBytes = 0;
            for (const auto& stmt : parser.statements) {
                if (!stmt->deleted) {
                    if (stmt->bytes.size() > 0) totalInstructions++;
                    totalBytes += stmt->bytes.size();
                }
            }
            std::cout << "Validation successful (dry-run mode)\n";
            std::cout << "  Symbols: " << symbols.size() << "\n";
            std::cout << "  Instructions: " << totalInstructions << "\n";
            std::cout << "  Total size: " << totalBytes << " bytes\n";
            return 0;
        }

        if (relocMode) {
            // Relocatable object mode: produce .o45 file
            parser.pass2(false);
            auto o45 = emitO45(parser);
            if (!o45.empty()) {
                std::ofstream out(output_file, std::ios::binary);
                out.write(reinterpret_cast<const char*>(o45.data()), o45.size());
                std::cout << "Object file: " << output_file << " (" << o45.size() << " bytes)" << std::endl;
                if (!listing_file.empty()) {
                    writeListing(listing_file, parser, source);
                    std::cout << "Listing generated to " << listing_file << std::endl;
                }
                // Phase 3.2: Generate source map if requested
                if (!sourceMapFile.empty()) {
                    writeSourceMap(sourceMapFile, parser, source);
                    std::cout << "Source map generated to " << sourceMapFile << std::endl;
                }
            }
        } else if (listingLevel == 2) {
            parser.pass2(false); // Run optimizer and resolve addresses
            std::ofstream out(output_file);
            M65Emitter e(out, predefinedSymbols["cc45.zeroPageStart"]);
            AssemblerGenerator::generate(&parser, e);
            std::cout << "Expanded listing generated to " << output_file << std::endl;
            // Phase 3.2: Generate source map if requested
            if (!sourceMapFile.empty()) {
                writeSourceMap(sourceMapFile, parser, source);
                std::cout << "Source map generated to " << sourceMapFile << std::endl;
            }
        } else {
            bool isPrg = false;
            if (output_file.length() >= 4 && output_file.substr(output_file.length() - 4) == ".prg") {
                isPrg = true;
            }
            auto binary = parser.pass2(isPrg);
            if (!binary.empty()) {
                std::ofstream out(output_file, std::ios::binary);
                out.write(reinterpret_cast<const char*>(binary.data()), binary.size());
                std::cout << "Assembled to " << output_file << " (" << binary.size() << " bytes)" << std::endl;
                if (!listing_file.empty()) {
                    writeListing(listing_file, parser, source);
                    std::cout << "Listing generated to " << listing_file << std::endl;
                }
                // Phase 3.2: Generate source map if requested
                if (!sourceMapFile.empty()) {
                    writeSourceMap(sourceMapFile, parser, source);
                    std::cout << "Source map generated to " << sourceMapFile << std::endl;
                }
                // Phase 3.3: Launch emulator if requested
                if (launchEmulator && isPrg) {
                    launchEmulatorWithBinary(output_file, emulatorPath);
                } else if (launchEmulator && !isPrg) {
                    std::cerr << "Warning: Emulator launch requires .prg output (use -o program.prg)" << std::endl;
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << input_file << ": " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
