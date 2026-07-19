#pragma once
#include "AsmIR.hpp"
#include <string>
#include <vector>
#include <set>
#include <map>
#include <regex>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <mutex>
#include <chrono>

// Macro invocation detection and expansion
struct MacroInvocation {
    std::string macroName;
    std::vector<std::string> arguments;
    int sourceLine = 0;
};

// Conditional compilation block tracking
struct ConditionalBlock {
    enum Type { IF, IFDEF, IFNDEF, ELSE, ENDIF };
    Type type;
    std::string condition;  // For #if, the full condition expression
    int startLine = 0;
    int endLine = 0;
    bool isActive = true;   // Whether this block's content is included
};

// Conditional compilation state
struct ConditionalState {
    std::vector<bool> blockStack;      // Stack of conditional states
    std::set<std::string> definedSymbols;  // Symbols/macros that are defined
    int nestingLevel = 0;

    bool isActive() const {
        if (blockStack.empty()) return true;
        return blockStack.back();
    }

    void push(bool condition) {
        blockStack.push_back(condition && isActive());
        nestingLevel++;
    }

    void pop() {
        if (!blockStack.empty()) {
            blockStack.pop_back();
            nestingLevel--;
        }
    }

    void setElse() {
        if (!blockStack.empty()) {
            blockStack.back() = !blockStack.back();
        }
    }
};

// Phase 4: Include file cache for performance
struct IncludeFileCache {
    std::string content;
    std::string filepath;
    std::filesystem::file_time_type lastModified;
    int cacheHits = 0;

    bool isStale() const {
        try {
            auto currentMtime = std::filesystem::last_write_time(filepath);
            return currentMtime != lastModified;
        } catch (...) {
            return true;
        }
    }
};

// Phase 4: Performance metrics tracking
struct PerformanceMetrics {
    int totalFiles = 0;
    int processedFiles = 0;
    int cachedIncludes = 0;
    std::chrono::milliseconds totalTime{0};
    std::chrono::milliseconds parseTime{0};
    std::chrono::milliseconds expandTime{0};
    std::chrono::milliseconds writeTime{0};

    double averageTimePerFile() const {
        return (totalFiles > 0) ? totalTime.count() / static_cast<double>(totalFiles) : 0.0;
    }

    double cacheHitRate() const {
        return (processedFiles > 0) ? (cachedIncludes / static_cast<double>(processedFiles)) * 100.0 : 0.0;
    }
};

// Macro extraction and expansion utilities
class MacroUtils {
private:
    // Phase 4: Include file cache and metrics
    static std::map<std::string, IncludeFileCache>& getIncludeCache() {
        static std::map<std::string, IncludeFileCache> cache;
        return cache;
    }

    static std::mutex& getCacheMutex() {
        static std::mutex mutex;
        return mutex;
    }

    static PerformanceMetrics& getMetrics() {
        static PerformanceMetrics metrics;
        return metrics;
    }

public:
    // Phase 4: Cache management
    static void clearCache() {
        std::lock_guard<std::mutex> lock(getCacheMutex());
        getIncludeCache().clear();
    }

    static void resetMetrics() {
        getMetrics() = PerformanceMetrics();
    }

    static const PerformanceMetrics& getPerformanceMetrics() {
        return getMetrics();
    }

    static int getCacheHitCount() {
        return getMetrics().cachedIncludes;
    }

    // Extract macros from ca65 format source
    static void extractCa65Macros(const std::string& source, AsmIR::Module& module) {
        std::istringstream iss(source);
        std::string line;
        int lineNum = 0;
        bool inMacro = false;
        AsmIR::Macro currentMacro;

        while (std::getline(iss, line)) {
            lineNum++;
            std::string trimmed = trimLine(line);

            // Check for .macro definition
            if (trimmed.find(".macro") == 0) {
                inMacro = true;
                parseCa65MacroHeader(trimmed, currentMacro);
                currentMacro.definitionLine = lineNum;
                continue;
            }

            // Check for .endmacro
            if (trimmed.find(".endmacro") == 0) {
                if (inMacro && !currentMacro.name.empty()) {
                    module.macros[currentMacro.name] = currentMacro;
                }
                inMacro = false;
                currentMacro = AsmIR::Macro();
                continue;
            }

            // Collect macro body
            if (inMacro) {
                currentMacro.body.push_back(trimmed);
            }
        }
    }

