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
#include "SymbolExpressionEvaluator.hpp"
#include "BasicMinifier.hpp"
#include "BasicTooling.hpp"
#include "Version.hpp"
#include "BasicStructured.hpp"
#include "BasicDecompiler.hpp"
#include <filesystem>
#include <map>
#include <memory>
#include <regex>
#include <sys/wait.h>
#include <unistd.h>

namespace fs = std::filesystem;

struct Options {
    std::string inputFile;
    std::string outputFile;
    std::string symbolFile;
    std::string labelTableFile;
    std::string docsFile;
    std::string includePath;
    uint16_t loadAddress = 0x2001;
    uint16_t lineIncrement = 10;
    bool verbose = false;
    bool showVersion = false;
    bool showHelp = false;
    bool listTokens = false;
    bool generateDocs = false;
    bool preserveSpaces = false;
    bool validateOnly = false;  // Check-only mode (no compilation)
    bool strictValidation = false;  // Treat warnings as errors
    bool minify = false;  // Code minification (remove comments, whitespace)
    bool dryRun = false;  // Don't write files, just report actions
    bool watchMode = false;  // Monitor file changes and recompile
    bool generateSourceMap = false;  // Generate source map
    std::string sourceMapFile;  // Output file for source map
    bool launchEmulator = false;  // Run the program in xemu afterwards
    std::string emulatorPath;     // xemu-xmega65 binary ("" searches PATH)
    bool listing = false;         // --list: BASIC lines next to their source
    bool decompile = false;       // --decompile: .prg -> basic45 source
    bool keepNames = false;       // --keep-names: no variable renaming
    std::string diskImage;        // --disk: add the program to this image
    std::string diskName;         // --disk-name: its name there
    std::vector<std::string> linkFiles;   // --link: C / asm / .o45 to link in
    std::string linkLib;                  // --lib: the C library for --link
};

class SymbolTable {
public:
    SymbolTable() : evaluator(nullptr) {}

    void loadFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file) {
            std::cerr << "Warning: Could not open symbol file: " << filename << std::endl;
            return;
        }

        // Initialize evaluator if not already done
        if (!evaluator) {
            evaluator = std::make_unique<SymbolExpressionEvaluator>();
        }

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == ';' || line[0] == '#') continue;

            std::istringstream iss(line);
            std::string name;
            uint16_t addr;

            if (iss >> std::hex >> addr >> name) {
                symbols[name] = addr;
                // Also add to evaluator for expression evaluation
                if (evaluator) {
                    evaluator->addSymbol(name, addr);
                }
            }
        }
    }

    void addSymbol(const std::string& name, uint16_t addr) {
        if (!evaluator) evaluator = std::make_unique<SymbolExpressionEvaluator>();
        symbols[name] = addr;
        evaluator->addSymbol(name, addr);
    }
    bool has(const std::string& name) const { return symbols.count(name) > 0; }
    bool empty() const { return symbols.empty(); }

    std::string substitute(const std::string& text) const {
        std::string result = text;
        size_t pos = 0;

        while ((pos = result.find('{', pos)) != std::string::npos) {
            size_t end = result.find('}', pos);
            if (end == std::string::npos) break;

            std::string expr = result.substr(pos + 1, end - pos - 1);
            std::string replacement;

            // Try to evaluate as expression if evaluator is available
            if (evaluator) {
                int value = evaluator->evaluateExpression(expr);
                if (value >= 0) {
                    // Successfully evaluated expression
                    replacement = std::to_string(value);
                } else {
                    // Not a valid expression, try simple symbol lookup
                    auto it = symbols.find(expr);
                    if (it != symbols.end()) {
                        replacement = std::to_string(it->second);
                    } else {
                        // Not found, skip this substitution
                        pos = end + 1;
                        continue;
                    }
                }
            } else {
                // No evaluator, do simple symbol lookup
                auto it = symbols.find(expr);
                if (it != symbols.end()) {
                    replacement = std::to_string(it->second);
                } else {
                    pos = end + 1;
                    continue;
                }
            }

            // Replace the {expr} with the result
            result.replace(pos, end - pos + 1, replacement);
            pos += replacement.length();
        }

        return result;
    }

