#include "BasicPreprocessor.hpp"
#include <sstream>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <filesystem>

namespace fs = std::filesystem;

BasicPreprocessor::BasicPreprocessor(const std::string& base)
    : basePath(base) {
    addIncludePath(".");
    addIncludePath(base);
}

void BasicPreprocessor::addIncludePath(const std::string& path) {
    includePaths.push_back(path);
}

std::string BasicPreprocessor::preprocess(const std::string& source, const std::string& filename) {
    includedFiles.clear();
    macros.clear();
    conditionalStack.clear();
    processedIncludes.clear();

    std::string dir = basePath;
    if (filename != "<stdin>") {
        dir = fs::path(filename).parent_path().string();
        if (dir.empty()) dir = basePath;
    }

    return preprocessInternal(source, dir);
}

std::string BasicPreprocessor::preprocessInternal(const std::string& source, const std::string& baseDir, int depth) {
    if (depth > 100) {
        throw std::runtime_error("Preprocessor: Maximum include depth exceeded (circular includes?)");
    }

    std::istringstream iss(source);
    std::string line;
    std::ostringstream result;
    int lineNum = 0;

    while (std::getline(iss, line)) {
        lineNum++;

        // Remove leading/trailing whitespace
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) {
            // Empty line
            if (isConditionalActive()) {
                result << "\n";
            }
            continue;
        }

        std::string trimmedLine = line.substr(start);

        // Check for preprocessor directives
        if (trimmedLine[0] == '#') {
            std::istringstream directive(trimmedLine.substr(1));
            std::string cmd;
            directive >> cmd;

            if (cmd == "include") {
                if (!isConditionalActive()) continue;

                std::string filename;
                char quote;
                directive >> quote >> filename;

                // Remove closing quote
                if (!filename.empty() && filename.back() == quote) {
                    filename.pop_back();
                }

                std::string includeFile = findIncludeFile(filename, baseDir);
                if (includeFile.empty()) {
                    throw std::runtime_error("Preprocessor: Cannot find include file: " + filename);
                }

                if (processedIncludes.count(includeFile) == 0) {
                    processedIncludes.insert(includeFile);
                    includedFiles.push_back(includeFile);

                    std::string fileContents = readFile(includeFile);
                    std::string dir = fs::path(includeFile).parent_path().string();
                    if (dir.empty()) dir = baseDir;

                    std::string processed = preprocessInternal(fileContents, dir, depth + 1);
                    result << processed;
                    if (!processed.empty() && processed.back() != '\n') {
                        result << "\n";
                    }
                }
            } else if (cmd == "define") {
                if (!isConditionalActive()) continue;

                std::string name;
                directive >> name;

                std::string value;
                std::getline(directive, value);

                // Trim leading space
                if (!value.empty() && value[0] == ' ') {
                    value = value.substr(1);
                }

                MacroDefinition macro;
                macro.name = name;
                macro.value = value;
                macro.isParameterized = false;

                macros[name] = macro;
            } else if (cmd == "ifdef") {
                std::string symbol;
                directive >> symbol;

                ConditionalState state;
                state.active = macros.count(symbol) > 0;
                state.hasElse = false;
                state.hasSeenTrue = state.active;

                if (!conditionalStack.empty() && !isConditionalActive()) {
                    state.active = false;
                }

                conditionalStack.push_back(state);
            } else if (cmd == "ifndef") {
                std::string symbol;
                directive >> symbol;

                ConditionalState state;
                state.active = macros.count(symbol) == 0;
                state.hasElse = false;
                state.hasSeenTrue = state.active;

                if (!conditionalStack.empty() && !isConditionalActive()) {
                    state.active = false;
                }

                conditionalStack.push_back(state);
            } else if (cmd == "else") {
                if (conditionalStack.empty()) {
                    throw std::runtime_error("Preprocessor: #else without #ifdef/#ifndef");
                }

                ConditionalState& state = conditionalStack.back();
                if (state.hasElse) {
                    throw std::runtime_error("Preprocessor: Multiple #else for same #ifdef");
                }

                state.hasElse = true;
                bool parentActive = conditionalStack.size() == 1 ||
                    std::all_of(conditionalStack.begin(), conditionalStack.end() - 1,
                        [](const ConditionalState& s) { return s.active; });

                state.active = parentActive && !state.hasSeenTrue;
            } else if (cmd == "endif") {
                if (conditionalStack.empty()) {
                    throw std::runtime_error("Preprocessor: #endif without #ifdef/#ifndef");
                }

                conditionalStack.pop_back();
            } else if (cmd == "undef") {
                if (!isConditionalActive()) continue;

                std::string symbol;
                directive >> symbol;
                macros.erase(symbol);
            }

            // Don't output directive lines
            continue;
        }

        if (isConditionalActive()) {
            // Expand macros and output line
            std::string expanded = expandMacros(trimmedLine);
            result << expanded << "\n";
        }
    }

    if (!conditionalStack.empty()) {
        throw std::runtime_error("Preprocessor: Unclosed #ifdef/#ifndef");
    }

    return result.str();
}

std::string BasicPreprocessor::expandMacros(const std::string& line) {
    std::string result = line;

    for (const auto& [name, macro] : macros) {
        // Simple text substitution (not handling parameterized macros yet)
        size_t pos = 0;
        while ((pos = result.find(name, pos)) != std::string::npos) {
            // Check word boundaries
            bool validBefore = (pos == 0 || !std::isalnum(result[pos - 1]));
            bool validAfter = (pos + name.length() >= result.length() ||
                             !std::isalnum(result[pos + name.length()]));

            if (validBefore && validAfter) {
                result.replace(pos, name.length(), macro.value);
                pos += macro.value.length();
            } else {
                pos++;
            }
        }
    }

    return result;
}

std::string BasicPreprocessor::findIncludeFile(const std::string& filename, const std::string& currentDir) {
    // First try relative to current directory
    std::string fullPath = (fs::path(currentDir) / filename).string();
    if (fs::exists(fullPath)) {
        return fullPath;
    }

    // Then try include paths
    for (const auto& includePath : includePaths) {
        fullPath = (fs::path(includePath) / filename).string();
        if (fs::exists(fullPath)) {
            return fullPath;
        }
    }

    return "";
}

std::string BasicPreprocessor::readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        throw std::runtime_error("Preprocessor: Cannot open file: " + filename);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool BasicPreprocessor::isConditionalActive() const {
    if (conditionalStack.empty()) {
        return true;
    }

    for (const auto& state : conditionalStack) {
        if (!state.active) {
            return false;
        }
    }

    return true;
}
