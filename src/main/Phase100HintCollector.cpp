// Phase 100.1: Hint Collector Implementation
#include "Phase100HintCollector.hpp"
#include <algorithm>
#include <sstream>

namespace phase100 {

void HintCollector::collectFromObjectFiles(const std::vector<std::string>& o45FilePaths) {
    // Parse hints from each object file
    for (const auto& path : o45FilePaths) {
        auto fileHints = parseO45Hints(path);
        for (auto& hint : fileHints) {
            addHint(hint);
        }
    }

    // Analyze dependencies after collection
    analyzeHintDependencies();
}

void HintCollector::addHint(const OptimizationHint& hint) {
    auto hintPtr = std::make_shared<OptimizationHint>(hint);
    hints_.push_back(hintPtr);
}

void HintCollector::analyzeHintDependencies() {
    buildDependencyGraph();
    validateHints();
}

std::vector<std::shared_ptr<OptimizationHint>> HintCollector::getAllHints() const {
    return hints_;
}

std::vector<std::shared_ptr<OptimizationHint>> HintCollector::getHintsByPhase(
    const std::string& phase) const {
    std::vector<std::shared_ptr<OptimizationHint>> result;
    for (const auto& hint : hints_) {
        if (hint->phase == phase) {
            result.push_back(hint);
        }
    }
    return result;
}

std::vector<std::shared_ptr<OptimizationHint>> HintCollector::getHintsByType(
    const std::string& hintType) const {
    std::vector<std::shared_ptr<OptimizationHint>> result;
    for (const auto& hint : hints_) {
        if (hint->hintType == hintType) {
            result.push_back(hint);
        }
    }
    return result;
}

std::vector<std::shared_ptr<OptimizationHint>> HintCollector::getHintsForFunction(
    const std::string& funcName) const {
    std::vector<std::shared_ptr<OptimizationHint>> result;
    for (const auto& hint : hints_) {
        if (hint->targetFunction == funcName) {
            result.push_back(hint);
        }
    }
    return result;
}

std::vector<std::shared_ptr<OptimizationHint>> HintCollector::getHintsForLoop(
    const std::string& loopLabel) const {
    std::vector<std::shared_ptr<OptimizationHint>> result;
    for (const auto& hint : hints_) {
        if (hint->targetLoop == loopLabel) {
            result.push_back(hint);
        }
    }
    return result;
}

bool HintCollector::canApplyTogether(const std::shared_ptr<OptimizationHint>& h1,
                                     const std::shared_ptr<OptimizationHint>& h2) const {
    return areCompatible(*h1, *h2);
}

std::vector<std::shared_ptr<OptimizationHint>> HintCollector::getOptimalApplicationOrder() const {
    std::vector<std::shared_ptr<OptimizationHint>> result;

    // Sort by priority (descending), then by benefit (descending)
    auto sorted = hints_;
    std::sort(sorted.begin(), sorted.end(),
        [](const auto& a, const auto& b) {
            if (a->priority != b->priority) {
                return a->priority > b->priority;
            }
            return a->estimatedBenefit > b->estimatedBenefit;
        });

    // Topological sort considering dependencies
    std::set<std::shared_ptr<OptimizationHint>> applied;
    std::vector<std::shared_ptr<OptimizationHint>> toApply = sorted;

    while (!toApply.empty()) {
        bool madeProgress = false;

        for (auto it = toApply.begin(); it != toApply.end(); ++it) {
            const auto& hint = *it;

            // Check if all dependencies are applied
            bool canApply = true;
            for (const auto& depName : hint->dependsOn) {
                bool depApplied = false;
                for (const auto& appliedHint : applied) {
                    if (appliedHint->hintType == depName) {
                        depApplied = true;
                        break;
                    }
                }
                if (!depApplied) {
                    canApply = false;
                    break;
                }
            }

            if (canApply) {
                result.push_back(hint);
                applied.insert(hint);
                toApply.erase(it);
                madeProgress = true;
                break;
            }
        }

        if (!madeProgress) {
            // Circular dependency or unsatisfiable - apply remaining in order
            for (const auto& hint : toApply) {
                if (!applied.count(hint)) {
                    result.push_back(hint);
                    applied.insert(hint);
                }
            }
            break;
        }
    }

    return result;
}

int HintCollector::getApplicableHintCount() const {
    int count = 0;
    for (const auto& hint : hints_) {
        if (hint->isApplicable) {
            count++;
        }
    }
    return count;
}

double HintCollector::getAverageEstimatedBenefit() const {
    if (hints_.empty()) return 0.0;

    int totalBenefit = 0;
    for (const auto& hint : hints_) {
        if (hint->isApplicable) {
            totalBenefit += hint->estimatedBenefit;
        }
    }

    return (double)totalBenefit / getApplicableHintCount();
}

std::vector<OptimizationHint> HintCollector::parseO45Hints(
    const std::string& o45FilePath) {
    std::vector<OptimizationHint> result;

    // TODO: Implement O45 hint parsing
    // This would read from the .o45 file format and extract hint sections
    // For now, return empty (hints will be added via addHint() in tests)

    return result;
}

void HintCollector::buildDependencyGraph() {
    hintGraph_.clear();

    // Create nodes for each hint
    std::map<std::shared_ptr<OptimizationHint>, std::shared_ptr<HintGraphNode>> nodeMap;
    for (const auto& hint : hints_) {
        auto node = std::make_shared<HintGraphNode>();
        node->hint = hint;
        hintGraph_.push_back(node);
        nodeMap[hint] = node;
    }

    // Build edges based on dependencies
    for (const auto& hint : hints_) {
        auto& srcNode = nodeMap[hint];

        for (const auto& depName : hint->dependsOn) {
            for (const auto& other : hints_) {
                if (other->hintType == depName && other != hint) {
                    srcNode->dependencies.push_back(nodeMap[other]);
                    nodeMap[other]->dependents.push_back(srcNode);
                }
            }
        }
    }
}

void HintCollector::validateHints() {
    // Validate each hint
    for (auto& hint : hints_) {
        hint->isApplicable = true;

        // Check for conflicts
        for (const auto& conflictName : hint->conflictsWith) {
            for (const auto& other : hints_) {
                if (other->hintType == conflictName) {
                    // Mark as not independently applicable if conflicts exist
                    // but may still be applicable with conflict resolution
                }
            }
        }

        // Validate required properties
        if (hint->targetFunction.empty() && hint->targetLoop.empty()) {
            hint->isApplicable = false;
            hint->applicabilityReason = "No target function or loop";
        }
    }
}

bool HintCollector::areCompatible(const OptimizationHint& h1,
                                  const OptimizationHint& h2) const {
    // Same target function/loop can have multiple hints
    if (h1.targetFunction == h2.targetFunction ||
        h1.targetLoop == h2.targetLoop) {
        // Check explicit conflicts
        if (h1.conflictsWith.count(h2.hintType) > 0 ||
            h2.conflictsWith.count(h1.hintType) > 0) {
            return false;
        }
        return true;
    }

    // Different targets are generally compatible
    return true;
}

} // namespace phase100

