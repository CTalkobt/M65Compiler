// Phase 100.4: Cross-Hint Dependency Analysis
// Analyzes loops with multiple optimization hints and coordinates them

#pragma once

#include "Phase100Coordinator.hpp"
#include <string>
#include <vector>
#include <map>
#include <set>

namespace phase100 {

// Loop with multiple hints
struct MultiHintLoop {
    std::string loopLabel;
    std::vector<std::shared_ptr<OptimizationHint>> hints;
    int estimatedIterations = 0;
    std::vector<std::string> dependencyOrder;  // Optimal application order
    bool isSafe = true;
    std::string safetyNote;
};

// Dependency analyzer for LTCO
class DependencyAnalyzer {
public:
    explicit DependencyAnalyzer(const LinkTimeCoordinator& coordinator)
        : coordinator_(coordinator) {}

    // Find all loops with multiple hints
    std::vector<MultiHintLoop> analyzeLoopDependencies();

    // Build hint dependency graph for a loop
    void buildLoopDependencyGraph(MultiHintLoop& loop);

    // Check if hints are safe to apply together in loop
    bool isLoopHintSetSafe(const MultiHintLoop& loop);

    // Generate optimal hint application order for loop
    std::vector<std::string> computeOptimalHintOrder(
        const MultiHintLoop& loop);

    // Estimate combined benefit in loop context
    int estimateLoopBenefit(const MultiHintLoop& loop);

private:
    const LinkTimeCoordinator& coordinator_;
};

} // namespace phase100
