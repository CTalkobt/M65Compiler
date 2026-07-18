#pragma once
#include "AsmIR.hpp"
#include <string>
#include <vector>
#include <set>
#include <regex>
#include <sstream>
#include <algorithm>

// Macro invocation detection and expansion
struct MacroInvocation {
    std::string macroName;
    std::vector<std::string> arguments;
    int sourceLine = 0;
};

// Macro extraction and expansion utilities
class MacroUtils {
public:
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
