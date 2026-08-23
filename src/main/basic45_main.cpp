#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include "BasicEmitter.hpp"
#include "PETSCIIEncoder.hpp"
#include "BasicPreprocessor.hpp"
#include "BasicDocGenerator.hpp"
#include "BasicValidator.hpp"

struct Options {
    std::string inputFile;
    std::string outputFile;
    std::string symbolFile;
    std::string labelTableFile;
    std::string docsFile;
    std::string includePath;
    uint16_t loadAddress = 0x0801;
    uint16_t lineIncrement = 10;
    bool verbose = false;
    bool showVersion = false;
    bool showHelp = false;
    bool listTokens = false;
    bool generateDocs = false;
    bool preserveSpaces = false;
    bool validateOnly = false;  // Check-only mode (no compilation)
    bool strictValidation = false;  // Treat warnings as errors
    // Note: Label-based system is now the default (no --labels flag needed)
};

class SymbolTable {
public:
    void loadFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file) {
            std::cerr << "Warning: Could not open symbol file: " << filename << std::endl;
            return;
        }

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == ';' || line[0] == '#') continue;

            std::istringstream iss(line);
            std::string name;
            uint16_t addr;

            if (iss >> std::hex >> addr >> name) {
                symbols[name] = addr;
            }
        }
    }

    std::string substitute(const std::string& text) const {
        std::string result = text;
        size_t pos = 0;

        while ((pos = result.find('{', pos)) != std::string::npos) {
            size_t end = result.find('}', pos);
            if (end == std::string::npos) break;

            std::string name = result.substr(pos + 1, end - pos - 1);
            auto it = symbols.find(name);

            if (it != symbols.end()) {
                char buffer[16];
                snprintf(buffer, sizeof(buffer), "%u", it->second);
                result.replace(pos, end - pos + 1, buffer);
                pos += std::strlen(buffer);
            } else {
                pos = end + 1;
            }
        }

        return result;
    }

private:
    std::unordered_map<std::string, uint16_t> symbols;
};

static void printUsage(const char* progName) {
    std::cout << "Usage: " << progName << " [options] <input.bas> -o <output.prg>" << std::endl;
    std::cout << "Label-Based BASIC Compiler — Source uses labels, not line numbers" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -o <file>           Output file (default: output.prg)" << std::endl;
    std::cout << "  --symbols <f>       Load symbol table from file (cc45 -E output)" << std::endl;
    std::cout << "  --load <addr>       Load address in hex (default: 0x0801)" << std::endl;
    std::cout << "  --increment <n>     Auto line number increment (default: 10)" << std::endl;
    std::cout << "  --label-table <f>   Output label→line number mapping to file" << std::endl;
    std::cout << "  --docs <f>          Generate markdown documentation to file" << std::endl;
    std::cout << "  -I <path>           Add include search path for #include" << std::endl;
    std::cout << "  --preserve-spaces   Preserve spaces in tokenized output" << std::endl;
    std::cout << "  --validate          Validate only (no compilation)" << std::endl;
    std::cout << "  --strict            Treat validation warnings as errors" << std::endl;
    std::cout << "  --list-tokens       List all supported BASIC keywords and exit" << std::endl;
    std::cout << "  -v, --verbose       Verbose output" << std::endl;
    std::cout << "  --version           Show version" << std::endl;
    std::cout << "  -h, --help          Show this help" << std::endl;
}

static Options parseArgs(int argc, char* argv[]) {
    Options opts;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-o" && i + 1 < argc) {
            opts.outputFile = argv[++i];
        } else if (arg == "--symbols" && i + 1 < argc) {
            opts.symbolFile = argv[++i];
        } else if (arg == "--load" && i + 1 < argc) {
            std::istringstream iss(argv[++i]);
            iss >> std::hex >> opts.loadAddress;
        } else if (arg == "-v" || arg == "--verbose") {
            opts.verbose = true;
        } else if (arg == "--label-table" && i + 1 < argc) {
            opts.labelTableFile = argv[++i];
        } else if (arg == "--increment" && i + 1 < argc) {
            std::istringstream iss(argv[++i]);
            iss >> opts.lineIncrement;
        } else if (arg == "--docs" && i + 1 < argc) {
            opts.docsFile = argv[++i];
            opts.generateDocs = true;
        } else if (arg == "-I" && i + 1 < argc) {
            opts.includePath = argv[++i];
        } else if (arg == "--preserve-spaces") {
            opts.preserveSpaces = true;
        } else if (arg == "--validate") {
            opts.validateOnly = true;
        } else if (arg == "--strict") {
            opts.strictValidation = true;
        } else if (arg == "--list-tokens") {
            opts.listTokens = true;
        } else if (arg == "--version") {
            opts.showVersion = true;
        } else if (arg == "-h" || arg == "--help") {
            opts.showHelp = true;
        } else if (arg[0] != '-') {
            opts.inputFile = arg;
        }
    }

    if (opts.outputFile.empty() && !opts.inputFile.empty()) {
        opts.outputFile = opts.inputFile;
        size_t dot = opts.outputFile.rfind('.');
        if (dot != std::string::npos) {
            opts.outputFile = opts.outputFile.substr(0, dot);
        }
        opts.outputFile += ".prg";
    }

    return opts;
}

static std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Error: Could not open file: " << filename << std::endl;
        exit(1);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

static void writeFile(const std::string& filename, const std::vector<uint8_t>& data) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Could not open file for writing: " << filename << std::endl;
        exit(1);
    }

    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    if (!file) {
        std::cerr << "Error: Could not write file: " << filename << std::endl;
        exit(1);
    }
}

