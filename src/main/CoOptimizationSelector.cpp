#include "CoOptimizationSelector.hpp"
#include <algorithm>

CoOptimizationSelector::CoOptimizationSelector(const FunctionAnalyzer& funcAnalyzer,
                                             const CallGraphAnalyzer& callGraph,
                                             const DevirtualizationDetector& devirtualizer)
    : funcAnalyzer_(funcAnalyzer), callGraph_(callGraph), devirtualizer_(devirtualizer) {}

std::vector<CoOptimizationSelector::InlinePair> CoOptimizationSelector::getRecommendedInlines() const {
    std::vector<InlinePair> pairs;

    const auto& callGraph = callGraph_.getCallGraph();

    // Analyze each function as a potential callee
    for (const auto& [calleeName, calleeNode] : callGraph) {
        // Skip functions that don't make sense to inline
        if (calleeName.empty() || calleeNode.isRoot) continue;
        if (!calleeNode.isLeaf) continue; // Must be leaf function
        if (calleeNode.incomingCallCount == 0) continue; // Not called
        if (calleeNode.incomingCallCount > 3) continue; // Too many call sites

        // Skip recursive functions
        if (calleeNode.characteristics && calleeNode.characteristics->isRecursive) continue;

        // For each caller of this callee
        for (const auto& caller : calleeNode.callers) {
            // Safety checks
            if (!canInline(caller, calleeName)) continue;

            InlinePair pair;
            pair.caller = caller;
            pair.callee = calleeName;
            pair.isSafe = true;
            pair.benefitScore = estimateInlineBenefit(caller, calleeName);

            if (pair.benefitScore > 0) {
                pairs.push_back(pair);
            }
        }
    }

    // Sort by benefit score (descending)
    std::sort(pairs.begin(), pairs.end(), [](const InlinePair& a, const InlinePair& b) {
        return a.benefitScore > b.benefitScore;
    });

    // Limit to top recommendations (avoid overwhelming code growth)
    if (pairs.size() > 10) {
        pairs.resize(10);
    }

    return pairs;
}

std::vector<DevirtualizationDetector::VirtualMethodInfo>
CoOptimizationSelector::getDevirtualizationCandidates() const {
    return devirtualizer_.getDevirtualizableMethods();
}

std::vector<CoOptimizationSelector::CoOptimizationGroup>
CoOptimizationSelector::getCoOptimizationGroups() const {
    std::vector<CoOptimizationGroup> groups;

    // Strategy: Find groups of related leaf functions for co-optimization
    // Group functions that:
    // 1. Call each other (strongly connected components)
    // 2. Share common callers
    // 3. Have similar characteristics (both loops, both simple, etc.)

    auto leaves = callGraph_.getLeafFunctions();
    std::set<std::string> processed;

    for (const auto& leaf : leaves) {
        if (processed.count(leaf)) continue;

        auto leafNode = callGraph_.getFunctionNode(leaf);
        if (!leafNode) continue;

        // Find shared callers
        std::set<std::string> commonCallers(leafNode->callers.begin(), leafNode->callers.end());

        // Start a potential group
        CoOptimizationGroup group;
        group.functions.push_back(leaf);
        group.optimization = "inline";
        processed.insert(leaf);

        // Look for other leaves that share callers
        for (const auto& otherLeaf : leaves) {
            if (processed.count(otherLeaf)) continue;

            auto otherNode = callGraph_.getFunctionNode(otherLeaf);
            if (!otherNode) continue;

            // Check for common callers
            int sharedCallerCount = 0;
            for (const auto& caller : otherNode->callers) {
                if (commonCallers.count(caller)) {
                    sharedCallerCount++;
                }
            }

            // If they share a caller and both are small, group them
            if (sharedCallerCount > 0 &&
                leafNode->characteristics && otherNode->characteristics &&
                leafNode->characteristics->estimatedCodeSize < 20 &&
                otherNode->characteristics->estimatedCodeSize < 20) {
                group.functions.push_back(otherLeaf);
                processed.insert(otherLeaf);
            }
        }

        // Only add non-trivial groups (size > 1)
        if (group.functions.size() > 1) {
            group.groupBenefit = 5; // Placeholder benefit score
            groups.push_back(group);
        }
    }

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
    // Benefit estimation based on:
    // - Callee size vs call overhead (JSR = 3 bytes, return = 1+ bytes)
    // - Number of call sites (more sites = more total overhead)
    // - Caller and callee complexity interaction

    auto calleeNode = callGraph_.getFunctionNode(callee);
    if (!calleeNode || !calleeNode->characteristics) {
        return 0;
    }

    int calleeSize = calleeNode->characteristics->estimatedCodeSize;
    int callCount = calleeNode->incomingCallCount;

    // Call overhead per invocation (JSR = 3 bytes, RTS preserved = ~2 bytes)
    int callOverhead = 5;

    // Total overhead for all call sites
    int totalOverhead = callOverhead * callCount;

    // Benefit is positive if function is small relative to total call overhead
    // And if inlining is selective (only for smallest functions)
    int benefit = totalOverhead - (calleeSize * (callCount - 1));

    // Bonus for very small functions (< 8 bytes)
    if (calleeSize > 0 && calleeSize < 8) {
        benefit += 10;
    }

    // Bonus for single-call sites
    if (callCount == 1) {
        benefit += 5;
    }

    return std::max(0, benefit);
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
