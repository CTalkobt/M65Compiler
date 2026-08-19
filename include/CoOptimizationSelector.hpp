#pragma once

#include "AST.hpp"
#include "CallGraphAnalyzer.hpp"
#include "DevirtualizationDetector.hpp"
#include "FunctionAnalyzer.hpp"
#include <map>
#include <set>
#include <string>
#include <vector>

// Cross-function optimization selector
// Uses call graph, recursion info, and virtual method analysis to recommend optimizations

class CoOptimizationSelector {
public:
    struct InlinePair {
        std::string caller;
        std::string callee;
        int benefitScore = 0;  // Higher = more benefit
        bool isSafe = true;
    };

    struct CoOptimizationGroup {
        std::vector<std::string> functions;
        int groupBenefit = 0;
        std::string optimization;  // "inline", "unroll", "tcs"
    };

    CoOptimizationSelector(const FunctionAnalyzer& funcAnalyzer,
                          const CallGraphAnalyzer& callGraph,
                          const DevirtualizationDetector& devirtualizer);

    // Get recommended inline pairs (caller, callee pairs that should be inlined)
    std::vector<InlinePair> getRecommendedInlines() const;

    // Get functions that can be safely devirtualized
    std::vector<DevirtualizationDetector::VirtualMethodInfo> getDevirtualizationCandidates() const;

    // Get co-optimization groups
    std::vector<CoOptimizationGroup> getCoOptimizationGroups() const;

    // Check if inlining is safe between two functions
    bool canInline(const std::string& caller, const std::string& callee) const;

    // Estimate code size benefit of inlining
    int estimateInlineBenefit(const std::string& caller, const std::string& callee) const;

private:
    const FunctionAnalyzer& funcAnalyzer_;
    const CallGraphAnalyzer& callGraph_;
    const DevirtualizationDetector& devirtualizer_;

    // Helper methods
    bool hasRecursion(const std::string& func) const;
    bool wouldCreateCycle(const std::string& caller, const std::string& callee) const;
    int estimateFunctionSize(const std::string& funcName) const;
};
