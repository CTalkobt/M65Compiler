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
            std::cout << "Usage: ca45 [options] <input_file.s>" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  -c             Produce relocatable .o45 object file instead of binary" << std::endl;
            std::cout << "  -o <filename>  Specify output filename (default: out.bin, or out.o45 with -c)" << std::endl;
            std::cout << "                 If filename ends in .prg, a 2-byte load address header is added." << std::endl;
            std::cout << "  -L <filename>  Generate assembly listing file" << std::endl;
            std::cout << "  -l <level>     Listing level: 1=Binary (default), 2=Expanded Assembly" << std::endl;
            std::cout << "  -v             Enable verbose output (phase info)" << std::endl;
            std::cout << "  -vv            Extra verbose output (token dumps)" << std::endl;
            std::cout << "  -Dname=val     Define a symbol (e.g., -Dcc45.zeroPageStart=$10)" << std::endl;
            std::cout << "  -I<path>       Add include search path" << std::endl;
            std::cout << "  -Roptimizer    Report optimizer actions to stderr" << std::endl;
            std::cout << "  -Rmachstate    Trace MachineState register/flag tracking to stderr" << std::endl;
            std::cout << "  --experimental Enable experimental optimizations (HIGHLY UNSTABLE, likely to break code)" << std::endl;
            std::cout << "  -?             Display this help message" << std::endl;
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
    try {
        parser.pass1();

        if (parser.hasErrors()) {
            for (const auto& err : parser.getErrors()) {
                std::cerr << err << std::endl;
            }
            return 1;
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
            }
        } else if (listingLevel == 2) {
            parser.pass2(false); // Run optimizer and resolve addresses
            std::ofstream out(output_file);
            M65Emitter e(out, predefinedSymbols["cc45.zeroPageStart"]);
            AssemblerGenerator::generate(&parser, e);
            std::cout << "Expanded listing generated to " << output_file << std::endl;
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
            }
        }
    } catch (const std::exception& e) {
        std::cerr << input_file << ": " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
