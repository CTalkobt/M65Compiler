#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <cstring>
#include <filesystem>
#include <algorithm>
#include <iomanip>
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
#include "MacroUtils.hpp"
#include "ThreadPool.hpp"
#include <chrono>
#include <atomic>

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
    std::cerr << "  --validate     Verify round-trip conversion (A → target → A)\n";
    std::cerr << "  --stats        Show detailed conversion statistics and loss analysis\n";
    std::cerr << "  -l             List supported formats and exit\n";
    std::cerr << "  -v             Verbose output\n";
    std::cerr << "  --help         Show this help message\n";
    std::cerr << "  --version      Show version information\n";
    std::cerr << "\nBatch conversion:\n";
    std::cerr << "  " << progName << " -f ca65 -t acme *.s65        Convert all .s65 files to .asm\n";
    std::cerr << "  " << progName << " -t oscar -o output/ *.asm   Convert files to oscar format in output dir\n";
    std::cerr << "\nStatistics & validation:\n";
    std::cerr << "  " << progName << " --stats -t oscar input.s65          Show conversion stats\n";
    std::cerr << "  " << progName << " --validate -t oscar input.s65       Test round-trip preservation\n";
    std::cerr << "  " << progName << " --stats --validate -t acme *.s65   Stats + validation with verbose\n";
}

// Validation statistics tracking
struct ValidationStats {
    int totalLabels = 0;
    int preservedLabels = 0;
    int totalSymbols = 0;
    int preservedSymbols = 0;
    int totalDirectives = 0;
    int preservedDirectives = 0;
    int totalComments = 0;
    int preservedComments = 0;
    std::string originalOutput;
    std::string roundTripOutput;
    bool isLossless = true;

    double getPreservationPercent() const {
        int totalItems = totalLabels + totalSymbols + totalDirectives + totalComments;
        if (totalItems == 0) return 100.0;
        int preserved = preservedLabels + preservedSymbols + preservedDirectives + preservedComments;
        return (preserved * 100.0) / totalItems;
    }
};

// Conversion statistics tracking
struct ConversionStats {
    std::string inputFile;
    std::string inputFormat;
    std::string targetFormat;

    // Source metrics
    int srcLabels = 0;
    int srcInstructions = 0;
    int srcDirectives = 0;
    int srcComments = 0;
    int srcSymbols = 0;
    int srcMacros = 0;  // Detected via .macro/.endmacro patterns
    int srcLines = 0;

    // Target metrics
    int tgtLabels = 0;
    int tgtInstructions = 0;
    int tgtDirectives = 0;
    int tgtComments = 0;
    int tgtSymbols = 0;
    int tgtLines = 0;

    // Loss tracking
    int lostLabels = 0;
    int lostInstructions = 0;
    int lostDirectives = 0;
    int lostComments = 0;
    int lostSymbols = 0;
    int lostMacros = 0;

    // Detailed loss reasons
    std::vector<std::string> unsupportedDirectives;
    std::vector<std::string> conversionIssues;

    // Calculated metrics
    double getInstructionPreservation() const {
        if (srcInstructions == 0) return 100.0;
        return ((srcInstructions - lostInstructions) * 100.0) / srcInstructions;
    }

    double getMetadataPreservation() const {
        int total = srcLabels + srcDirectives + srcComments + srcSymbols;
        if (total == 0) return 100.0;
        int lost = lostLabels + lostDirectives + lostComments + lostSymbols;
        return ((total - lost) * 100.0) / total;
    }

    double getOverallPreservation() const {
        int total = srcLabels + srcInstructions + srcDirectives + srcComments + srcSymbols;
        if (total == 0) return 100.0;
        int lost = lostLabels + lostInstructions + lostDirectives + lostComments + lostSymbols;
        return ((total - lost) * 100.0) / total;
    }

    int getTotalLoss() const {
        return lostLabels + lostInstructions + lostDirectives + lostComments + lostSymbols + lostMacros;
    }
};

// Count labels in module
static int countLabels(const AsmIR::Module& module) {
    int count = 0;
    for (const auto& stmt : module.statements) {
        if (stmt.type == AsmIR::Statement::Type::LABEL) {
            count++;
        }
    }
    return count;
}

// Count symbols in module
static int countSymbols(const AsmIR::Module& module) {
    return module.symbols.size();
}

