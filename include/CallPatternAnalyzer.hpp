#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

// Forward declarations
class TranslationUnit;

// Represents a single function call site (for pattern analysis)
struct CallAnalysisInfo {
    std::string callerId;          // Function that makes the call
    std::string calleeName;        // Function being called
    int argumentCount;             // Number of arguments
    bool hasConstantArgs;          // Whether any args are constants
    bool inLoop;                   // Is call inside a loop?
    int loopNestDepth;            // How deeply nested in loops
};

// Function-level information
struct FunctionInfo {
    std::string name;
    std::string module;
    int paramCount;
    int localVarCount;
    bool hasNoLocals;              // No local variables
    bool hasNoCalls;               // No function calls
    std::vector<CallAnalysisInfo> callSites;  // Where this function is called from
};

class CallPatternAnalyzer {
public:
    CallPatternAnalyzer() = default;

    // Analyze translation unit and extract function information
    void analyze(TranslationUnit& tu, const std::string& moduleName);

    // Get function info by name
    FunctionInfo* getFunctionInfo(const std::string& name);
    const FunctionInfo* getFunctionInfo(const std::string& name) const;

    // Get all functions
    std::unordered_map<std::string, FunctionInfo>& getAllFunctions();
    const std::unordered_map<std::string, FunctionInfo>& getAllFunctions() const;

    // Print analysis results
    void printResults() const;

private:
    std::unordered_map<std::string, FunctionInfo> functions_;
    std::string currentModule_;
};
