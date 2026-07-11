#pragma once
#include <vector>
#include <string>

class ConfigLoader {
public:
    /// Load configuration file from ~/.config/m65/<program>.conf
    /// Returns a vector of tokens as if they were command-line arguments
    /// Lines starting with # are comments, blank lines are ignored
    /// Tokens are parsed shell-style (supports quoted strings with spaces)
    static std::vector<std::string> loadConfig(const std::string& programName);
};
