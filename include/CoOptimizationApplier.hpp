#pragma once

#include "AST.hpp"
#include "CoOptimizationSelector.hpp"
#include <vector>
#include <string>

// Phase 87: Executes co-optimization group strategies
// Applies batch optimization to related functions identified by Phase 86

class CoOptimizationApplier {
public:
    struct OptimizationResult {
        int groupCount = 0;
        int functionsOptimized = 0;
        int estimatedSavings = 0;  // bytes
        std::string optimizationType;
    };

    // Apply co-optimization groups from selector
    OptimizationResult apply(TranslationUnit& unit,
                           const CoOptimizationSelector& selector);

private:
    // Apply batch inlining to a group of functions
    void applyGroupInlining(const CoOptimizationSelector::CoOptimizationGroup& group,
                          TranslationUnit& unit);

    // Mark functions in a group for batch optimization
    void markGroupForOptimization(const std::vector<std::string>& functions,
                                 const std::string& strategy);

    OptimizationResult result_;
};
