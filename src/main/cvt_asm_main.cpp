#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>
#include <filesystem>
#include <algorithm>
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

static std::string getFormatExtension(const std::string& format) {
    std::string fmt = format;
    std::transform(fmt.begin(), fmt.end(), fmt.begin(), ::tolower);

    if (fmt == "ca45") return ".s45";
    if (fmt == "ca65") return ".s65";
    if (fmt == "acme") return ".a65";
    if (fmt == "oscar") return ".s6502";
    if (fmt == "merlin64") return ".m64";
    if (fmt == "x65") return ".x65";
    if (fmt == "kickassembler") return ".ka";
    return ".asm";
}

static std::string generateOutputPath(const std::string& inputFile,
                                      const std::string& targetFormat,
                                      const std::string& outputDir) {
    namespace fs = std::filesystem;

    fs::path inPath(inputFile);
    std::string baseName = inPath.stem().string();
    std::string ext = getFormatExtension(targetFormat);

    if (outputDir.empty()) {
        return baseName + ext;
    } else {
        return (fs::path(outputDir) / (baseName + ext)).string();
    }
}

static bool convertFile(const std::string& inputFile,
                       const std::string& inputFormat,
                       const std::string& targetFormat,
                       const std::string& outputFile,
                       bool verbose) {
    // Read input file
    std::ifstream inFile(inputFile);
    if (!inFile.is_open()) {
        std::cerr << "Error: Cannot open input file '" << inputFile << "'\n";
        return false;
    }

    std::stringstream buffer;
    buffer << inFile.rdbuf();
    std::string source = buffer.str();
    inFile.close();

    // Auto-detect input format if not specified
    std::string actualFormat = inputFormat;
    if (actualFormat.empty()) {
        auto detected = FormatDetection::detectFormat(inputFile);
        if (detected) {
            actualFormat = *detected;
            if (verbose) {
                std::cerr << "Auto-detected input format: " << actualFormat << "\n";
            }
        } else {
            std::cerr << "Error: Could not auto-detect input format for '" << inputFile << "'. Specify with -f <format>\n";
            return false;
        }
    }

    if (verbose) {
        std::cerr << "Processing: " << inputFile << "\n";
        std::cerr << "  Input format: " << actualFormat << "\n";
        std::cerr << "  Target format: " << targetFormat << "\n";
    }

    // Create parser
    auto parser = ParserRegistry::getInstance().createParser(actualFormat);
    if (!parser) {
        std::cerr << "Error: Unsupported input format '" << actualFormat << "'\n";
        return false;
    }

    // Parse
    if (verbose) {
        std::cerr << "  Parsing...\n";
    }

    auto module = parser->parse(source);

    if (parser->hasErrors()) {
        for (const auto& err : parser->getErrors()) {
            std::cerr << "  Parse error: " << err << "\n";
        }
        return false;
    }

    if (verbose) {
        if (!parser->getWarnings().empty()) {
            std::cerr << "  Parse warnings:\n";
            for (const auto& warn : parser->getWarnings()) {
                std::cerr << "    " << warn << "\n";
            }
        }
        std::cerr << "  Parsed " << module.statements.size() << " statements\n";
    }

    // Create writer
    auto writer = WriterRegistry::getInstance().createWriter(targetFormat);
    if (!writer) {
        std::cerr << "Error: Unsupported target format '" << targetFormat << "'\n";
        return false;
    }

    // Write
    if (verbose) {
        std::cerr << "  Writing to " << targetFormat << "...\n";
    }

    std::string output = writer->write(module);

    if (writer->hasErrors()) {
        for (const auto& err : writer->getErrors()) {
            std::cerr << "  Write error: " << err << "\n";
        }
        return false;
    }

    if (verbose) {
        if (!writer->getWarnings().empty()) {
            std::cerr << "  Write warnings:\n";
            for (const auto& warn : writer->getWarnings()) {
                std::cerr << "    " << warn << "\n";
            }
        }
    }

    // Output result
    if (outputFile.empty()) {
        std::cout << output;
    } else {
        // Create parent directories if needed
        namespace fs = std::filesystem;
        fs::path outPath(outputFile);
        fs::path parentDir = outPath.parent_path();

        if (!parentDir.empty() && !fs::exists(parentDir)) {
            try {
                fs::create_directories(parentDir);
            } catch (const std::exception& e) {
                std::cerr << "Error: Cannot create output directory: " << e.what() << "\n";
                return false;
            }
        }

        std::ofstream outFile(outputFile);
        if (!outFile.is_open()) {
            std::cerr << "Error: Cannot open output file '" << outputFile << "'\n";
            return false;
        }
        outFile << output;
        outFile.close();

        if (verbose) {
            std::cerr << "  Output written to '" << outputFile << "'\n";
        }
    }

    return true;
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

    if (verbose) {
        std::cerr << "Target format: " << targetFormat << "\n";
        if (!outputDir.empty()) {
            std::cerr << "Output directory: " << outputDir << "\n";
        }
    }

    // Batch conversion: process each input file
    int failureCount = 0;
    int successCount = 0;

    for (size_t i = 0; i < inputFiles.size(); ++i) {
        const std::string& inputFile = inputFiles[i];
        std::string outputFile;

        // For single file with no output dir: use stdout
        // For single file with output dir: generate filename
        // For multiple files: always generate filename
        if (inputFiles.size() == 1 && outputDir.empty()) {
            outputFile = "";  // stdout
        } else {
            outputFile = generateOutputPath(inputFile, targetFormat, outputDir);
        }

        if (verbose && inputFiles.size() > 1) {
            std::cerr << "\n[" << (i + 1) << "/" << inputFiles.size() << "] ";
        }

        if (!convertFile(inputFile, inputFormat, targetFormat, outputFile, verbose)) {
            failureCount++;
        } else {
            successCount++;
        }
    }

    if (verbose && inputFiles.size() > 1) {
        std::cerr << "\n\nBatch conversion complete: " << successCount << " succeeded, " << failureCount << " failed\n";
    }

    return (failureCount > 0) ? 1 : 0;
}
