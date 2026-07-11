#include "ConfigLoader.hpp"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <iostream>

std::vector<std::string> ConfigLoader::loadConfig(const std::string& programName) {
    std::vector<std::string> tokens;

    // Construct config file path: ~/.config/m65/<program>.conf
    const char* homeDir = std::getenv("HOME");
    if (!homeDir) return tokens;  // No HOME, skip config

    std::string configPath = std::string(homeDir) + "/.config/m65/" + programName + ".conf";
    std::ifstream configFile(configPath);
    if (!configFile.is_open()) return tokens;  // No config file, skip

    std::string line;
    while (std::getline(configFile, line)) {
        // Skip comments and blank lines
        if (line.empty() || line[0] == '#') continue;

        // Remove leading/trailing whitespace
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;  // Blank line
        line = line.substr(start);

        // Simple shell-style tokenization
        std::istringstream iss(line);
        std::string token;
        bool inQuote = false;
        char quoteChar = 0;

        for (size_t i = 0; i < line.length(); ++i) {
            char c = line[i];

            if (!inQuote) {
                if (c == '"' || c == '\'') {
                    inQuote = true;
                    quoteChar = c;
                } else if (c == ' ' || c == '\t') {
                    if (!token.empty()) {
                        tokens.push_back(token);
                        token.clear();
                    }
                } else {
                    token += c;
                }
            } else {
                if (c == quoteChar) {
                    inQuote = false;
                } else {
                    token += c;
                }
            }
        }

        // Add final token
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }

    return tokens;
}