// Count directives in module
static int countDirectives(const AsmIR::Module& module) {
    int count = 0;
    for (const auto& stmt : module.statements) {
        if (stmt.type == AsmIR::Statement::Type::DIRECTIVE) {
            count++;
        }
    }
    return count;
}

// Count comments in module
static int countComments(const AsmIR::Module& module) {
    int count = 0;
    for (const auto& stmt : module.statements) {
        if (stmt.type == AsmIR::Statement::Type::COMMENT) {
            count++;
        }
    }
    return count;
}

// Extract labels from output text
static std::set<std::string> extractLabels(const std::string& output) {
    std::set<std::string> labels;
    std::istringstream iss(output);
    std::string line;
    while (std::getline(iss, line)) {
        // Simple heuristic: lines ending with ':' are likely labels
        if (!line.empty() && line.back() == ':') {
            std::string label = line;
            // Remove leading/trailing whitespace
            label.erase(0, label.find_first_not_of(" \t"));
            label.pop_back(); // Remove trailing ':'
            if (!label.empty()) {
                labels.insert(label);
            }
        }
    }
    return labels;
}

// Detect macros in source text
static int detectMacros(const std::string& source) {
    int count = 0;
    std::istringstream iss(source);
    std::string line;
    while (std::getline(iss, line)) {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        // Check for .macro or macro patterns
        if (line.find(".macro") == 0 || line.find("macro") == 0 ||
            line.find(".define") == 0) {
            count++;
        }
    }
    return count;
}

// Count instructions in module
static int countInstructions(const AsmIR::Module& module) {
    int count = 0;
    for (const auto& stmt : module.statements) {
        if (stmt.type == AsmIR::Statement::Type::INSTRUCTION) {
            count++;
        }
    }
    return count;
}

