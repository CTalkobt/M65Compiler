#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_map>

enum class Severity { Error, Warning, Note };

// Load source lines from file (caches for reuse)
inline std::vector<std::string> loadSourceFile(const std::string& filename) {
    static std::unordered_map<std::string, std::vector<std::string>> cache;

    if (cache.find(filename) != cache.end()) {
        return cache[filename];
    }

    std::vector<std::string> lines;
    std::ifstream file(filename);
    if (!file.is_open()) {
        return lines;  // Return empty if file not found
    }

    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }

    cache[filename] = lines;
    return lines;
}

// Add suggestions based on error message patterns
inline void addErrorSuggestions(std::string& output, const std::string& msg) {
    std::vector<std::string> suggestions;

    // Missing semicolon (strongest pattern)
    if (msg.find("Expected ';'") != std::string::npos) {
        suggestions.push_back("  • Did you forget a semicolon?");
        suggestions.push_back("  • Add ';' at end of the previous line");
    }

    // Missing closing parenthesis
    if (msg.find("Expected ')'") != std::string::npos) {
        suggestions.push_back("  • Missing closing parenthesis ')'");
        if (msg.find("OPEN_BRACE") != std::string::npos) {
            suggestions.push_back("  • Did you forget ')' before '{'?");
        }
    }

    // Missing closing brace
    if (msg.find("Expected '}'") != std::string::npos) {
        suggestions.push_back("  • Missing closing brace '}'");
        suggestions.push_back("  • Check for mismatched opening brace");
    }

    // Unexpected token - type keywords (suggests missing semicolon on previous line)
    if (msg.find("Found '") != std::string::npos) {
        if (msg.find("RETURN") != std::string::npos ||
            msg.find("INT") != std::string::npos ||
            msg.find("CHAR") != std::string::npos ||
            msg.find("VOID") != std::string::npos ||
            msg.find("FLOAT") != std::string::npos ||
            msg.find("DOUBLE") != std::string::npos) {
            suggestions.push_back("  • Check if a semicolon is missing on the previous line");
        }
    }

    // Assignment vs comparison
    if (msg.find("Expected '='") != std::string::npos) {
        suggestions.push_back("  • Did you mean '==' instead of '=' for comparison?");
    }

    // Type-related errors
    if (msg.find("Expected") != std::string::npos) {
        if (msg.find("type") != std::string::npos) {
            suggestions.push_back("  • Check type declaration or cast");
        }
        if (msg.find("return type") != std::string::npos) {
            suggestions.push_back("  • Specify return type explicitly (int, void, etc.)");
        }
        if (msg.find("parameter type") != std::string::npos) {
            suggestions.push_back("  • Function parameters need explicit types");
        }
    }

    // Struct/union undefined
    if (msg.find("Unknown struct") != std::string::npos ||
        msg.find("Unknown union") != std::string::npos) {
        suggestions.push_back("  • Did you forget to define the struct/union?");
        suggestions.push_back("  • Check spelling of struct/union name");
    }

    // Common function mistakes
    if (msg.find("Expected parameter type") != std::string::npos) {
        suggestions.push_back("  • Each parameter must have an explicit type");
        suggestions.push_back("  • Example: int func(int x, int y)");
    }

    // Undefined identifier
    if (msg.find("undefined") != std::string::npos) {
        suggestions.push_back("  • Check if variable/function is declared");
        suggestions.push_back("  • Check spelling");
    }

    if (!suggestions.empty()) {
        output += "\n\nSuggestions:\n";
        for (const auto& sugg : suggestions) {
            output += sugg + "\n";
        }
    }
}

// Enhanced formatDiagnostic with context and suggestions
inline std::string formatDiagnostic(const std::string& file, int line, int col,
                                     Severity sev, const std::string& msg) {
    const char* s = sev == Severity::Error ? "error" :
                    sev == Severity::Warning ? "warning" : "note";

    std::string header = file + ":" + std::to_string(line) + ":" +
                        std::to_string(col) + ": " + s + ": " + msg;

    // Load source context
    auto sourceLines = loadSourceFile(file);

    // Add context if available (show line before, the error line, and line after)
    if (!sourceLines.empty() && line > 0 && line <= (int)sourceLines.size()) {
        std::ostringstream context;
        context << header;

        int errorIdx = line - 1;  // Convert to 0-based
        int startIdx = std::max(0, errorIdx - 1);
        int endIdx = std::min((int)sourceLines.size() - 1, errorIdx + 1);

        for (int i = startIdx; i <= endIdx; i++) {
            int displayLine = i + 1;
            context << "\n " << displayLine << " | " << sourceLines[i];

            // Show error position on main error line
            if (i == errorIdx) {
                context << "\n   | ";
                for (int j = 0; j < col - 1 && j < (int)sourceLines[i].length(); j++) {
                    context << " ";
                }
                context << "^";
            }
        }

        std::string result = context.str();
        addErrorSuggestions(result, msg);
        return result;
    }

    // Fallback if file not found - add suggestions without context
    addErrorSuggestions(header, msg);
    return header;
}
