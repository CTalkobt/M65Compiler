#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

// Represents a single argument pattern for a function call
struct ArgumentPattern {
    int paramIndex;                 // Which parameter (0-based)
    bool isConstant;               // Is this argument always constant?
    long constantValue;            // If constant, what value?
    std::string valueRange;        // If variable, what range? (e.g., "0-100", "loop_counter")
    int occurrences;               // How many times this pattern appears
};

// Represents all information about one call site
struct CallSite {
    std::string callerId;          // Function that makes the call (e.g., "main")
    int lineNumber;                // Source line number (if available)
    std::vector<ArgumentPattern> arguments;  // Patterns for each argument
    bool isLoopNested;             // Is this call inside a loop?
    int frequency;                 // Estimated call frequency (1=once, 2=loop, etc.)
};

// Complete profile for a single function
struct GlobalFunctionProfile {
    std::string name;              // Function name (e.g., "compute")
    std::string module;            // Source module (e.g., "file1.c")
    std::vector<CallSite> callSites;  // All places this function is called
    int codeSize;                  // Estimated size in bytes
    bool isLeaf;                   // No function calls (Phase 90 compatible)
    int totalCallSites;            // Total number of call sites
    std::vector<std::string> callers;  // List of functions that call this
    bool hasExternalCaller;        // Called from outside its module
    bool isDeadCode;               // Never called externally
};

// Specialization candidate tracking
struct SpecializationInfo {
    std::string baseFunctionName;
    std::vector<ArgumentPattern> constantArgs;
    int estimatedSavings;
    float roi;  // Return on Investment (savings / code overhead)
};

class GlobalFunctionDatabase {
public:
    static GlobalFunctionDatabase& instance();

    // Add a function profile to the database
    void addFunctionProfile(const GlobalFunctionProfile& profile);

    // Get profile for a specific function
    GlobalFunctionProfile* getFunctionProfile(const std::string& name);
    const GlobalFunctionProfile* getFunctionProfile(const std::string& name) const;

    // Get all specialization candidates
    std::vector<SpecializationInfo> getSpecializationCandidates() const;

    // Identify dead code (functions with no external callers)
    std::vector<std::string> getDeadFunctions() const;

    // Get all functions callable from outside their module
    std::vector<GlobalFunctionProfile*> getExternalFunctions();

    // Print statistics for debugging
    void printStatistics() const;

    // Clear database (for testing)
    void clear();

private:
    GlobalFunctionDatabase() = default;

    std::unordered_map<std::string, GlobalFunctionProfile> profiles_;

    // Helper to calculate ROI for specialization
    float calculateROI(const GlobalFunctionProfile& profile,
                      const std::vector<ArgumentPattern>& constants) const;
};
