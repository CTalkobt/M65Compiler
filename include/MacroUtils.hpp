#pragma once
#include "AsmIR.hpp"
#include <string>
#include <vector>
#include <regex>

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

private:
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
