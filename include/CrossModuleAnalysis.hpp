#pragma once

#include <string>
#include <vector>
#include <map>
#include <cstdint>

namespace ipo {

// Phase 92.1: Cross-Module Call Site Analysis
// Represents a call site with constant argument patterns

struct CallSitePattern {
    std::string functionName;           // "_multiply_by_n"
    std::vector<int> constantParamIndices;  // Which params are constant [1]
    std::vector<uint64_t> constantValues;   // Values for those params [4]

    bool operator==(const CallSitePattern& other) const {
        return functionName == other.functionName &&
               constantParamIndices == other.constantParamIndices &&
               constantValues == other.constantValues;
    }
};

struct CallSiteInfo {
    CallSitePattern pattern;
    std::string sourceFunction;         // "_main"
    int sourceLine = 0;                 // For debugging
    int callCount = 1;                  // Multiple identical calls at same site
};

// Call site collector during code generation
class CallSiteCollector {
public:
    CallSiteCollector() = default;

    // Track a function call with constant argument pattern
    void recordCallSite(const CallSitePattern& pattern,
                      const std::string& sourceFunc,
                      int line) {
        CallSiteInfo info{pattern, sourceFunc, line};
        callSites_.push_back(info);
    }

    // Get all recorded call sites
    const std::vector<CallSiteInfo>& getCallSites() const {
        return callSites_;
    }

    // Format for emission as .callsite directive
    std::string formatDirective(const CallSiteInfo& info) const;

    // Clear for next function
    void reset() {
        callSites_.clear();
    }

private:
    std::vector<CallSiteInfo> callSites_;
};

// Phase 92.1: Global cross-module call graph (linker-level)
struct CrossModuleCallGraph {
    // functionName -> (pattern -> count across all modules)
    std::map<std::string,
             std::map<std::vector<uint64_t>, int>>
        functionCallPatterns;

    // Track which modules (object files) have each pattern
    std::map<std::string,
             std::map<std::vector<uint64_t>, std::vector<std::string>>>
        patternModules;  // func -> pattern -> list of .o45 files

    void recordPattern(const std::string& func,
                      const std::vector<uint64_t>& pattern,
                      const std::string& module) {
        functionCallPatterns[func][pattern]++;
        patternModules[func][pattern].push_back(module);
    }

    // Identify specialization candidates (>1 module calling with same pattern)
    bool isCrossModulePattern(const std::string& func,
                             const std::vector<uint64_t>& pattern) const {
        auto funcIt = functionCallPatterns.find(func);
        if (funcIt == functionCallPatterns.end()) return false;

        auto patIt = funcIt->second.find(pattern);
        if (patIt == funcIt->second.end()) return false;

        // Cross-module if appears in 2+ different modules
        auto modIt = patternModules.find(func);
        if (modIt == patternModules.end()) return false;

        auto modPatternsIt = modIt->second.find(pattern);
        if (modPatternsIt == modIt->second.end()) return false;

        return modPatternsIt->second.size() >= 2;
    }
};

} // namespace ipo