    // Extract macros from ACME format source
    static void extractACMEMacros(const std::string& source, AsmIR::Module& module) {
        std::istringstream iss(source);
        std::string line;
        int lineNum = 0;
        bool inMacro = false;
        AsmIR::Macro currentMacro;

        while (std::getline(iss, line)) {
            lineNum++;
            std::string trimmed = trimLine(line);

            // Check for .macro or macro definition
            if (trimmed.find(".macro") == 0 || trimmed.find("macro") == 0) {
                inMacro = true;
                parseACMEMacroHeader(trimmed, currentMacro);
                currentMacro.definitionLine = lineNum;
                continue;
            }

            // Check for closing brace
            if (trimmed.find("}") != std::string::npos && inMacro) {
                if (!currentMacro.name.empty()) {
                    module.macros[currentMacro.name] = currentMacro;
                }
                inMacro = false;
                currentMacro = AsmIR::Macro();
                continue;
            }

            // Collect macro body
            if (inMacro) {
                currentMacro.body.push_back(trimmed);
            }
        }
    }

    // Extract macros from KickAssembler format source
    static void extractKickAssemblerMacros(const std::string& source, AsmIR::Module& module) {
        // Similar to ACME - uses .macro name { ... }
        extractACMEMacros(source, module);
    }

    // Format macro as ca65 output
    static std::string formatCa65Macro(const AsmIR::Macro& macro) {
        std::string result = ".macro " + macro.name;

        for (const auto& param : macro.parameters) {
            result += " " + param;
        }
        result += "\n";

        for (const auto& line : macro.body) {
            result += "  " + line + "\n";
        }

        result += ".endmacro\n";
        return result;
    }

    // Format macro as ACME output
    static std::string formatACMEMacro(const AsmIR::Macro& macro) {
        std::string result = ".macro " + macro.name + " {\n";

        for (const auto& line : macro.body) {
            result += "  " + line + "\n";
        }

        result += "}\n";
        return result;
    }

    // Expand macro invocation (for formats without native macro support)
    static std::vector<std::string> expandMacro(
        const AsmIR::Macro& macro,
        const std::vector<std::string>& arguments) {
        std::vector<std::string> expanded;

        // Simple parameter substitution
        for (const auto& line : macro.body) {
            std::string expandedLine = line;

            // Replace parameters with arguments (\1, \2, etc. or direct names)
            for (size_t i = 0; i < macro.parameters.size() && i < arguments.size(); ++i) {
                // Replace \1, \2, etc. style parameters
                std::string placeholder = "\\" + std::to_string(i + 1);
                size_t pos = 0;
                while ((pos = expandedLine.find(placeholder, pos)) != std::string::npos) {
                    expandedLine.replace(pos, placeholder.length(), arguments[i]);
                    pos += arguments[i].length();
                }

                // Replace named parameters
                size_t pos2 = 0;
                while ((pos2 = expandedLine.find(macro.parameters[i], pos2)) != std::string::npos) {
                    expandedLine.replace(pos2, macro.parameters[i].length(), arguments[i]);
                    pos2 += arguments[i].length();
                }
            }

            expanded.push_back(expandedLine);
        }

        return expanded;
    }

    // Detect macro invocation in a line of code with full argument parsing
    static bool detectMacroInvocation(const std::string& line,
                                      const AsmIR::Module& module,
                                      MacroInvocation& invocation,
                                      std::string& errorMsg) {
        std::string trimmed = trimLine(line);
        if (trimmed.empty()) return false;

        // Try each macro to see if this line invokes it
        for (const auto& [macroName, macro] : module.macros) {
            // Check if line starts with macro name (instruction-like position)
            if (trimmed.find(macroName) != 0) continue;

            // Ensure it's a complete word match (not part of longer identifier)
            size_t macroEnd = macroName.length();
            if (macroEnd < trimmed.length()) {
                char nextChar = trimmed[macroEnd];
                if (std::isalnum(nextChar) || nextChar == '_') continue;  // Not a match
            }

            invocation.macroName = macroName;
            invocation.sourceLine = 0;  // Would be set by caller

            // Extract argument portion (everything after macro name)
            std::string argPortion = trimmed.substr(macroEnd);
            argPortion = trimLine(argPortion);

            // Parse arguments with validation
            invocation.arguments = parseArgumentsWithValidation(argPortion, macro, errorMsg);

            // Validate argument count
            if (invocation.arguments.size() != macro.parameters.size()) {
                errorMsg = "Macro '" + macroName + "' expects " +
                          std::to_string(macro.parameters.size()) + " arguments, got " +
                          std::to_string(invocation.arguments.size());
                return false;
            }

            return true;
        }

        return false;
    }

