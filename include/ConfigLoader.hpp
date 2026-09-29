#pragma once

#include <vector>
#include <string>

class ConfigLoader {
public:
    // Load configuration from ~/.config/m65/<programName>.conf
    // Returns vector of command-line tokens to prepend to argv
    // Returns empty vector if config file not found or cannot be read
    static std::vector<std::string> loadConfig(const std::string& programName);
};
