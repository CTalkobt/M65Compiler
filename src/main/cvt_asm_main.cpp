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
#include "AcmeParser.hpp"
#include "AcmeWriter.hpp"
#include "OscarParser.hpp"
#include "OscarWriter.hpp"
#include "Merlin64Parser.hpp"
#include "Merlin64Writer.hpp"
#include "X65Parser.hpp"
#include "X65Writer.hpp"
#include "KickAssemblerParser.hpp"
#include "KickAssemblerWriter.hpp"
#include "FormatDetection.hpp"
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

    // Register ACME parser
    ParserRegistry::getInstance().registerParser("acme", []() -> AsmParser* {
        return new AcmeParser();
    });

    // Register ACME writer
    WriterRegistry::getInstance().registerWriter("acme", []() -> AsmWriter* {
        return new AcmeWriter();
    });

    // Register Oscar parser
    ParserRegistry::getInstance().registerParser("oscar", []() -> AsmParser* {
        return new OscarParser();
    });

    // Register Oscar writer
    WriterRegistry::getInstance().registerWriter("oscar", []() -> AsmWriter* {
        return new OscarWriter();
    });

    // Register Merlin 64 parser
    ParserRegistry::getInstance().registerParser("merlin64", []() -> AsmParser* {
        return new Merlin64Parser();
    });

    // Register Merlin 64 writer
    WriterRegistry::getInstance().registerWriter("merlin64", []() -> AsmWriter* {
        return new Merlin64Writer();
    });

    // Register x65 parser
    ParserRegistry::getInstance().registerParser("x65", []() -> AsmParser* {
        return new X65Parser();
    });

    // Register x65 writer
    WriterRegistry::getInstance().registerWriter("x65", []() -> AsmWriter* {
        return new X65Writer();
    });
}

static void showUsage(const char* progName) {
    std::cerr << "cvt_asm - Assembler format converter\n";
    std::cerr << suiteVersionString("cvt_asm") << "\n\n";
    std::cerr << "Usage: " << progName << " [options] <input-file> [<input-file> ...]\n\n";
    std::cerr << "Options:\n";
    std::cerr << "  -f <format>    Input format (default: auto-detect from extension/modeline)\n";
    std::cerr << "  -t <format>    Target format (default: ca45)\n";
    std::cerr << "  -o <file>      Output file or directory (for multiple files, treated as directory)\n";
    std::cerr << "  -l             List supported formats and exit\n";
    std::cerr << "  -v             Verbose output\n";
    std::cerr << "  --help         Show this help message\n";
    std::cerr << "  --version      Show version information\n";
    std::cerr << "\nBatch conversion:\n";
    std::cerr << "  " << progName << " -f ca65 -t acme *.s65        Convert all .s65 files to .asm\n";
    std::cerr << "  " << progName << " -t oscar -o output/ *.asm   Convert files to oscar format in output dir\n";
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

    std::vector<std::string> inputFiles;
    std::string inputFormat = "";  // Empty means auto-detect
    std::string targetFormat = "ca45";
    std::string outputDir;
    bool verbose = false;

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-f" && i + 1 < argc) {
            inputFormat = argv[++i];
        } else if (arg == "-t" && i + 1 < argc) {
            targetFormat = argv[++i];
        } else if (arg == "-o" && i + 1 < argc) {
            outputDir = argv[++i];
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
            inputFiles.push_back(arg);
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            showUsage(argv[0]);
            return 1;
        }
    }

    if (inputFiles.empty()) {
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

    // Auto-detect input format if not specified
    if (inputFormat.empty()) {
        auto detected = FormatDetection::detectFormat(inputFile);
        if (detected) {
            inputFormat = *detected;
            if (verbose) {
                std::cerr << "Auto-detected input format: " << inputFormat << "\n";
            }
        } else {
            std::cerr << "Error: Could not auto-detect input format. Specify with -f <format>\n";
            std::cerr << "Supported formats: ca45, ca65, acme, oscar, merlin64, x65, kickassembler\n";
            return 1;
        }
    }

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