private:
    std::unordered_map<std::string, uint16_t> symbols;
    std::unique_ptr<SymbolExpressionEvaluator> evaluator;
};


static void printUsage(const char* progName) {
    std::cout << "Usage: " << progName << " [options] <input.bas> [-o <output.prg>]" << std::endl;
    std::cout << "       " << progName << " --decompile <program.prg> [-o <output.bas>]" << std::endl;
    std::cout << "Label-Based BASIC 65 Compiler — Source uses labels, not line numbers" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -o <file>           Output file (default: the input name with .prg)" << std::endl;
    std::cout << "  --symbols <f>       Symbol file (ln45 -m map) for {name} substitution" << std::endl;
    std::cout << "  --load <addr>       Load address in hex (default: 0x2001, MEGA65 BASIC 65)" << std::endl;
    std::cout << "  --increment <n>     Line numbers n, 2n, 3n, ... (default: 10)" << std::endl;
    std::cout << "  --label-table <f>   Write label, line number and address to file" << std::endl;
    std::cout << "  --list              Print the BASIC lines next to their source lines" << std::endl;
    std::cout << "  --keep-names        Do not rename variables longer than two characters" << std::endl;
    std::cout << "  --docs <f>          Generate markdown documentation to file" << std::endl;
    std::cout << "  -I <path>           Add include search path for #include" << std::endl;
    std::cout << "  --minify            Minify output (remove comments, join lines)" << std::endl;
    std::cout << "  --validate          Validate only (no compilation)" << std::endl;
    std::cout << "  --strict            Treat validation warnings as errors" << std::endl;
    std::cout << "  --dry-run           Don't write files, report actions only" << std::endl;
    std::cout << "  --source-map <f>    Write a JSON map of BASIC lines to source lines" << std::endl;
    std::cout << "  --disk <image>      Also add the program to a disk image (created if missing)" << std::endl;
    std::cout << "  --disk-name <name>  Its name on the disk (default: the output file name)" << std::endl;
    std::cout << "  --emulator          Run the program in xemu (xemu-xmega65) afterwards" << std::endl;
    std::cout << "  --emulator-path <f> The xemu-xmega65 binary (default: found on PATH)" << std::endl;
    std::cout << "  --link <file>       Link a C (.c), assembly (.s45) or object (.o45) file" << std::endl;
    std::cout << "                      after the program; SYS {name} calls its function name" << std::endl;
    std::cout << "  --lib <file>        The C library for --link (default: c45.lib)" << std::endl;
    std::cout << "  --decompile         Turn a BASIC .prg into basic45 source (also for a .prg input)" << std::endl;
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
        } else if (arg == "--minify") {
            opts.minify = true;
        } else if (arg == "--preserve-spaces") {
            opts.preserveSpaces = true;
        } else if (arg == "--validate") {
            opts.validateOnly = true;
        } else if (arg == "--strict") {
            opts.strictValidation = true;
        } else if (arg == "--dry-run") {
            opts.dryRun = true;
        } else if (arg == "--watch") {
            opts.watchMode = true;
        } else if (arg == "--source-map" && i + 1 < argc) {
            opts.sourceMapFile = argv[++i];
            opts.generateSourceMap = true;
        } else if (arg == "--emulator") {
            opts.launchEmulator = true;
        } else if (arg == "--emulator-path" && i + 1 < argc) {
            opts.emulatorPath = argv[++i];
        } else if (arg == "--list") {
            opts.listing = true;
        } else if (arg == "--keep-names") {
            opts.keepNames = true;
        } else if (arg == "--decompile") {
            opts.decompile = true;
        } else if (arg == "--disk" && i + 1 < argc) {
            opts.diskImage = argv[++i];
        } else if (arg == "--disk-name" && i + 1 < argc) {
            opts.diskName = argv[++i];
        } else if (arg == "--link" && i + 1 < argc) {
            opts.linkFiles.push_back(argv[++i]);
        } else if (arg == "--lib" && i + 1 < argc) {
            opts.linkLib = argv[++i];
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

    // a .prg input is decompiled
    if (opts.inputFile.size() > 4 &&
        opts.inputFile.compare(opts.inputFile.size() - 4, 4, ".prg") == 0)
        opts.decompile = true;

    if (opts.outputFile.empty() && !opts.inputFile.empty()) {
        opts.outputFile = opts.inputFile;
        size_t dot = opts.outputFile.rfind('.');
        if (dot != std::string::npos) {
            opts.outputFile = opts.outputFile.substr(0, dot);
        }
        opts.outputFile += opts.decompile ? ".bas" : ".prg";
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

static std::vector<uint8_t> readBinary(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Could not open file: " << filename << std::endl;
        exit(1);
    }
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
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

// The directory of this executable (the other tools are next to it)
static std::string toolDir(const char* argv0) {
    std::error_code ec;
    fs::path self = fs::read_symlink("/proc/self/exe", ec);
    if (ec) self = fs::absolute(argv0, ec);
    return self.parent_path().string();
}

static std::string shellQuote(const std::string& s) {
    std::string q = "'";
    for (char c : s) { if (c == '\'') q += "'\\''"; else q += c; }
    return q + "'";
}

// Run a command; its standard output (and error, merged) in *out
static int runCommand(const std::string& cmd, std::string* out) {
    FILE* p = popen((cmd + " 2>&1").c_str(), "r");
    if (!p) return -1;
    char buf[512];
    std::string text;
    while (fgets(buf, sizeof buf, p)) text += buf;
    int rc = pclose(p);
    if (out) *out = text;
    return WIFEXITED(rc) ? WEXITSTATUS(rc) : -1;
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


// One build of the program from the lowered source: symbol substitution,
// minify, validation (with messages at their source lines) and emission
struct Build {
    std::unique_ptr<BasicValidator> validator;
    std::unique_ptr<BasicEmitter> emitter;
    std::vector<int> minOrigin;     // after minify: line -> lowered line (empty: identity)
    std::vector<uint8_t> binary;
    std::string text;               // the text given to the parser
    bool ok = false;
};

int main(int argc, char* argv[]) {
    Options opts = parseArgs(argc, argv);

    if (opts.listTokens) {
        printTokens();
        return 0;
    }

    if (opts.showVersion) {
        std::cout << suiteVersionString("basic45") << std::endl;
        return 0;
    }

    if (opts.showHelp || opts.inputFile.empty()) {
        printUsage(argv[0]);
        return opts.showHelp ? 0 : 1;
    }

    // --decompile: a tokenized program back to basic45 source
    if (opts.decompile) {
        auto r = decompileBasic(readBinary(opts.inputFile));
        for (const auto& w : r.warnings) std::cerr << opts.inputFile << ": warning: " << w << std::endl;
        if (!r.ok) {
            std::cerr << opts.inputFile << ": error: " << r.error << std::endl;
            return 1;
        }
        if (opts.outputFile == "-") {
            std::cout << r.source;
        } else {
            std::ofstream out(opts.outputFile);
            if (!out) { std::cerr << "Error: Could not write " << opts.outputFile << std::endl; return 1; }
            out << r.source;
            if (opts.verbose) std::cout << "Source written to: " << opts.outputFile << std::endl;
        }
        return 0;
    }

    if (opts.verbose) {
        std::cout << "Input:  " << opts.inputFile << std::endl;
        std::cout << "Output: " << opts.outputFile << std::endl;
        std::cout << "Load:   $" << std::hex << opts.loadAddress << std::dec << std::endl;
        if (opts.lineIncrement != 10) {
            std::cout << "Line increment: " << opts.lineIncrement << std::endl;
        }
    }

    std::string originalSource = readFile(opts.inputFile);
    std::string sourceCode;

    // Preprocess (handle #include, #define, #ifdef, etc.)
    BasicPreprocessor preprocessor;
    if (!opts.includePath.empty()) {
        preprocessor.addIncludePath(opts.includePath);
    }
    try {
        sourceCode = preprocessor.preprocess(originalSource, opts.inputFile);
        if (opts.verbose && !preprocessor.getIncludedFiles().empty()) {
            std::cout << "Included files: " << preprocessor.getIncludedFiles().size() << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Preprocessor error: " << e.what() << std::endl;
        return 1;
    }

    // Block IF / proc / call (BasicStructured)
    BasicStructuredResult lowered = lowerBasicStructured(sourceCode);
    auto preOrigin = [&](int loweredLine) -> std::pair<std::string, int> {
        if (loweredLine < 1 || loweredLine > (int)lowered.origin.size()) return {"", 0};
        auto o = preprocessor.getLineOrigin(lowered.origin[loweredLine - 1]);
        if (o.first.empty()) o.first = opts.inputFile;
        return o;
    };
    if (!lowered.errors.empty()) {
        for (const auto& [line, msg] : lowered.errors) {
            auto o = preprocessor.getLineOrigin(line);
            std::cerr << (o.first.empty() ? opts.inputFile : o.first) << ":" << (o.second ? o.second : line)
                      << ":1: error: " << msg << std::endl;
        }
        return 1;
    }

    SymbolTable symbols;
    if (!opts.symbolFile.empty()) {
        symbols.loadFromFile(opts.symbolFile);
        if (opts.verbose) {
            std::cout << "Symbols loaded from: " << opts.symbolFile << std::endl;
        }
    }

    // --link: SYS {name} calls the C (or asm) function name through a thunk
    // (__bas_name, which enters crt0's __basic_call)
    std::vector<std::string> sysTargets;
    std::string programText = lowered.text;
    if (!opts.linkFiles.empty()) {
        std::regex sysRef(R"((\bsys\s*)\{([A-Za-z_]\w*)\})", std::regex::icase);
        std::string out;
        auto begin = std::sregex_iterator(programText.begin(), programText.end(), sysRef);
        size_t last = 0;
        for (auto it = begin; it != std::sregex_iterator(); ++it) {
            const auto& m = *it;
            // not in a comment line (' or REM) or inside a string
            size_t lineStart = programText.rfind('\n', m.position(0));
            lineStart = lineStart == std::string::npos ? 0 : lineStart + 1;
            std::string before = programText.substr(lineStart, m.position(0) - lineStart);
            size_t first = before.find_first_not_of(" \t");
            if (first != std::string::npos && (before[first] == '\'' || before.compare(first, 3, "REM") == 0))
                continue;
            if (std::count(before.begin(), before.end(), '"') % 2) continue;
            std::string name = m[2].str();
            if (std::find(sysTargets.begin(), sysTargets.end(), name) == sysTargets.end())
                sysTargets.push_back(name);
            out += programText.substr(last, m.position(0) - last);
            out += m[1].str() + "{__bas_" + name + "}";
            last = m.position(0) + m.length(0);
        }
        out += programText.substr(last);
        programText = out;
    }

    auto build = [&](const SymbolTable& syms) -> Build {
        Build b;
        std::string text = syms.empty() ? programText : syms.substitute(programText);
        if (opts.minify) {
            BasicMinifier minifier;
            text = minifier.minify(text);
            b.minOrigin = minifier.origins();
        }
        b.text = text;
        b.validator = std::make_unique<BasicValidator>(opts.inputFile);
        std::vector<int> minOrigin = b.minOrigin;
        b.validator->setOrigin([&preOrigin, minOrigin](int line) {
            int l = line;
            if (!minOrigin.empty()) l = (line >= 1 && line <= (int)minOrigin.size()) ? minOrigin[line - 1] : 0;
            return preOrigin(l);
        });
        b.validator->analyze(text, opts.lineIncrement);
        if (b.validator->hasErrors()) {
            b.validator->printErrors();
            int errorCount = b.validator->getErrorCount();
            int warningCount = b.validator->getWarningCount();
            if (errorCount > 0 || (opts.strictValidation && warningCount > 0)) {
                std::cerr << "Compilation failed: " << errorCount << " error(s), "
                          << warningCount << " warning(s)" << std::endl;
                return b;
            }
        }
        b.emitter = std::make_unique<BasicEmitter>(opts.loadAddress, opts.lineIncrement);
        b.emitter->setRenameVariables(!opts.keepNames);
        if (opts.preserveSpaces) b.emitter->setPreserveSpaces(true);
        b.binary = b.emitter->emitFromLabels(b.validator->getParser());
        b.ok = true;
        return b;
    };

    // The source (file, line) of a line of a build's parsed text
    auto originOf = [&](const Build& b, int line) {
        int l = line;
        if (!b.minOrigin.empty()) l = (line >= 1 && line <= (int)b.minOrigin.size()) ? b.minOrigin[line - 1] : 0;
        return preOrigin(l);
    };

    if (opts.validateOnly) {
        if (opts.linkFiles.empty()) {
            Build b = build(symbols);
            if (!b.ok) return 1;
        }
        if (opts.verbose) std::cout << "Validation successful" << std::endl;
        return 0;
    }

    Build result;
    std::vector<uint8_t> binary;
    if (opts.linkFiles.empty()) {
        result = build(symbols);
        if (!result.ok) return 1;
        binary = result.binary;
    } else {
        // Pass 1: every symbol not yet known as $FFFF (the most digits), to
        // place the code after the program
        SymbolTable placeholder;
        if (!opts.symbolFile.empty()) placeholder.loadFromFile(opts.symbolFile);
        std::regex braces(R"(\{([^}]*)\})");
        std::regex ident(R"([A-Za-z_]\w*)");
        PETSCIIEncoder petscii;
        for (auto it = std::sregex_iterator(programText.begin(), programText.end(), braces);
             it != std::sregex_iterator(); ++it) {
            std::string inner = (*it)[1].str();
            std::vector<uint8_t> esc;
            if (petscii.escapeBytes(inner, esc)) continue;
            for (auto jt = std::sregex_iterator(inner.begin(), inner.end(), ident);
                 jt != std::sregex_iterator(); ++jt)
                if (!placeholder.has(jt->str())) placeholder.addSymbol(jt->str(), 0xFFFF);
        }
        Build first = build(placeholder);
        if (!first.ok) return 1;
        uint32_t basicEnd = opts.loadAddress + (uint32_t)first.binary.size() - 2;
        uint32_t codeBase = (basicEnd + 0xFF) & ~0xFFu;

        // The thunks, the linked files, the library
        std::string tools = toolDir(argv[0]);
        fs::path tmp = fs::temp_directory_path() / ("basic45_link_" + std::to_string(getpid()));
        fs::create_directories(tmp);
        std::vector<std::string> objects;
        {
            std::ofstream th(tmp / "thunks.s45");
            th << "; basic45 --link: a thunk per SYS {name} target\n";
            th << ".extern __basic_call\n.extern ___bc_target\n";
            th << "; main is not needed (the program is BASIC); a C main overrides this\n";
            th << ".weak _main\n";
            th << ".segment \"code\"\n_main:\n    rts\n";
            for (const auto& n : sysTargets) {
                std::string target = n[0] == '_' ? n : "_" + n;
                th << ".extern " << target << "\n.global __bas_" << n << "\n__bas_" << n << ":\n"
                   << "    pha\n    lda #<" << target << "\n    sta ___bc_target\n"
                   << "    lda #>" << target << "\n    sta ___bc_target + 1\n"
                   << "    pla\n    jmp __basic_call\n";
            }
        }
        std::string log;
        auto step = [&](const std::string& cmd, const std::string& what) {
            if (opts.verbose) std::cout << "  " << cmd << std::endl;
            if (runCommand(cmd, &log) != 0) {
                std::cerr << "basic45: " << what << " failed:\n" << log;
                return false;
            }
            return true;
        };
        if (!step(shellQuote(tools + "/ca45") + " -c " + shellQuote((tmp / "thunks.s45").string()) +
                  " -o " + shellQuote((tmp / "thunks.o45").string()), "assembling the thunks"))
            return 1;
        objects.push_back((tmp / "thunks.o45").string());
        for (size_t i = 0; i < opts.linkFiles.size(); i++) {
            const std::string& f = opts.linkFiles[i];
            std::string ext = fs::path(f).extension().string();
            std::string o = (tmp / ("link" + std::to_string(i) + ".o45")).string();
            if (ext == ".c") {
                if (!step(shellQuote(tools + "/cc45") + " -c -O2 " + shellQuote(f) + " -o " + shellQuote(o),
                          "compiling " + f)) return 1;
            } else if (ext == ".s45" || ext == ".s" || ext == ".asm") {
                if (!step(shellQuote(tools + "/ca45") + " -c " + shellQuote(f) + " -o " + shellQuote(o),
                          "assembling " + f)) return 1;
            } else {
                o = f;
            }
            objects.push_back(o);
        }
        std::string lib = opts.linkLib;
        if (lib.empty()) {
            for (const char* rel : {"/../lib/build/c45.lib", "/../lib/cc45/c45.lib"})
                if (fs::exists(tools + rel)) { lib = tools + rel; break; }
            if (lib.empty()) lib = "c45.lib";   // ln45 searches CC45_LIB
        }
        char base[8];
        snprintf(base, sizeof base, "%04X", codeBase);
        std::string codePrg = (tmp / "code.prg").string();
        std::string cmd = shellQuote(tools + "/ln45") + " -prg -m -t " + base + " -o " + shellQuote(codePrg);
        for (const auto& o : objects) cmd += " " + shellQuote(o);
        cmd += " " + shellQuote(lib);
        if (!step(cmd, "linking")) return 1;

        // The symbol map: "  0000203a _name"
        SymbolTable linked;
        if (!opts.symbolFile.empty()) linked.loadFromFile(opts.symbolFile);
        std::istringstream map(log);
        std::string mline;
        std::regex mapLine(R"(^\s+([0-9a-fA-F]{8})\s+(\S+)\s*$)");
        std::smatch mm;
        while (std::getline(map, mline))
            if (std::regex_match(mline, mm, mapLine)) {
                std::string name = mm[2].str();
                unsigned long v = std::stoul(mm[1].str(), nullptr, 16);
                linked.addSymbol(name, (uint16_t)v);
                // {name} may leave out C's leading underscore
                if (name.size() > 1 && name[0] == '_' && !linked.has(name.substr(1)))
                    linked.addSymbol(name.substr(1), (uint16_t)v);
            }
        auto code = readBinary(codePrg);
        if (!getenv("BASIC45_KEEP_TMP")) fs::remove_all(tmp);
        if (code.size() < 2) { std::cerr << "basic45: the linked code is empty" << std::endl; return 1; }
        uint32_t imageBase = code[0] | (code[1] << 8);

        // Pass 2: the real addresses; the program can only get shorter
        result = build(linked);
        if (!result.ok) return 1;
        binary = result.binary;
        uint32_t end = opts.loadAddress + (uint32_t)binary.size() - 2;
        if (end > imageBase) {
            std::cerr << "basic45: the program ($" << std::hex << end << ") overlaps the linked code ($"
                      << imageBase << ")" << std::dec << std::endl;
            return 1;
        }
        binary.insert(binary.end(), imageBase - end, 0x00);
        binary.insert(binary.end(), code.begin() + 2, code.end());
        if (opts.verbose) {
            std::cout << "Linked code at $" << std::hex << imageBase << "-$"
                      << imageBase + code.size() - 3 << std::dec << std::endl;
            for (const auto& n : sysTargets)
                std::cout << "  SYS {" << n << "}: thunk __bas_" << n << std::endl;
        }
    }

    // Dry-run mode: report actions without writing files
    if (opts.dryRun) {
        DryRunMode dryRun;
        dryRun.reportAction("write", opts.outputFile + " (" + std::to_string(binary.size()) + " bytes)");
        if (!opts.labelTableFile.empty()) dryRun.reportAction("write", opts.labelTableFile);
        if (opts.generateDocs && !opts.docsFile.empty()) dryRun.reportAction("write", opts.docsFile);
        if (opts.generateSourceMap && !opts.sourceMapFile.empty()) dryRun.reportAction("write", opts.sourceMapFile);
        if (!opts.diskImage.empty()) dryRun.reportAction("add to", opts.diskImage);
        std::cout << dryRun.simulateCompile(opts.inputFile, result.text, binary.size());
        return 0;
    }

    // Write compiled output
    writeFile(opts.outputFile, binary);

    if (!opts.labelTableFile.empty()) {
        result.emitter->outputLabelTable(opts.labelTableFile);
        if (opts.verbose) {
            std::cout << "Label table written to: " << opts.labelTableFile << std::endl;
        }
    }

    // Source lines, read once per file (for --list and the source map)
    std::map<std::string, std::vector<std::string>> fileLines;
    auto sourceText = [&](const std::string& file, int line) -> std::string {
        auto it = fileLines.find(file);
        if (it == fileLines.end()) {
            std::vector<std::string> lines;
            std::ifstream in(file);
            std::string l;
            while (std::getline(in, l)) lines.push_back(l);
            it = fileLines.insert({file, lines}).first;
        }
        if (line < 1 || line > (int)it->second.size()) return "";
        std::string s = it->second[line - 1];
        size_t b = s.find_first_not_of(" \t");
        return b == std::string::npos ? "" : s.substr(b);
    };

    // The variables renamed to short names
    const auto& vars = result.emitter->getVariableMap();
    if ((opts.verbose || opts.listing) && !vars.empty()) {
        std::cout << "Variables:";
        for (const auto& [from, to] : vars) std::cout << " " << from << "->" << to;
        std::cout << std::endl;
    }

    if (opts.listing) {
        for (const auto& el : result.emitter->getEmittedLines()) {
            auto o = el.sourceLine ? originOf(result, el.sourceLine) : std::pair<std::string, int>{"", 0};
            char head[64];
            snprintf(head, sizeof head, "%5u  $%04X  ", el.lineNumber, el.address);
            std::string where = o.second ? fs::path(o.first).filename().string() + ":" + std::to_string(o.second) : "";
            std::cout << head;
            printf("%-16s", where.c_str());
            std::cout << "  " << (o.second ? sourceText(o.first, o.second) : ":") << std::endl;
        }
    }

    if (opts.generateDocs && !opts.docsFile.empty()) {
        BasicDocGenerator docGen;
        docGen.setLabelLines(result.validator->getParser().getLabelMap());
        std::string docs = docGen.generateMarkdown(originalSource, opts.inputFile);
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

    // Source map: each BASIC line and the source line it came from
    if (opts.generateSourceMap && !opts.sourceMapFile.empty()) {
        SourceMapGenerator mapGen;
        for (const auto& el : result.emitter->getEmittedLines()) {
            if (!el.sourceLine) continue;
            auto o = originOf(result, el.sourceLine);
            if (!o.second) continue;
            mapGen.addMapping({el.lineNumber, (uint16_t)o.second, o.first, sourceText(o.first, o.second)});
        }
        std::ofstream mapFile(opts.sourceMapFile);
        if (mapFile) {
            mapFile << mapGen.generateJSON();
            if (opts.verbose) {
                std::cout << "Source map written to: " << opts.sourceMapFile << std::endl;
            }
        } else {
            std::cerr << "Warning: Could not write source map file: " << opts.sourceMapFile << std::endl;
        }
    }

    // --disk: the program onto a disk image (disk45), created if missing
    if (!opts.diskImage.empty()) {
        std::string tools = toolDir(argv[0]);
        std::string disk45 = shellQuote(tools + "/disk45");
        std::string name = opts.diskName;
        if (name.empty()) name = fs::path(opts.outputFile).stem().string();
        if (name.size() > 16) name = name.substr(0, 16);
        std::string log;
        if (!fs::exists(opts.diskImage)) {
            std::string label = name;
            if (runCommand(disk45 + " create " + shellQuote(opts.diskImage) + " -n " + shellQuote(label), &log) != 0) {
                std::cerr << "basic45: could not create " << opts.diskImage << ":\n" << log;
                return 1;
            }
        }
        runCommand(disk45 + " remove " + shellQuote(opts.diskImage) + " " + shellQuote(name), &log);
        if (runCommand(disk45 + " add " + shellQuote(opts.diskImage) + " " + shellQuote(opts.outputFile) + " " +
                       shellQuote(name), &log) != 0) {
            std::cerr << "basic45: could not add the program to " << opts.diskImage << ":\n" << log;
            return 1;
        }
        if (opts.verbose) std::cout << "Added \"" << name << "\" to " << opts.diskImage << std::endl;
    }

    // Run it in xemu
    if (opts.launchEmulator) {
        EmulatorLauncher launcher;
        if (!launcher.launch(opts.outputFile, opts.emulatorPath, opts.diskImage)) {
            std::cerr << "Warning: Could not launch emulator" << std::endl;
        }
    }

    if (opts.verbose) {
        std::cout << "Generated " << binary.size() << " bytes" << std::endl;
    }

    return 0;
}