    // Overload for backwards compatibility
    static bool detectMacroInvocation(const std::string& line,
                                      const AsmIR::Module& module,
                                      MacroInvocation& invocation) {
        std::string dummyError;
        return detectMacroInvocation(line, module, invocation, dummyError);
    }

    // Process macro invocations in module, expanding for target format
    static void processAndExpandMacros(AsmIR::Module& module,
                                       const std::string& targetFormat,
                                       bool shouldExpand = false) {
        // Formats that don't support macros natively
        static const std::set<std::string> noMacroFormats = {
            "oscar", "merlin64", "x65"  // These formats need macro expansion
        };

        // If target format doesn't support macros, expand all invocations
        if (noMacroFormats.count(targetFormat) > 0) {
            shouldExpand = true;
        }

        if (!shouldExpand) return;  // Format supports macros, no expansion needed

        // Scan statements for macro invocations and expand them
        std::vector<AsmIR::Statement> expandedStatements;

        for (const auto& stmt : module.statements) {
            if (stmt.type == AsmIR::Statement::Type::INSTRUCTION) {
                // Check if this looks like a macro invocation
                auto macroIt = module.macros.find(stmt.instr.mnemonic);
                if (macroIt != module.macros.end()) {
                    const auto& macro = macroIt->second;

                    // PHASE 3a: Parse arguments from operand
                    std::string operandText = stmt.instr.operand.text;
                    std::string errorMsg;
                    auto args = parseArgumentsWithValidation(operandText, macro, errorMsg);

                    // Validate argument count
                    if (!errorMsg.empty() || args.size() != macro.parameters.size()) {
                        // Add error comment instead of expanding
                        AsmIR::Statement errStmt;
                        errStmt.type = AsmIR::Statement::Type::COMMENT;
                        if (errorMsg.empty()) {
                            errStmt.comment = "; ERROR: macro '" + macro.name + "' expects " +
                                            std::to_string(macro.parameters.size()) + " args, got " +
                                            std::to_string(args.size());
                        } else {
                            errStmt.comment = "; ERROR: " + errorMsg;
                        }
                        expandedStatements.push_back(errStmt);
                        continue;
                    }

                    // Expand with proper argument substitution
                    auto expanded = expandMacroWithArguments(macro, args);

                    // Convert expanded lines to statements
                    for (const auto& line : expanded) {
                        AsmIR::Statement newStmt;
                        newStmt.type = AsmIR::Statement::Type::COMMENT;
                        newStmt.comment = line + " ; (from macro " + macro.name + ")";
                        expandedStatements.push_back(newStmt);
                    }
                    continue;
                }
            }

            // Not a macro invocation, keep as-is
            expandedStatements.push_back(stmt);
        }

        if (shouldExpand && expandedStatements.size() > module.statements.size()) {
            module.statements = expandedStatements;
        }
    }

    // Parse macro arguments with proper handling of quoted strings and nested parens
    static std::vector<std::string> parseArgumentsWithValidation(
        const std::string& argString,
        const AsmIR::Macro& macro,
        std::string& errorMsg) {
        std::vector<std::string> args;

        if (argString.empty()) {
            return args;  // No arguments
        }

        bool inQuotes = false;
        int inParens = 0;  // Track nesting depth
        std::string current;

        for (size_t i = 0; i < argString.length(); ++i) {
            char c = argString[i];

            // Handle quoted strings
            if (c == '"' || c == '\'') {
                inQuotes = !inQuotes;
                current += c;
                continue;
            }

            // Skip processing if in quotes
            if (inQuotes) {
                current += c;
                continue;
            }

            // Track parentheses nesting
            if (c == '(') {
                inParens++;
                current += c;
            } else if (c == ')') {
                inParens--;
                current += c;
            }
            // Split on comma or space (only if not in parens or quotes)
            else if ((c == ',' || std::isspace(c)) && inParens == 0) {
                std::string trimmed = current;
                trimmed.erase(0, trimmed.find_first_not_of(" \t"));
                trimmed.erase(trimmed.find_last_not_of(" \t") + 1);
                if (!trimmed.empty()) {
                    args.push_back(trimmed);
                }
                current.clear();
            } else {
                current += c;
            }
        }

        // Add last argument
        if (!current.empty()) {
            std::string trimmed = current;
            trimmed.erase(0, trimmed.find_first_not_of(" \t"));
            trimmed.erase(trimmed.find_last_not_of(" \t") + 1);
            if (!trimmed.empty()) {
                args.push_back(trimmed);
            }
        }

        // Validate we didn't end in an incomplete quote or paren
        if (inQuotes) {
            errorMsg = "Unclosed quote in macro arguments";
            return {};
        }
        if (inParens != 0) {
            errorMsg = "Unmatched parentheses in macro arguments";
            return {};
        }

        return args;
    }

