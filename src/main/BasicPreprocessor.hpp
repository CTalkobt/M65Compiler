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

    // Set current file and line (for __FILE__, __LINE__)
    void setCurrentFile(const std::string& filename) { currentFile = filename; }

    // Get list of processed files (for dependency tracking)
    const std::vector<std::string>& getIncludedFiles() const { return includedFiles; }

    // Where output line n (1-based) came from: (file, line); ("", 0) if unknown
    std::pair<std::string, int> getLineOrigin(int outputLine) const {
        if (outputLine < 1 || outputLine > (int)lineOrigins.size()) return {"", 0};
        return lineOrigins[outputLine - 1];
    }

    // Get macro definitions
    const std::map<std::string, MacroDefinition>& getMacros() const { return macros; }

private:
    std::string basePath;
    std::string currentFile;  // For __FILE__ macro
    int currentLine = 0;      // For __LINE__ macro
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
    std::string expandPredefinedMacros(const std::string& line);
    std::string findIncludeFile(const std::string& filename, const std::string& currentDir);
    std::string readFile(const std::string& filename);
    std::string preprocessInternal(const std::string& source, const std::string& baseDir, int depth = 0,
                                   const std::string& file = "");
    std::vector<std::pair<std::string, int>> lineOrigins;   // per output line

    bool isConditionalActive() const;
    std::string getCurrentDate() const;
    std::string getCurrentTime() const;
};