// Detect unsupported directives per format
static std::vector<std::string> detectUnsupportedDirectives(
    const AsmIR::Module& module,
    const std::string& targetFormat) {
    std::vector<std::string> unsupported;
    std::set<std::string> seen;

    for (const auto& stmt : module.statements) {
        if (stmt.type == AsmIR::Statement::Type::DIRECTIVE) {
            std::string dirName = stmt.dir.name;
            if (seen.count(dirName)) continue;
            seen.insert(dirName);

            // Check format-specific unsupported directives
            bool isUnsupported = false;
            std::string reason;

            if (targetFormat == "oscar") {
                // Oscar doesn't support some modern directives
                if (dirName == "cpu" || dirName == "o45") {
                    isUnsupported = true;
                    reason = "45GS02-specific directive";
                }
            } else if (targetFormat == "acme") {
                if (dirName == "o45" || dirName == "func_flags") {
                    isUnsupported = true;
                    reason = "Format-specific metadata";
                }
            } else if (targetFormat == "kickassembler") {
                if (dirName == "zp_uses" || dirName == "zp_clobbers") {
                    isUnsupported = true;
                    reason = "Stack/ZP frame tracking";
                }
            }

            if (isUnsupported) {
                unsupported.push_back(dirName + " (" + reason + ")");
            }
        }
    }

    return unsupported;
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

static bool collectConversionStats(const std::string& inputFile,
                                    const std::string& inputFormat,
                                    const std::string& targetFormat,
                                    bool verbose,
                                    ConversionStats& stats) {
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

    // Auto-detect input format
    std::string actualFormat = inputFormat;
    if (actualFormat.empty()) {
        auto detected = FormatDetection::detectFormat(inputFile);
        if (detected) {
            actualFormat = *detected;
        } else {
            std::cerr << "Error: Could not auto-detect input format\n";
            return false;
        }
    }

    stats.inputFile = inputFile;
    stats.inputFormat = actualFormat;
    stats.targetFormat = targetFormat;

    // Count source metrics
    stats.srcLines = std::count(source.begin(), source.end(), '\n') + 1;
    stats.srcComments = std::count(source.begin(), source.end(), ';');
    stats.srcMacros = detectMacros(source);

    // Parse source
    auto srcParser = ParserRegistry::getInstance().createParser(actualFormat);
    if (!srcParser) {
        std::cerr << "Error: Unsupported input format\n";
        return false;
    }

    auto srcModule = srcParser->parse(source);
    if (srcParser->hasErrors()) {
        std::cerr << "Error: Failed to parse source\n";
        return false;
    }

    stats.srcLabels = countLabels(srcModule);
    stats.srcInstructions = countInstructions(srcModule);
    stats.srcDirectives = countDirectives(srcModule);
    stats.srcSymbols = countSymbols(srcModule);

    // Convert to target format
    auto tgtWriter = WriterRegistry::getInstance().createWriter(targetFormat);
    if (!tgtWriter) {
        std::cerr << "Error: Unsupported target format\n";
        return false;
    }

    std::string tgtOutput = tgtWriter->write(srcModule);
    if (tgtWriter->hasErrors()) {
        std::cerr << "Error: Failed to write target format\n";
        return false;
    }

    // Parse target format to count resulting metrics
    auto tgtParser = ParserRegistry::getInstance().createParser(targetFormat);
    if (!tgtParser) {
        std::cerr << "Error: Cannot re-parse target format\n";
        return false;
    }

    auto tgtModule = tgtParser->parse(tgtOutput);
    if (tgtParser->hasErrors()) {
        // Warn but don't fail - target might have parsing issues
        if (verbose) {
            std::cerr << "Warning: Target format output has parsing issues\n";
        }
    }

    stats.tgtLabels = countLabels(tgtModule);
    stats.tgtInstructions = countInstructions(tgtModule);
    stats.tgtDirectives = countDirectives(tgtModule);
    stats.tgtComments = std::count(tgtOutput.begin(), tgtOutput.end(), ';');
    stats.tgtSymbols = countSymbols(tgtModule);
    stats.tgtLines = std::count(tgtOutput.begin(), tgtOutput.end(), '\n') + 1;

    // Calculate losses
    stats.lostLabels = std::max(0, stats.srcLabels - stats.tgtLabels);
    stats.lostInstructions = std::max(0, stats.srcInstructions - stats.tgtInstructions);
    stats.lostDirectives = std::max(0, stats.srcDirectives - stats.tgtDirectives);
    stats.lostComments = std::max(0, stats.srcComments - stats.tgtComments);
    stats.lostSymbols = std::max(0, stats.srcSymbols - stats.tgtSymbols);
    stats.lostMacros = stats.srcMacros;  // Macros always lost in conversion

    // Detect unsupported directives
    stats.unsupportedDirectives = detectUnsupportedDirectives(srcModule, targetFormat);

    if (verbose) {
        std::cerr << "Conversion analysis: " << inputFile << " (" << actualFormat
                  << " → " << targetFormat << ")\n";
    }

    return true;
}

static bool validateRoundTrip(const std::string& inputFile,
                             const std::string& inputFormat,
                             const std::string& targetFormat,
                             bool verbose,
                             ValidationStats& stats) {
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
        } else {
            std::cerr << "Error: Could not auto-detect input format for '" << inputFile << "'\n";
            return false;
        }
    }

    if (verbose) {
        std::cerr << "Validating round-trip: " << inputFile << " (" << actualFormat
                  << " → " << targetFormat << " → " << actualFormat << ")\n";
    }

    // Parse original input
    auto origParser = ParserRegistry::getInstance().createParser(actualFormat);
    if (!origParser) {
        std::cerr << "Error: Unsupported input format '" << actualFormat << "'\n";
        return false;
    }

    auto origModule = origParser->parse(source);
    if (origParser->hasErrors()) {
        std::cerr << "Error: Failed to parse original file\n";
        return false;
    }

    // Count original elements
    stats.totalLabels = countLabels(origModule);
    stats.totalSymbols = countSymbols(origModule);
    stats.totalDirectives = countDirectives(origModule);
    stats.totalComments = countComments(origModule);

    // Write to target format
    auto toTargetWriter = WriterRegistry::getInstance().createWriter(targetFormat);
    if (!toTargetWriter) {
        std::cerr << "Error: Unsupported target format '" << targetFormat << "'\n";
        return false;
    }

    std::string targetOutput = toTargetWriter->write(origModule);
    if (toTargetWriter->hasErrors()) {
        std::cerr << "Error: Failed to write to target format\n";
        return false;
    }

    // Parse target format output
    auto fromTargetParser = ParserRegistry::getInstance().createParser(targetFormat);
    if (!fromTargetParser) {
        std::cerr << "Error: Cannot re-parse target format\n";
        return false;
    }

    auto targetModule = fromTargetParser->parse(targetOutput);
    if (fromTargetParser->hasErrors()) {
        std::cerr << "Error: Failed to parse target format output\n";
        return false;
    }

    // Write back to original format
    auto backParser = ParserRegistry::getInstance().createParser(actualFormat);
    if (!backParser) {
        std::cerr << "Error: Cannot create parser for round-trip\n";
        return false;
    }

    auto backWriter = WriterRegistry::getInstance().createWriter(actualFormat);
    if (!backWriter) {
        std::cerr << "Error: Cannot create writer for round-trip\n";
        return false;
    }

    std::string roundTripOutput = backWriter->write(targetModule);
    if (backWriter->hasErrors()) {
        std::cerr << "Error: Failed to write back to original format\n";
        return false;
    }

    // Count preserved elements
    auto origLabels = extractLabels(source);
    auto roundTripLabels = extractLabels(roundTripOutput);
    for (const auto& label : origLabels) {
        if (roundTripLabels.count(label)) {
            stats.preservedLabels++;
        }
    }

    // Count preserved symbols
    for (const auto& [name, sym] : origModule.symbols) {
        if (targetModule.symbols.count(name)) {
            stats.preservedSymbols++;
        }
    }

    // Simple heuristic: count preserved directives by type
    int origDirectives = 0, targetDirectives = 0;
    for (const auto& stmt : origModule.statements) {
        if (stmt.type == AsmIR::Statement::Type::DIRECTIVE) {
            origDirectives++;
        }
    }
    for (const auto& stmt : targetModule.statements) {
        if (stmt.type == AsmIR::Statement::Type::DIRECTIVE) {
            targetDirectives++;
        }
    }
    stats.preservedDirectives = (origDirectives == targetDirectives) ? origDirectives : 0;

    // Check for comments (rough heuristic: count lines with semicolons)
    int origComments = std::count(source.begin(), source.end(), ';');
    int roundTripComments = std::count(roundTripOutput.begin(), roundTripOutput.end(), ';');
    stats.preservedComments = (origComments == roundTripComments) ? roundTripComments : 0;
    stats.totalComments = origComments;

    stats.originalOutput = source;
    stats.roundTripOutput = roundTripOutput;
    stats.isLossless = (stats.getPreservationPercent() >= 95.0);  // 95% threshold for "lossless"

    return true;
}

