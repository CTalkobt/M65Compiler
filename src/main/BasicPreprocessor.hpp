#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>

struct MacroDefinition {
    std::string name;
    std::string value;
    std::vector<std::string> params;  // For parameterized macros
    bool isParameterized;
};

class BasicPreprocessor {
public:
    BasicPreprocessor(const std::string& basePath = ".");

    // Main preprocessing function
    std::string preprocess(const std::string& source, const std::string& filename = "<stdin>");

    // Set include search paths
    void addIncludePath(const std::string& path);

    // Get list of processed files (for dependency tracking)
    const std::vector<std::string>& getIncludedFiles() const { return includedFiles; }

    // Get macro definitions
    const std::map<std::string, MacroDefinition>& getMacros() const { return macros; }

private:
    std::string basePath;
    std::vector<std::string> includePaths;
    std::vector<std::string> includedFiles;  // Track included files
    std::map<std::string, MacroDefinition> macros;
    std::set<std::string> processedIncludes;  // Prevent circular includes

    struct ConditionalState {
        bool active;
        bool hasElse;
        bool hasSeenTrue;
    };

    std::vector<ConditionalState> conditionalStack;

    // Helper methods
    std::string processLine(const std::string& line, const std::string& baseDir);
    std::string expandMacros(const std::string& line);
    std::string findIncludeFile(const std::string& filename, const std::string& currentDir);
    std::string readFile(const std::string& filename);
    std::string preprocessInternal(const std::string& source, const std::string& baseDir, int depth = 0);

    bool isConditionalActive() const;
};
