#pragma once

#include "GlobalFunctionDatabase.hpp"
#include "IPOAnalyzer.hpp"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

// Represents a specialization decision for code generation
// Phase 91.3.4: Infrastructure for generating specialized function variants
// Full implementation deferred to Phase 91.3.5+ pending AST cloning support
struct SpecializedFunctionPlan {
    std::string baseFunctionName;       // Original function name
    std::string specializationName;     // Specialized variant name
    std::vector<long> constantArgs;     // Constant values for parameters
    int estimatedSavings;               // Estimated code size savings
};

// Generates specialization plans from IPOAnalyzer decisions
// Current phase provides analysis and planning; code generation deferred
class SpecializationCodeGenerator {
public:
    SpecializationCodeGenerator();

    // Analyze specialization candidates and create plans
    // Returns list of specialization plans ready for implementation
    std::vector<SpecializedFunctionPlan> analyzeSpecializations(
        const std::vector<SpecializationDecision>& decisions);

    // Get detailed analysis of specialization opportunities
    // Provides metrics for ROI calculation and candidate ranking
    struct SpecializationAnalysis {
        int totalCandidates = 0;
        int viableCandidates = 0;  // Candidates meeting ROI threshold
        int estimatedTotalSavings = 0;
        std::vector<SpecializedFunctionPlan> plans;
    };

    SpecializationAnalysis analyze(
        const std::vector<SpecializationDecision>& decisions);

private:
    // Helper to calculate implementation effort for a specialization
    int estimateImplementationEffort(const SpecializationDecision& decision);

    // Helper to rank candidates by ROI
    float calculateROI(int estimatedSavings, int implementationEffort);
};