    // Phase 3c: Library path management for .include directives
    static std::vector<std::string>& getLibraryPaths() {
        static std::vector<std::string> libraryPaths = {
            ".",           // Current directory
            "./include",   // Typical include directory
            "./macros",    // Macro library directory
        };
        return libraryPaths;
    }

    static void addLibraryPath(const std::string& path) {
        getLibraryPaths().push_back(path);
    }

    static void setLibraryPaths(const std::vector<std::string>& paths) {
        getLibraryPaths() = paths;
    }

    // Find include file in library paths
    static std::string findIncludeFile(const std::string& filename) {
        // Try as absolute or relative path first
        if (std::filesystem::exists(filename)) {
            return std::filesystem::absolute(filename).string();
        }

        // Search in library paths
        for (const auto& libPath : getLibraryPaths()) {
            std::string fullPath = libPath + "/" + filename;
            if (std::filesystem::exists(fullPath)) {
                return std::filesystem::absolute(fullPath).string();
            }
        }

        // Not found
        return "";
    }

    // Read include file with error handling and caching (Phase 4)
    static bool readIncludeFile(const std::string& filename, std::string& content, std::string& error) {
        std::string fullPath = findIncludeFile(filename);
        if (fullPath.empty()) {
            error = "Include file not found: " + filename;
            return false;
        }

        // Phase 4: Check cache first
        {
            std::lock_guard<std::mutex> lock(getCacheMutex());
            auto& cache = getIncludeCache();
            if (cache.count(fullPath) > 0) {
                auto& cached = cache[fullPath];
                // Verify cache is still valid (not stale)
                if (!cached.isStale()) {
                    content = cached.content;
                    cached.cacheHits++;
                    getMetrics().cachedIncludes++;
                    return true;
                } else {
                    // Cache is stale, remove it
                    cache.erase(fullPath);
                }
            }
        }

        // Read from file
        std::ifstream file(fullPath);
        if (!file.is_open()) {
            error = "Cannot open include file: " + filename;
            return false;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        content = buffer.str();

        // Phase 4: Store in cache
        {
            std::lock_guard<std::mutex> lock(getCacheMutex());
            auto& cache = getIncludeCache();
            IncludeFileCache cacheEntry;
            cacheEntry.content = content;
            cacheEntry.filepath = fullPath;
            try {
                cacheEntry.lastModified = std::filesystem::last_write_time(fullPath);
            } catch (...) {
                // If we can't get mtime, cache without expiration check
            }
            cache[fullPath] = cacheEntry;
        }

        return true;
    }

    // Process .include directives with circular dependency detection
    static std::string processIncludes(const std::string& source, AsmIR::Module& module,
                                       std::set<std::string>& processedFiles,
                                       std::vector<std::string>& errors) {
        std::istringstream iss(source);
        std::stringstream oss;
        std::string line;

        while (std::getline(iss, line)) {
            std::string trimmed = trimLine(line);

            // Check for .include directive
            if (trimmed.find(".include") == 0 || trimmed.find("#include") == 0) {
                std::string directive = trimmed.substr(trimmed.find(".include") == 0 ? 8 : 8);
                directive = trimLine(directive);

                // Parse filename from "filename" or <filename>
                std::string filename;
                if (directive.length() >= 2) {
                    if ((directive[0] == '"' && directive.back() == '"') ||
                        (directive[0] == '<' && directive.back() == '>')) {
                        filename = directive.substr(1, directive.length() - 2);
                    }
                }

                if (!filename.empty()) {
                    std::string fullPath = findIncludeFile(filename);
                    if (fullPath.empty()) {
                        errors.push_back("Include file not found: " + filename);
                        continue;
                    }

                    // Detect circular dependencies
                    if (processedFiles.count(fullPath) > 0) {
                        errors.push_back("Circular include detected: " + filename);
                        continue;
                    }

                    // Read and process included file
                    std::string includeContent;
                    std::string error;
                    if (!readIncludeFile(filename, includeContent, error)) {
                        errors.push_back(error);
                        continue;
                    }

                    // Mark file as processed
                    processedFiles.insert(fullPath);

                    // Recursively process includes in the included file
                    std::string processedInclude = processIncludes(includeContent, module, processedFiles, errors);

                    // Output the processed content with a comment indicating the source
                    oss << "; --- Included from: " << filename << " ---\n";
                    oss << processedInclude;
                    oss << "; --- End include: " << filename << " ---\n";
                } else {
                    errors.push_back("Invalid include directive: " + trimmed);
                }
                continue;
            }

            // Pass through other lines unchanged
            oss << line << "\n";
        }

        return oss.str();
    }

    // Phase 3b: Evaluate conditional directive
    static bool evaluateCondition(const std::string& condition, const ConditionalState& state) {
        std::string trimmed = trimLine(condition);

        // Handle #ifdef SYMBOL
        if (trimmed.find("ifdef") == 0) {
            std::string symbol = trimmed.substr(5);
            symbol = trimLine(symbol);
            return state.definedSymbols.count(symbol) > 0;
        }

        // Handle #ifndef SYMBOL
        if (trimmed.find("ifndef") == 0) {
            std::string symbol = trimmed.substr(6);
            symbol = trimLine(symbol);
            return state.definedSymbols.count(symbol) == 0;
        }

        // Handle #if EXPRESSION (simple evaluation)
        if (trimmed.find("if") == 0) {
            std::string expr = trimmed.substr(2);
            expr = trimLine(expr);
            return evaluateExpression(expr, state);
        }

        return false;
    }

    // Evaluate simple conditional expressions
    static bool evaluateExpression(const std::string& expr, const ConditionalState& state) {
        std::string trimmed = trimLine(expr);

        // Handle defined(SYMBOL)
        std::regex definedRegex(R"(defined\s*\(\s*(\w+)\s*\))");
        std::smatch match;
        if (std::regex_search(trimmed, match, definedRegex)) {
            std::string symbol = match[1];
            return state.definedSymbols.count(symbol) > 0;
        }

        // Handle !defined(SYMBOL)
        std::regex notDefinedRegex(R"(!\s*defined\s*\(\s*(\w+)\s*\))");
        if (std::regex_search(trimmed, match, notDefinedRegex)) {
            std::string symbol = match[1];
            return state.definedSymbols.count(symbol) == 0;
        }

        // Simple constant evaluation (1 = true, 0 = false)
        if (trimmed == "1" || trimmed == "true") return true;
        if (trimmed == "0" || trimmed == "false") return false;

        // Default to true if we can't evaluate
        return true;
    }

    // Process conditional compilation in source
    static std::string processConditionals(const std::string& source, AsmIR::Module& module) {
        std::istringstream iss(source);
        std::stringstream oss;
        std::string line;
        ConditionalState state;

        // Populate defined symbols from module macros
        for (const auto& [name, macro] : module.macros) {
            state.definedSymbols.insert(name);
        }

        // Populate defined symbols from module symbols
        for (const auto& [name, sym] : module.symbols) {
            if (sym.is_global) {
                state.definedSymbols.insert(name);
            }
        }

        while (std::getline(iss, line)) {
            std::string trimmed = trimLine(line);

            // Check for conditional directives
            if (trimmed.find("#ifdef") == 0) {
                std::string symbol = trimmed.substr(6);
                symbol = trimLine(symbol);
                state.push(state.definedSymbols.count(symbol) > 0);
                continue;
            }

            if (trimmed.find("#ifndef") == 0) {
                std::string symbol = trimmed.substr(7);
                symbol = trimLine(symbol);
                state.push(state.definedSymbols.count(symbol) == 0);
                continue;
            }

            if (trimmed.find("#if") == 0) {
                std::string condition = trimmed.substr(3);
                condition = trimLine(condition);
                bool result = evaluateExpression(condition, state);
                state.push(result);
                continue;
            }

            if (trimmed.find("#else") == 0) {
                state.setElse();
                continue;
            }

            if (trimmed.find("#endif") == 0) {
                state.pop();
                continue;
            }

            // Output line if we're in an active block
            if (state.isActive()) {
                oss << line << "\n";
            }
        }

        return oss.str();
    }

    // Expand macro with full argument substitution
    static std::vector<std::string> expandMacroWithArguments(
        const AsmIR::Macro& macro,
        const std::vector<std::string>& arguments) {
        std::vector<std::string> expanded;

        // Validate argument count
        if (arguments.size() != macro.parameters.size()) {
            // Return body with error comment
            expanded.push_back("; ERROR: macro argument count mismatch");
            for (const auto& line : macro.body) {
                expanded.push_back(line);
            }
            return expanded;
        }

        // Expand with parameter substitution
        for (const auto& line : macro.body) {
            std::string expandedLine = line;

            // Substitute each parameter
            for (size_t i = 0; i < macro.parameters.size(); ++i) {
                const std::string& paramName = macro.parameters[i];
                const std::string& argValue = arguments[i];

                // Replace parameter name (word boundary check)
                size_t pos = 0;
                while ((pos = expandedLine.find(paramName, pos)) != std::string::npos) {
                    // Check word boundaries
                    bool isWordStart = (pos == 0) || !std::isalnum(expandedLine[pos - 1]);
                    bool isWordEnd = (pos + paramName.length() >= expandedLine.length()) ||
                                    !std::isalnum(expandedLine[pos + paramName.length()]);

                    if (isWordStart && isWordEnd) {
                        expandedLine.replace(pos, paramName.length(), argValue);
                        pos += argValue.length();
                    } else {
                        pos += paramName.length();
                    }
                }

                // Also replace numbered placeholders (\1, \2, etc.)
                std::string placeholder = "\\" + std::to_string(i + 1);
                pos = 0;
                while ((pos = expandedLine.find(placeholder, pos)) != std::string::npos) {
                    expandedLine.replace(pos, placeholder.length(), argValue);
                    pos += argValue.length();
                }
            }

            expanded.push_back(expandedLine);
        }

        return expanded;
    }

private:
    static std::vector<std::string> parseArguments(const std::string& argString) {
        std::vector<std::string> args;
        std::string current;

        for (char c : argString) {
            if (c == ',' || c == ' ') {
                std::string trimmed = current;
                trimmed.erase(0, trimmed.find_first_not_of(" \t"));
                trimmed.erase(trimmed.find_last_not_of(" \t") + 1);
                if (!trimmed.empty()) {
                    args.push_back(trimmed);
                }
                current.clear();
            } else {
                current += c;
            }
        }

        if (!current.empty()) {
            std::string trimmed = current;
            trimmed.erase(0, trimmed.find_first_not_of(" \t"));
            trimmed.erase(trimmed.find_last_not_of(" \t") + 1);
            if (!trimmed.empty()) {
                args.push_back(trimmed);
            }
        }

        return args;
    }
    static std::string trimLine(const std::string& line) {
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) return "";
        size_t end = line.find_last_not_of(" \t");
        return line.substr(start, end - start + 1);
    }

    static void parseCa65MacroHeader(const std::string& line, AsmIR::Macro& macro) {
        // Format: .macro name param1, param2, ...
        std::istringstream iss(line);
        std::string token;

        iss >> token;  // .macro
        iss >> macro.name;

        while (iss >> token) {
            // Remove trailing comma if present
            if (!token.empty() && token.back() == ',') {
                token.pop_back();
            }
            if (!token.empty()) {
                macro.parameters.push_back(token);
            }
        }
    }

    static void parseACMEMacroHeader(const std::string& line, AsmIR::Macro& macro) {
        // Format: .macro name { or macro name {
        size_t macroPos = line.find("macro");
        if (macroPos == std::string::npos) return;

        size_t bracePos = line.find('{');
        std::string header = line.substr(macroPos + 5,
                                        (bracePos != std::string::npos ? bracePos : line.length()) - (macroPos + 5));

        // Extract name (first non-whitespace token)
        std::istringstream iss(header);
        iss >> macro.name;
    }
};
