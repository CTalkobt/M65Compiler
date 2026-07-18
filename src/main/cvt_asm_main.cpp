#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>
#include "AsmParser.hpp"
#include "AsmWriter.hpp"
#include "Ca45Parser.hpp"
#include "Ca45Writer.hpp"
#include "Ca65Parser.hpp"
#include "Ca65Writer.hpp"
#include "KickAssemblerParser.hpp"
#include "KickAssemblerWriter.hpp"
#include "Version.hpp"

static void registerFormats() {
    // Register ca45 parser
    ParserRegistry::getInstance().registerParser("ca45", []() -> AsmParser* {
        return new Ca45Parser();
    });

    // Register ca45 writer
    WriterRegistry::getInstance().registerWriter("ca45", []() -> AsmWriter* {
        return new Ca45Writer();
    });

    // Register KickAssembler parser
    ParserRegistry::getInstance().registerParser("kickassembler", []() -> AsmParser* {
        return new KickAssemblerParser();
    });

    // Register KickAssembler writer
    WriterRegistry::getInstance().registerWriter("kickassembler", []() -> AsmWriter* {
        return new KickAssemblerWriter();
    });

    // Register ca65 parser
    ParserRegistry::getInstance().registerParser("ca65", []() -> AsmParser* {
        return new Ca65Parser();
    });

    // Register ca65 writer
    WriterRegistry::getInstance().registerWriter("ca65", []() -> AsmWriter* {
        return new Ca65Writer();
    });
}

static void showUsage(const char* progName) {
    std::cerr << "cvt_asm - Assembler format converter\n";
    std::cerr << suiteVersionString("cvt_asm") << "\n\n";
    std::cerr << "Usage: " << progName << " [options] <input-file>\n\n";
    std::cerr << "Options:\n";
    std::cerr << "  -f <format>    Input format (default: ca45)\n";
    std::cerr << "  -t <format>    Target format (default: ca45)\n";
    std::cerr << "  -o <file>      Output file (default: stdout)\n";
    std::cerr << "  -l             List supported formats and exit\n";
    std::cerr << "  -v             Verbose output\n";
    std::cerr << "  --help         Show this help message\n";
    std::cerr << "  --version      Show version information\n";
}

static void listFormats() {
    auto parsers = ParserRegistry::getInstance().getAvailableParsers();
    auto writers = WriterRegistry::getInstance().getAvailableWriters();

    std::cout << "Supported input formats:\n";
    for (const auto& p : parsers) {
        std::cout << "  " << p << "\n";
    }
    std::cout << "\nSupported output formats:\n";
    for (const auto& w : writers) {
        std::cout << "  " << w << "\n";
    }
}

int main(int argc, char** argv) {
    registerFormats();

    std::string inputFile;
    std::string inputFormat = "ca45";
    std::string targetFormat = "ca45";
    std::string outputFile;
    bool verbose = false;

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-f" && i + 1 < argc) {
            inputFormat = argv[++i];
        } else if (arg == "-t" && i + 1 < argc) {
            targetFormat = argv[++i];
        } else if (arg == "-o" && i + 1 < argc) {
            outputFile = argv[++i];
        } else if (arg == "-l" || arg == "--list-formats") {
            listFormats();
            return 0;
        } else if (arg == "-v") {
            verbose = true;
        } else if (arg == "--help") {
            showUsage(argv[0]);
            return 0;
        } else if (arg == "--version") {
            std::cout << suiteVersionString("cvt_asm") << "\n";
            return 0;
        } else if (arg[0] != '-') {
            inputFile = arg;
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            showUsage(argv[0]);
            return 1;
        }
    }

    if (inputFile.empty()) {
        std::cerr << "Error: No input file specified\n";
        showUsage(argv[0]);
        return 1;
    }

    // Read input file
    std::ifstream inFile(inputFile);
    if (!inFile.is_open()) {
        std::cerr << "Error: Cannot open input file '" << inputFile << "'\n";
        return 1;
    }

    std::stringstream buffer;
    buffer << inFile.rdbuf();
    std::string source = buffer.str();
    inFile.close();

    if (verbose) {
        std::cerr << "Input format: " << inputFormat << "\n";
        std::cerr << "Target format: " << targetFormat << "\n";
        std::cerr << "Input file: " << inputFile << "\n";
        if (!outputFile.empty()) {
            std::cerr << "Output file: " << outputFile << "\n";
        }
    }

    // Create parser
    auto parser = ParserRegistry::getInstance().createParser(inputFormat);
    if (!parser) {
        std::cerr << "Error: Unsupported input format '" << inputFormat << "'\n";
        return 1;
    }

    // Parse
    if (verbose) {
        std::cerr << "Parsing...\n";
    }

    auto module = parser->parse(source);

    if (parser->hasErrors()) {
        for (const auto& err : parser->getErrors()) {
            std::cerr << "Parse error: " << err << "\n";
        }
        return 1;
    }

    if (verbose) {
        if (!parser->getWarnings().empty()) {
            std::cerr << "Parse warnings:\n";
            for (const auto& warn : parser->getWarnings()) {
                std::cerr << "  " << warn << "\n";
            }
        }
        std::cerr << "Parsed " << module.statements.size() << " statements\n";
    }

    // Create writer
    auto writer = WriterRegistry::getInstance().createWriter(targetFormat);
    if (!writer) {
        std::cerr << "Error: Unsupported target format '" << targetFormat << "'\n";
        return 1;
    }

    // Write
    if (verbose) {
        std::cerr << "Writing to " << targetFormat << "...\n";
    }

    std::string output = writer->write(module);

    if (writer->hasErrors()) {
        for (const auto& err : writer->getErrors()) {
            std::cerr << "Write error: " << err << "\n";
        }
        return 1;
    }

    if (verbose) {
        if (!writer->getWarnings().empty()) {
            std::cerr << "Write warnings:\n";
            for (const auto& warn : writer->getWarnings()) {
                std::cerr << "  " << warn << "\n";
            }
        }
    }

    // Output result
    if (outputFile.empty()) {
        std::cout << output;
    } else {
        std::ofstream outFile(outputFile);
        if (!outFile.is_open()) {
            std::cerr << "Error: Cannot open output file '" << outputFile << "'\n";
            return 1;
        }
        outFile << output;
        outFile.close();

        if (verbose) {
            std::cerr << "Output written to '" << outputFile << "'\n";
        }
    }

    return 0;
}