int main(int argc, char** argv) {
    registerFormats();

    std::vector<std::string> inputFiles;
    std::string inputFormat = "";  // Empty means auto-detect
    std::string targetFormat = "ca45";
    std::string outputDir;
    bool verbose = false;
    bool validateMode = false;
    bool statsMode = false;
    bool metricsMode = false;  // Phase 4a
    bool clearCache = false;   // Phase 4a
    bool parallelMode = false;  // Phase 4b
    int numThreads = 0;         // Phase 4b (0 = auto-detect)

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-f" && i + 1 < argc) {
            inputFormat = argv[++i];
        } else if (arg == "-t" && i + 1 < argc) {
            targetFormat = argv[++i];
        } else if (arg == "-o" && i + 1 < argc) {
            outputDir = argv[++i];
        } else if (arg == "--validate") {
            validateMode = true;
        } else if (arg == "--stats") {
            statsMode = true;
        } else if (arg == "--metrics") {  // Phase 4a
            metricsMode = true;
        } else if (arg == "--clear-cache") {  // Phase 4a
            clearCache = true;
        } else if (arg == "--parallel") {  // Phase 4b
            parallelMode = true;
        } else if (arg == "--threads" && i + 1 < argc) {  // Phase 4b
            numThreads = std::atoi(argv[++i]);
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
        if (!outputDir.empty() && !validateMode && !statsMode) {
            std::cerr << "Output directory: " << outputDir << "\n";
        }
    }

    // Conversion statistics mode
    if (statsMode) {
        int failureCount = 0;
        int successCount = 0;
        std::vector<ConversionStats> allStats;

        for (size_t i = 0; i < inputFiles.size(); ++i) {
            const std::string& inputFile = inputFiles[i];
            ConversionStats stats;

            if (verbose && inputFiles.size() > 1) {
                std::cerr << "\n[" << (i + 1) << "/" << inputFiles.size() << "] ";
            }

            if (!collectConversionStats(inputFile, inputFormat, targetFormat, verbose, stats)) {
                failureCount++;
            } else {
                successCount++;
                allStats.push_back(stats);
            }
        }

        // Report statistics
        std::cout << "\n";
        for (const auto& stats : allStats) {
            std::cout << "File: " << stats.inputFile << "\n";
            std::cout << "Conversion: " << stats.inputFormat << " → " << stats.targetFormat << "\n\n";

            std::cout << "Source Metrics:\n";
            std::cout << "  Lines:        " << stats.srcLines << "\n";
            std::cout << "  Labels:       " << stats.srcLabels << "\n";
            std::cout << "  Instructions: " << stats.srcInstructions << "\n";
            std::cout << "  Directives:   " << stats.srcDirectives << "\n";
            std::cout << "  Symbols:      " << stats.srcSymbols << "\n";
            std::cout << "  Comments:     " << stats.srcComments << "\n";
            std::cout << "  Macros:       " << stats.srcMacros << "\n\n";

            std::cout << "Target Metrics:\n";
            std::cout << "  Lines:        " << stats.tgtLines << "\n";
            std::cout << "  Labels:       " << stats.tgtLabels << "\n";
            std::cout << "  Instructions: " << stats.tgtInstructions << "\n";
            std::cout << "  Directives:   " << stats.tgtDirectives << "\n";
            std::cout << "  Symbols:      " << stats.tgtSymbols << "\n";
            std::cout << "  Comments:     " << stats.tgtComments << "\n\n";

            if (stats.getTotalLoss() > 0) {
                std::cout << "Loss Analysis:\n";
                if (stats.lostLabels > 0) {
                    std::cout << "  Labels lost:       " << stats.lostLabels << "\n";
                }
                if (stats.lostInstructions > 0) {
                    std::cout << "  Instructions lost: " << stats.lostInstructions << "\n";
                }
                if (stats.lostDirectives > 0) {
                    std::cout << "  Directives lost:   " << stats.lostDirectives << "\n";
                }
                if (stats.lostComments > 0) {
                    std::cout << "  Comments lost:     " << stats.lostComments << "\n";
                }
                if (stats.lostSymbols > 0) {
                    std::cout << "  Symbols lost:      " << stats.lostSymbols << "\n";
                }
                if (stats.lostMacros > 0) {
                    std::cout << "  ⚠ Macros lost:      " << stats.lostMacros
                              << " (always lost in conversion)\n";
                }

                if (!stats.unsupportedDirectives.empty()) {
                    std::cout << "\nUnsupported Directives:\n";
                    for (const auto& dir : stats.unsupportedDirectives) {
                        std::cout << "  • " << dir << "\n";
                    }
                }
                std::cout << "\n";
            }

            // Preservation percentages
            std::cout << "Preservation:\n";
            std::cout << "  Instructions: " << std::fixed << std::setprecision(1)
                      << stats.getInstructionPreservation() << "%\n";
            std::cout << "  Metadata:     " << stats.getMetadataPreservation() << "%\n";
            std::cout << "  Overall:      " << stats.getOverallPreservation() << "%\n";

            if (stats.getOverallPreservation() >= 95.0) {
                std::cout << "  Rating:       ✓ GOOD (minimal loss)\n";
            } else if (stats.getOverallPreservation() >= 80.0) {
                std::cout << "  Rating:       ⚠ FAIR (some loss)\n";
            } else {
                std::cout << "  Rating:       ✗ POOR (significant loss)\n";
            }
            std::cout << "\n";
        }

        if (verbose && inputFiles.size() > 1) {
            std::cerr << "\nStatistics complete: " << successCount << " analyzed, "
                      << failureCount << " failed\n";
        }

        return (failureCount > 0) ? 1 : 0;
    }

    // Round-trip validation mode
    if (validateMode) {
        int failureCount = 0;
        int successCount = 0;
        std::vector<ValidationStats> allStats;

        for (size_t i = 0; i < inputFiles.size(); ++i) {
            const std::string& inputFile = inputFiles[i];
            ValidationStats stats;

            if (verbose && inputFiles.size() > 1) {
                std::cerr << "\n[" << (i + 1) << "/" << inputFiles.size() << "] ";
            }

            if (!validateRoundTrip(inputFile, inputFormat, targetFormat, verbose, stats)) {
                failureCount++;
            } else {
                successCount++;
                allStats.push_back(stats);
            }
        }

        // Report validation results
        std::cout << "\n";
        for (const auto& stats : allStats) {
            std::cout << "Results:\n";
            std::cout << "  Labels preserved:    " << stats.preservedLabels << "/" << stats.totalLabels << "\n";
            std::cout << "  Symbols preserved:   " << stats.preservedSymbols << "/" << stats.totalSymbols << "\n";
            std::cout << "  Directives preserved: " << stats.preservedDirectives << "/" << stats.totalDirectives << "\n";
            std::cout << "  Comments preserved:  " << stats.preservedComments << "/" << stats.totalComments << "\n";
            std::cout << "  Overall preservation: " << std::fixed << std::setprecision(1)
                      << stats.getPreservationPercent() << "%\n";
            if (stats.isLossless) {
                std::cout << "  Status: ✓ LOSSLESS\n";
            } else {
                std::cout << "  Status: ✗ LOSSY\n";
            }
            std::cout << "\n";
        }

        if (verbose && inputFiles.size() > 1) {
            std::cerr << "\nRound-trip validation complete: " << successCount << " succeeded, " << failureCount << " failed\n";
        }

        return (failureCount > 0) ? 1 : 0;
    }

    // Phase 4a: Clear cache if requested
    if (clearCache) {
        MacroUtils::clearCache();
        if (verbose) {
            std::cerr << "Include file cache cleared.\n";
        }
    }

    // Phase 4a: Reset metrics at start of batch conversion
    MacroUtils::resetMetrics();
    auto batchStartTime = std::chrono::high_resolution_clock::now();

    // Phase 4b: Batch conversion with optional parallel processing
    std::atomic<int> failureCount{0};
    std::atomic<int> successCount{0};

    if (parallelMode && inputFiles.size() > 1) {
        // Phase 4b: Parallel batch conversion
        if (verbose) {
            std::cerr << "Using parallel conversion with "
                      << (numThreads > 0 ? std::to_string(numThreads) : "auto-detected")
                      << " threads\n";
        }

        ThreadPool pool(numThreads > 0 ? numThreads : 0);

        // Enqueue all conversion tasks
        for (size_t i = 0; i < inputFiles.size(); ++i) {
            const std::string& inputFile = inputFiles[i];
            std::string outputFile;

            if (inputFiles.size() == 1 && outputDir.empty()) {
                outputFile = "";
            } else {
                outputFile = generateOutputPath(inputFile, targetFormat, outputDir);
            }

            pool.enqueue([inputFile, inputFormat, targetFormat, outputFile, verbose,
                         &failureCount, &successCount, i, totalCount = inputFiles.size()]() {
                if (verbose) {
                    std::cerr << "[" << (i + 1) << "/" << totalCount << "] ";
                }

                if (convertFile(inputFile, inputFormat, targetFormat, outputFile, verbose)) {
                    successCount++;
                } else {
                    failureCount++;
                }
            });
        }

        // Wait for all conversions to complete
        pool.waitAll();

        if (verbose) {
            std::cerr << "\n";
        }
    } else {
        // Sequential batch conversion (Phase 2b)
        for (size_t i = 0; i < inputFiles.size(); ++i) {
            const std::string& inputFile = inputFiles[i];
            std::string outputFile;

            if (inputFiles.size() == 1 && outputDir.empty()) {
                outputFile = "";
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
            std::cerr << "\n";
        }
    }

    auto batchEndTime = std::chrono::high_resolution_clock::now();
    auto batchDuration = std::chrono::duration_cast<std::chrono::milliseconds>(batchEndTime - batchStartTime);

    if (verbose && inputFiles.size() > 1) {
        std::cerr << "\nBatch conversion complete: " << successCount << " succeeded, " << failureCount << " failed\n";
    }

    // Phase 4: Report performance metrics if enabled or verbose
    if (metricsMode || verbose) {
        std::cout << "\n=== Performance Metrics ===\n";
        std::cout << "Processing mode:          " << (parallelMode ? "PARALLEL" : "SEQUENTIAL") << "\n";
        if (parallelMode) {
            std::cout << "Worker threads:           " << (numThreads > 0 ? std::to_string(numThreads) : "auto-detected") << "\n";
        }
        std::cout << "Total files processed:    " << inputFiles.size() << "\n";
        std::cout << "Successful conversions:   " << successCount << "\n";
        std::cout << "Failed conversions:       " << failureCount << "\n";
        std::cout << "Total time:               " << batchDuration.count() << " ms\n";
        if (inputFiles.size() > 0) {
            std::cout << "Average time per file:    "
                      << (batchDuration.count() / static_cast<double>(inputFiles.size()))
                      << " ms\n";
        }
        std::cout << "Include cache hits:       " << MacroUtils::getCacheHitCount() << "\n";
        std::cout << "\n";
    }

    return (failureCount > 0) ? 1 : 0;
}
