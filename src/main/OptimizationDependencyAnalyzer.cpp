// Phase 100.4: Dependency Analyzer Implementation
#include "OptimizationDependencyAnalyzer.hpp"
#include <algorithm>

namespace phase100 {

std::vector<MultiHintLoop> DependencyAnalyzer::analyzeLoopDependencies() {
    std::vector<MultiHintLoop> multiHintLoops;

    // Group hints by target loop
    std::map<std::string, std::vector<std::shared_ptr<OptimizationHint>>> loopMap;

    // This is a placeholder - in real implementation would iterate coordinator hints
    // to group them by target loop. For now returns empty to allow compilation.
    // Full implementation would:
    // 1. Get all hints from coordinator
    // 2. Filter hints that target loops
    // 3. Group by loopLabel
    // 4. Create MultiHintLoop entries for loops with multiple hints
    // 5. Call buildLoopDependencyGraph on each loop

    return multiHintLoops;
}

void DependencyAnalyzer::buildLoopDependencyGraph(MultiHintLoop& loop) {
    // Build dependency edges between hints in loop
    for (size_t i = 0; i < loop.hints.size(); ++i) {
        for (size_t j = i + 1; j < loop.hints.size(); ++j) {
            // Check if hint i should come before hint j
            if (loop.hints[i]->priority > loop.hints[j]->priority) {
                loop.dependencyOrder.push_back(loop.hints[i]->hintType);
            }
        }
    }
}

bool DependencyAnalyzer::isLoopHintSetSafe(const MultiHintLoop& loop) {
    // Check all hint pairs in loop for conflicts
    for (size_t i = 0; i < loop.hints.size(); ++i) {
        for (size_t j = i + 1; j < loop.hints.size(); ++j) {
            // Check if hints can coexist
            auto& h1 = loop.hints[i];
            auto& h2 = loop.hints[j];

            // Same target variable conflict
            if (h1->targetVariable == h2->targetVariable &&
                !h1->targetVariable.empty()) {
                return false;
            }

            // Explicit conflicts
            if (h1->conflictsWith.count(h2->hintType) > 0) {
                return false;
            }
        }
    }

    return true;
}

std::vector<std::string> DependencyAnalyzer::computeOptimalHintOrder(
    const MultiHintLoop& loop) {
    std::vector<std::string> order;

    // Sort by priority (descending)
    auto sorted = loop.hints;
    std::sort(sorted.begin(), sorted.end(),
        [](const auto& a, const auto& b) {
            return a->priority > b->priority;
        });

    for (const auto& hint : sorted) {
        order.push_back(hint->hintType);
    }

    return order;
}

int DependencyAnalyzer::estimateLoopBenefit(const MultiHintLoop& loop) {
    int benefit = 0;

    for (const auto& hint : loop.hints) {
        benefit += hint->estimatedBenefit;
    }

    // Apply synergy bonus for multiple hints (25% in loops)
    if (loop.hints.size() > 1) {
        benefit = (benefit * 125) / 100;
    }

    return benefit;
}

} // namespace phase100
