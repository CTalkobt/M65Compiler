// Phase 102.2: Adaptive Selection Implementation
#include "Phase102AdaptiveSelector.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <numeric>

namespace phase102 {

AdaptiveSelectionResult Phase102AdaptiveSelector::selectHints(
    const CodePattern& pattern,
    const std::vector<OptimizationRecommendation>& recommendations,
    const SelectionStrategy& strategy) {

    AdaptiveSelectionResult result;
    result.targetName = pattern.targetName;
    totalSelections_++;

    // Filter recommendations by strategy
    auto filtered = filterRecommendations(recommendations, strategy);

    // Select hints passing all criteria
    for (const auto& rec : filtered) {
        result.selectedHints.push_back(rec.hint);
        confidenceValues_.push_back(rec.confidenceScore);
        result.estimatedTotalBenefit += rec.expectedBenefit;
    }

    // Resolve conflicts if enabled
    if (strategy.respectConflicts && result.selectedHints.size() > 1) {
        result.selectedHints = resolveConflicts(result.selectedHints);
    }

    // Estimate combined benefit with synergy
    if (strategy.enableSynergy && result.selectedHints.size() > 1) {
        double synergyBonus = calculateSynergyBonus(result.selectedHints);
        result.estimatedTotalBenefit *= (1.0 + synergyBonus);
    }

    // Determine if selection is optimal
    result.isOptimal = !result.selectedHints.empty() &&
                      result.estimatedTotalBenefit > strategy.benefitThreshold;
    if (result.isOptimal) optimalSelections_++;

    return result;
}

SelectionStrategy Phase102AdaptiveSelector::generateStrategy(
    const std::string& strategyType) {
    SelectionStrategy strategy;
    strategy.strategyName = strategyType;

    if (strategyType == "aggressive") {
        strategy.confidenceThreshold = 0.50;    // Lower threshold
        strategy.benefitThreshold = 2.0;        // Accept even small benefits
        strategy.enableSynergy = true;
    } else if (strategyType == "conservative") {
        strategy.confidenceThreshold = 0.90;    // High threshold
        strategy.benefitThreshold = 15.0;       // Only significant benefits
        strategy.enableSynergy = false;         // Don't rely on synergy
    } else {  // "balanced" (default)
        strategy.confidenceThreshold = 0.75;
        strategy.benefitThreshold = 5.0;
        strategy.enableSynergy = true;
    }

    return strategy;
}

std::vector<OptimizationRecommendation>
Phase102AdaptiveSelector::filterRecommendations(
    const std::vector<OptimizationRecommendation>& recommendations,
    const SelectionStrategy& strategy) {
    std::vector<OptimizationRecommendation> filtered;

    for (const auto& rec : recommendations) {
        // Apply confidence threshold
        if (rec.confidenceScore < strategy.confidenceThreshold) {
            continue;
        }

        // Apply benefit threshold
        if (rec.expectedBenefit < strategy.benefitThreshold) {
            continue;
        }

        // Check applicability
        if (!rec.isApplicable) {
            continue;
        }

        filtered.push_back(rec);
    }

    return filtered;
}

std::vector<std::shared_ptr<phase100::OptimizationHint>>
Phase102AdaptiveSelector::resolveConflicts(
    const std::vector<std::shared_ptr<phase100::OptimizationHint>>& hints) {
    std::vector<std::shared_ptr<phase100::OptimizationHint>> resolved;

    for (size_t i = 0; i < hints.size(); ++i) {
        bool conflictFound = false;

        for (size_t j = 0; j < resolved.size(); ++j) {
            if (isConflicting(hints[i], resolved[j])) {
                conflictFound = true;
                // Keep the higher priority hint
                if (hints[i]->priority > resolved[j]->priority) {
                    resolved.erase(resolved.begin() + j);
                } else {
                    conflictFound = true;
                    break;
                }
            }
        }

        if (!conflictFound) {
            resolved.push_back(hints[i]);
        }
    }

    return resolved;
}

double Phase102AdaptiveSelector::estimateCombinedBenefit(
    const std::vector<std::shared_ptr<phase100::OptimizationHint>>& hints,
    const CodePattern& pattern) {
    double totalBenefit = 0.0;

    for (const auto& hint : hints) {
        totalBenefit += hint->estimatedBenefit;
    }

    // Apply synergy bonus
    if (hints.size() > 1) {
        double synergyBonus = calculateSynergyBonus(hints);
        totalBenefit *= (1.0 + synergyBonus);
    }

    return totalBenefit;
}

std::string Phase102AdaptiveSelector::generateSelectionReport(
    const AdaptiveSelectionResult& result) {
    std::stringstream report;

    report << "╔════════════════════════════════════════════════════════════════╗\n";
    report << "║     Phase 102.2: Adaptive Hint Selection Report                ║\n";
    report << "║     Pattern-Based Optimization Recommendation                  ║\n";
    report << "╚════════════════════════════════════════════════════════════════╝\n\n";

    report << "SELECTION RESULTS\n";
    report << "──────────────────────────────────────────────────────────────────\n";
    report << "Target:                   " << result.targetName << "\n";
    report << "Selected Hints:           " << result.selectedHints.size() << "\n";
    report << "Rejected Hints:           " << result.rejectedHints.size() << "\n";
    report << "Estimated Total Benefit:  " << std::fixed << std::setprecision(1)
           << result.estimatedTotalBenefit << "%\n";
    report << "Optimal Selection:        " << (result.isOptimal ? "Yes" : "No") << "\n\n";

    // Selected hints table
    if (!result.selectedHints.empty()) {
        report << "SELECTED HINTS\n";
        report << "──────────────────────────────────────────────────────────────────\n";
        report << "Phase  Type              Priority  Benefit\n";
        report << "──────────────────────────────────────────────────────────────────\n";

        for (const auto& hint : result.selectedHints) {
            report << std::setw(5) << hint->phase
                   << "  " << std::left << std::setw(16) << hint->hintType
                   << "  " << std::right << std::setw(8) << hint->priority
                   << "  " << std::setw(6) << hint->estimatedBenefit << "\n";
        }
        report << "\n";
    }

    // Rejected hints
    if (!result.rejectedHints.empty()) {
        report << "REJECTED HINTS\n";
        report << "──────────────────────────────────────────────────────────────────\n";
        for (const auto& hint : result.rejectedHints) {
            report << "  • " << hint->hintType << " (Phase " << hint->phase << ")\n";
        }
        report << "\n";
    }

    return report.str();
}

double Phase102AdaptiveSelector::getAverageConfidence() const {
    if (confidenceValues_.empty()) return 0.0;

    double sum = std::accumulate(confidenceValues_.begin(),
                                confidenceValues_.end(), 0.0);
    return sum / confidenceValues_.size();
}

bool Phase102AdaptiveSelector::isConflicting(
    const std::shared_ptr<phase100::OptimizationHint>& h1,
    const std::shared_ptr<phase100::OptimizationHint>& h2) {
    // Check explicit conflicts
    if (h1->conflictsWith.count(h2->hintType) > 0 ||
        h2->conflictsWith.count(h1->hintType) > 0) {
        return true;
    }

    // Different phases usually don't conflict
    if (h1->phase != h2->phase) {
        return false;
    }

    // Same phase, same target might conflict
    if (h1->targetFunction == h2->targetFunction &&
        !h1->targetFunction.empty() &&
        h1->hintType != h2->hintType) {
        // Some same-phase hints can coexist
        return false;
    }

    return false;
}

double Phase102AdaptiveSelector::calculateSynergyBonus(
    const std::vector<std::shared_ptr<phase100::OptimizationHint>>& hints) {
    // Base synergy: 20% for 2 hints, +5% per additional hint
    if (hints.size() < 2) return 0.0;

    double bonus = 0.20;  // 20% for first pair
    if (hints.size() > 2) {
        bonus += (hints.size() - 2) * 0.05;  // 5% per additional
    }

    return std::min(0.40, bonus);  // Cap at 40%
}

std::string Phase102AdaptiveSelector::formatSelectionRow(
    const std::shared_ptr<phase100::OptimizationHint>& hint,
    double confidence) {
    std::stringstream row;

    row << std::left << std::setw(20) << hint->hintType
        << "  " << std::setw(6) << hint->phase
        << "  " << std::fixed << std::setprecision(0)
        << (confidence * 100.0) << "%\n";

    return row.str();
}

} // namespace phase102