static void printTokens() {
    BasicTokenizer tokenizer;
    const auto& keywords = tokenizer.getKeywords();
    const auto& escapeKeywords = tokenizer.getEscapeKeywords();

    std::vector<std::pair<uint8_t, std::string>> sorted;
    for (const auto& kw : keywords) {
        sorted.push_back({kw.second, kw.first});
    }
    std::sort(sorted.begin(), sorted.end());

    std::cout << "MEGA65 BASIC 65 Single-byte Keywords (total: " << sorted.size() << ")" << std::endl;
    std::cout << std::endl;

    int col = 0;
    for (const auto& token : sorted) {
        printf("0x%02X %-12s  ", token.first, token.second.c_str());
        col++;
        if (col % 4 == 0) {
            std::cout << std::endl;
        }
    }
    if (col % 4 != 0) {
        std::cout << std::endl;
    }

    std::cout << std::endl;
    std::cout << "MEGA65 BASIC 65 Two-byte Keywords (0xFE + escape byte, total: " << escapeKeywords.size() << ")" << std::endl;
    std::cout << std::endl;

    std::vector<std::pair<uint8_t, std::string>> escapeSorted;
    for (const auto& kw : escapeKeywords) {
        escapeSorted.push_back({kw.second, kw.first});
    }
    std::sort(escapeSorted.begin(), escapeSorted.end());

    col = 0;
    for (const auto& token : escapeSorted) {
        printf("0xFE%02X %-14s  ", token.first, token.second.c_str());
        col++;
        if (col % 3 == 0) {
            std::cout << std::endl;
        }
    }
    if (col % 3 != 0) {
        std::cout << std::endl;
    }
}

int main(int argc, char* argv[]) {
    Options opts = parseArgs(argc, argv);

    if (opts.listTokens) {
        printTokens();
        return 0;
    }

    if (opts.showVersion) {
        std::cout << "basic45 v1.0" << std::endl;
        return 0;
    }

    if (opts.showHelp || opts.inputFile.empty()) {
        printUsage(argv[0]);
        return opts.showHelp ? 0 : 1;
    }

    if (opts.verbose) {
        std::cout << "Input:  " << opts.inputFile << std::endl;
        std::cout << "Output: " << opts.outputFile << std::endl;
        std::cout << "Load:   $" << std::hex << opts.loadAddress << std::dec << std::endl;
        if (opts.lineIncrement != 10) {
            std::cout << "Line increment: " << opts.lineIncrement << std::endl;
        }
    }

    std::string sourceCode = readFile(opts.inputFile);

    // Preprocess (handle #include, #define, #ifdef, etc.)
    BasicPreprocessor preprocessor;
    if (!opts.includePath.empty()) {
        preprocessor.addIncludePath(opts.includePath);
    }
    try {
        sourceCode = preprocessor.preprocess(sourceCode, opts.inputFile);
        if (opts.verbose && !preprocessor.getIncludedFiles().empty()) {
            std::cout << "Included files: " << preprocessor.getIncludedFiles().size() << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Preprocessor error: " << e.what() << std::endl;
        return 1;
    }

    SymbolTable symbols;
    if (!opts.symbolFile.empty()) {
        symbols.loadFromFile(opts.symbolFile);
        sourceCode = symbols.substitute(sourceCode);
        if (opts.verbose) {
            std::cout << "Symbols loaded from: " << opts.symbolFile << std::endl;
        }
    }

    // Validation pass (Feature #1: Compile-Time Validation & Safety)
    // Now label-based: auto-assigns line numbers and validates labels
    BasicValidator validator(opts.inputFile);
    validator.analyze(sourceCode);

    if (validator.hasErrors()) {
        validator.printErrors();

        int errorCount = validator.getErrorCount();
        int warningCount = validator.getWarningCount();

        // Exit if there are errors, or if strict mode and there are warnings
        if (errorCount > 0 || (opts.strictValidation && warningCount > 0)) {
            if (opts.verbose || !opts.validateOnly) {
                std::cerr << "Compilation failed: " << errorCount << " error(s), "
                          << warningCount << " warning(s)" << std::endl;
            }
            return errorCount > 0 ? 1 : 1;
        }

        if (opts.verbose && warningCount > 0) {
            std::cerr << "Validation: " << warningCount << " warning(s) (non-fatal)" << std::endl;
        }
    }

    // If validation-only mode, exit here
    if (opts.validateOnly) {
        if (opts.verbose) {
            std::cout << "Validation successful" << std::endl;
        }
        return 0;
    }

    BasicEmitter emitter(opts.loadAddress, opts.lineIncrement);
    if (opts.preserveSpaces) {
        emitter.setPreserveSpaces(true);
    }

    // Emit from label-based parsed source (now the default system)
    auto binary = emitter.emitFromLabels(validator.getParser());

    writeFile(opts.outputFile, binary);

    if (!opts.labelTableFile.empty()) {
        emitter.outputLabelTable(opts.labelTableFile);
        if (opts.verbose) {
            std::cout << "Label table written to: " << opts.labelTableFile << std::endl;
        }
    }

    if (opts.generateDocs && !opts.docsFile.empty()) {
        BasicDocGenerator docGen;
        std::string docs = docGen.generateMarkdown(sourceCode, opts.inputFile);
        std::ofstream docFile(opts.docsFile);
        if (docFile) {
            docFile << docs;
            if (opts.verbose) {
                std::cout << "Documentation written to: " << opts.docsFile << std::endl;
            }
        } else {
            std::cerr << "Warning: Could not write documentation file: " << opts.docsFile << std::endl;
        }
    }

    if (opts.verbose) {
        std::cout << "Generated " << binary.size() << " bytes" << std::endl;
    }

    return 0;
}
