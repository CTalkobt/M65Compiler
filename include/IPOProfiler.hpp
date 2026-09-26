#pragma once

#include "GlobalFunctionDatabase.hpp"
#include "CallPatternAnalyzer.hpp"
#include <string>
#include <memory>

// Forward declarations
class IRBuilder;
namespace ir {
    struct Function;
    struct Module;
}

/**
 * IPOProfiler: Integrates with IRBuilder to collect function profiles
 * during IR generation and populate GlobalFunctionDatabase.
 *
 * This enables cross-module optimization analysis by capturing:
 * - Function signatures and parameter types
 * - Local variable counts
 * - Function call information
 * - Leaf function detection
 * - Call site argument patterns
 */
class IPOProfiler {
public:
    IPOProfiler() = default;

    // Called during IR generation to record function definition
    void recordFunctionDefinition(
        const std::string& functionName,
        int paramCount,
        int localVarCount,
        bool hasNoCalls,
        bool isLeafCandidate,
        const std::string& moduleName
    );

    // Called when a function call is encountered
    void recordFunctionCall(
        const std::string& callerId,
        const std::string& calleeName,
        int argumentCount,
        bool hasConstantArgs
    );

    // Finalize profiling after IR generation
    void finalizeProfiles();

    // Get the populated database (singleton)
    GlobalFunctionDatabase& getDatabase() {
        return GlobalFunctionDatabase::instance();
    }

    const GlobalFunctionDatabase& getDatabase() const {
        return GlobalFunctionDatabase::instance();
    }

private:
    std::string currentModule_;

    // Track functions being built
    struct FunctionBuildState {
        std::string name;
        int paramCount = 0;
        int localVarCount = 0;
        bool hasNoCalls = true;
        bool isLeafCandidate = false;
        std::vector<CallAnalysisInfo> callSites;
    };

    std::unordered_map<std::string, FunctionBuildState> buildStates_;
};
