#include "CoOptimizationSelector.hpp"
#include <algorithm>

CoOptimizationSelector::CoOptimizationSelector(const FunctionAnalyzer& funcAnalyzer,
                                             const CallGraphAnalyzer& callGraph,
                                             const DevirtualizationDetector& devirtualizer)
    : funcAnalyzer_(funcAnalyzer), callGraph_(callGraph), devirtualizer_(devirtualizer) {}

std::vector<CoOptimizationSelector::InlinePair> CoOptimizationSelector::getRecommendedInlines() const {
    std::vector<InlinePair> pairs;

    // Get simple callee functions (functions with single caller)
    auto simpleCallees = callGraph_.getSimpleCalleeFunctions();

    for (const auto& callee : simpleCallees) {
        auto calleeNode = callGraph_.getFunctionNode(callee);
        if (!calleeNode) continue;

        // Get the single caller
        auto callers = callGraph_.getCallers(callee);
        if (callers.size() != 1) continue;

        const auto& caller = *callers.begin();

        // Check safety
        if (!canInline(caller, callee)) continue;

        InlinePair pair;
        pair.caller = caller;
        pair.callee = callee;
        pair.isSafe = true;
        pair.benefitScore = estimateInlineBenefit(caller, callee);

        pairs.push_back(pair);
    }

    // Sort by benefit score (descending)
    std::sort(pairs.begin(), pairs.end(), [](const InlinePair& a, const InlinePair& b) {
        return a.benefitScore > b.benefitScore;
    });

    return pairs;
}

std::vector<DevirtualizationDetector::VirtualMethodInfo>
CoOptimizationSelector::getDevirtualizationCandidates() const {
    return devirtualizer_.getDevirtualizableMethods();
}

std::vector<CoOptimizationSelector::CoOptimizationGroup>
CoOptimizationSelector::getCoOptimizationGroups() const {
    std::vector<CoOptimizationGroup> groups;

    // Simple strategy: group leaf functions that are called together
    auto leaves = callGraph_.getLeafFunctions();

    // For now, return empty - this is a placeholder for future sophisticated grouping
    return groups;
}

bool CoOptimizationSelector::canInline(const std::string& caller, const std::string& callee) const {
    // Safety checks:
    // 1. Neither function is recursive
    // 2. Inlining wouldn't create a cycle
    // 3. Callee is a leaf function or very small

    if (hasRecursion(caller) || hasRecursion(callee)) {
        return false;
    }

    if (wouldCreateCycle(caller, callee)) {
        return false;
    }

    auto calleeNode = callGraph_.getFunctionNode(callee);
    if (!calleeNode) return false;

    // Must be a leaf function to be safely inlineable
    if (!calleeNode->isLeaf) {
        return false;
    }

    return true;
}

int CoOptimizationSelector::estimateInlineBenefit(const std::string& caller, const std::string& callee) const {
    // Benefit = callee_size + call_overhead - potential_code_growth
    // Call overhead typically 3-4 bytes (JSR + return setup)
    // Code growth from inlining is typically minimal for small functions

    int benefit = 0;

    auto calleeNode = callGraph_.getFunctionNode(callee);
    if (!calleeNode || !calleeNode->characteristics) {
        return benefit;
    }

    int calleeSize = calleeNode->characteristics->estimatedCodeSize;
    int callOverhead = 4; // Estimate: JSR (3 bytes) + return (1 byte)

    // If function is small and called only once, benefit is significant
    if (calleeSize > 0) {
        benefit = calleeSize + callOverhead;
    }

    return benefit;
}

bool CoOptimizationSelector::hasRecursion(const std::string& func) const {
    auto chars = funcAnalyzer_.getCharacteristics(func);
    if (!chars) return false;
    return chars->isRecursive;
}

bool CoOptimizationSelector::wouldCreateCycle(const std::string& caller, const std::string& callee) const {
    // Check if inlining callee into caller would create a cycle
    // This happens if callee transitively calls caller

    auto reachable = callGraph_.getReachableFunctions(callee);
    return reachable.count(caller) > 0;
}

int CoOptimizationSelector::estimateFunctionSize(const std::string& funcName) const {
    auto chars = funcAnalyzer_.getCharacteristics(funcName);
    if (!chars) return 0;
    return chars->estimatedCodeSize;
}
