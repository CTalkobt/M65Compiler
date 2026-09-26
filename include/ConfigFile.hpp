#pragma once
#include <string>
#include <map>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>

// Phase 5a: Configuration file support for .cvtasmrc
class ConfigFile {
public:
    struct Settings {
        // General settings
        std::string default_format = "ca45";
        int default_threads = 0;  // 0 = auto-detect
        std::string output_dir = "";
        bool validate_round_trip = false;
        bool clear_cache_per_batch = false;
        bool parallel_default = false;
        bool metrics_default = false;

        // Paths
        std::vector<std::string> library_paths;

        // Performance
        bool enable_profiling = false;
        bool show_timing_breakdown = false;
    };

private:
    Settings settings;
    std::string configPath;
    bool loaded = false;

    std::string trimString(const std::string& str) {
        size_t start = str.find_first_not_of(" \t\r\n");
        size_t end = str.find_last_not_of(" \t\r\n");
        return (start == std::string::npos) ? "" : str.substr(start, end - start + 1);
    }

    std::string toLower(const std::string& str) {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }

public:
    ConfigFile() {
        // Default library paths
        settings.library_paths.push_back(".");
        settings.library_paths.push_back("./include");
        settings.library_paths.push_back("./macros");
    }

    // Find and load config file
    bool load() {
        // Search for .cvtasmrc in current directory and parent directories
        std::filesystem::path searchPath = std::filesystem::current_path();

        while (true) {
            std::filesystem::path configCandidate = searchPath / ".cvtasmrc";

            if (std::filesystem::exists(configCandidate)) {
                return loadFile(configCandidate.string());
            }

            // Move to parent directory
            if (searchPath.has_parent_path() && searchPath.parent_path() != searchPath) {
                searchPath = searchPath.parent_path();
            } else {
                // Reached filesystem root, config file not found (this is okay)
                loaded = true;
                return true;
            }
        }
    }

    // Load config from specific file
    bool loadFile(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            return false;
        }

        configPath = path;
        std::string line;
        std::string currentSection;

        while (std::getline(file, line)) {
            line = trimString(line);

            // Skip empty lines and comments
            if (line.empty() || line[0] == ';' || line[0] == '#') {
                continue;
            }

            // Parse section headers [section]
            if (line[0] == '[' && line.back() == ']') {
                currentSection = toLower(line.substr(1, line.length() - 2));
                currentSection = trimString(currentSection);
                continue;
            }

            // Parse key=value pairs
            size_t eqPos = line.find('=');
            if (eqPos != std::string::npos) {
                std::string key = toLower(trimString(line.substr(0, eqPos)));
                std::string value = trimString(line.substr(eqPos + 1));

                parseSetting(currentSection, key, value);
            }
        }

        loaded = true;
        return true;
    }

    void parseSetting(const std::string& section, const std::string& key, const std::string& value) {
        if (section == "general") {
            if (key == "default_format") {
                settings.default_format = value;
            } else if (key == "default_threads") {
                settings.default_threads = std::stoi(value);
            } else if (key == "output_dir") {
                settings.output_dir = value;
            } else if (key == "validate_round_trip") {
                settings.validate_round_trip = (value == "true" || value == "1" || value == "yes");
            } else if (key == "clear_cache_per_batch") {
                settings.clear_cache_per_batch = (value == "true" || value == "1" || value == "yes");
            } else if (key == "parallel_default") {
                settings.parallel_default = (value == "true" || value == "1" || value == "yes");
            } else if (key == "metrics_default") {
                settings.metrics_default = (value == "true" || value == "1" || value == "yes");
            }
        } else if (section == "paths") {
            if (key == "library_paths") {
                // Parse colon-separated paths
                settings.library_paths.clear();
                std::istringstream iss(value);
                std::string path;
                while (std::getline(iss, path, ':')) {
                    path = trimString(path);
                    if (!path.empty()) {
                        settings.library_paths.push_back(path);
                    }
                }
            }
        } else if (section == "performance") {
            if (key == "enable_profiling") {
                settings.enable_profiling = (value == "true" || value == "1" || value == "yes");
            } else if (key == "show_timing_breakdown") {
                settings.show_timing_breakdown = (value == "true" || value == "1" || value == "yes");
            }
        }
    }

    // Getters
    const Settings& getSettings() const { return settings; }
    bool isLoaded() const { return loaded; }
    const std::string& getConfigPath() const { return configPath; }

    // Write default config file
    static bool writeDefault(const std::string& path = ".cvtasmrc") {
        std::ofstream file(path);
        if (!file.is_open()) {
            return false;
        }

        file << "; cvt_asm configuration file (.cvtasmrc)\n";
        file << "; Place this file in your project root or any parent directory\n\n";

        file << "[general]\n";
        file << "; Default target format (ca45, ca65, acme, oscar, merlin64, x65, kickassembler)\n";
        file << "default_format = ca45\n\n";

        file << "; Number of worker threads (0 = auto-detect CPU cores)\n";
        file << "default_threads = 0\n\n";

        file << "; Default output directory\n";
        file << "output_dir = \n\n";

        file << "; Validate round-trip conversion (may slow down batch)\n";
        file << "validate_round_trip = false\n\n";

        file << "; Clear include cache between batch files\n";
        file << "clear_cache_per_batch = false\n\n";

        file << "; Enable parallel processing by default\n";
        file << "parallel_default = false\n\n";

        file << "; Show metrics by default\n";
        file << "metrics_default = false\n\n";

        file << "[paths]\n";
        file << "; Colon-separated library paths for .include resolution\n";
        file << "library_paths = .:./include:./macros\n\n";

        file << "[performance]\n";
        file << "; Enable detailed performance profiling\n";
        file << "enable_profiling = false\n\n";

        file << "; Show per-file timing breakdown\n";
        file << "show_timing_breakdown = false\n";

        return true;
    }

    // Print loaded configuration
    void printConfig() const {
        if (!loaded) {
            std::cerr << "No configuration file loaded.\n";
            return;
        }

        std::cout << "Configuration loaded from: " << configPath << "\n\n";
        std::cout << "[general]\n";
        std::cout << "  default_format: " << settings.default_format << "\n";
        std::cout << "  default_threads: " << (settings.default_threads > 0 ? std::to_string(settings.default_threads) : "auto") << "\n";
        std::cout << "  output_dir: " << (settings.output_dir.empty() ? "(none)" : settings.output_dir) << "\n";
        std::cout << "  validate_round_trip: " << (settings.validate_round_trip ? "yes" : "no") << "\n";
        std::cout << "  parallel_default: " << (settings.parallel_default ? "yes" : "no") << "\n";
        std::cout << "  metrics_default: " << (settings.metrics_default ? "yes" : "no") << "\n\n";

        std::cout << "[paths]\n";
        std::cout << "  library_paths: ";
        for (size_t i = 0; i < settings.library_paths.size(); ++i) {
            if (i > 0) std::cout << ":";
            std::cout << settings.library_paths[i];
        }
        std::cout << "\n\n";

        std::cout << "[performance]\n";
        std::cout << "  enable_profiling: " << (settings.enable_profiling ? "yes" : "no") << "\n";
        std::cout << "  show_timing_breakdown: " << (settings.show_timing_breakdown ? "yes" : "no") << "\n";
    }
};
