// Phase 100.2: Constraint Resolution Implementation
#include "OptimizationConstraintResolver.hpp"
#include <algorithm>

namespace phase100 {

std::vector<HintApplicability> ConstraintResolver::resolveAll() {
    std::vector<HintApplicability> result;
    auto allHints = collector_.getAllHints();

    for (const auto& hint : allHints) {
        HintApplicability app;
        app.hint = hint;
        app.canApply = true;
        app.reason = "OK";

        // Check phase compatibility
        for (const auto& other : allHints) {
            if (other == hint) continue;
            if (!checkPhaseCompatibility(*hint, *other)) {
                app.canApply = false;
                app.blockedBy.push_back(other->hintType);
            }
        }

        // Check target compatibility
        for (const auto& other : allHints) {
            if (other == hint) continue;
            if (!checkTargetCompatibility(*hint, *other)) {
                app.canApply = false;
                app.blockedBy.push_back(other->hintType);
            }
        }

        // Estimate combined benefit
        app.estimatedCombinedBenefit = hint->estimatedBenefit;

        result.push_back(app);
    }

    return result;
}

bool ConstraintResolver::canApplyTogether(const OptimizationHint& h1,
                                          const OptimizationHint& h2) const {
    // Different targets generally compatible
    if (h1.targetFunction != h2.targetFunction &&
        h1.targetLoop != h2.targetLoop) {
        return true;
    }

    // Same target - check phase and property compatibility
    if (!checkPhaseCompatibility(h1, h2)) return false;
    if (!checkPropertyCompatibility(h1, h2)) return false;

    // Explicit conflicts
    if (h1.conflictsWith.count(h2.hintType) > 0) return false;
    if (h2.conflictsWith.count(h1.hintType) > 0) return false;

    return true;
}

std::vector<std::shared_ptr<OptimizationHint>>
    ConstraintResolver::getMaximalCompatibleSet() {
    auto allHints = collector_.getAllHints();
    std::vector<std::shared_ptr<OptimizationHint>> maxSet;

    // Greedy selection: start with highest benefit hints
    auto sorted = allHints;
    std::sort(sorted.begin(), sorted.end(),
        [](const auto& a, const auto& b) {
            return a->estimatedBenefit > b->estimatedBenefit;
        });

    for (const auto& hint : sorted) {
        bool canAdd = true;

        // Check compatibility with already selected hints
        for (const auto& selected : maxSet) {
            if (!canApplyTogether(*hint, *selected)) {
                canAdd = false;
                break;
            }
        }

        if (canAdd) {
            maxSet.push_back(hint);
        }
    }

    return maxSet;
}

std::vector<std::shared_ptr<OptimizationHint>>
    ConstraintResolver::getBlockedHints(const OptimizationHint& hint) const {
    std::vector<std::shared_ptr<OptimizationHint>> blocked;
    auto allHints = collector_.getAllHints();

    for (const auto& other : allHints) {
        if (other->hintType == hint.hintType) continue;
        if (!canApplyTogether(hint, *other)) {
            blocked.push_back(other);
        }
    }

    return blocked;
}

int ConstraintResolver::estimateCombinedBenefit(
    const std::vector<std::shared_ptr<OptimizationHint>>& hints) const {
    int total = 0;

    // Sum individual benefits
    for (const auto& hint : hints) {
        total += hint->estimatedBenefit;
    }

    // Bonus for coordinated hints (20% synergy)
    if (hints.size() > 1) {
        total = (total * 120) / 100;
    }

    return total;
}

bool ConstraintResolver::isSafeToApply(
    const OptimizationHint& hint,
    const std::vector<std::shared_ptr<OptimizationHint>>& alreadyApplied) const {
    // Check against all applied hints
    for (const auto& applied : alreadyApplied) {
        if (!canApplyTogether(hint, *applied)) {
            return false;
        }
    }

    return true;
}

bool ConstraintResolver::checkPhaseCompatibility(const OptimizationHint& h1,
                                                 const OptimizationHint& h2) const {
    // Phase 91 (IPO) + Phase 96.5 (Caching): Compatible
    if ((h1.phase == "91" && h2.phase == "96.5") ||
        (h1.phase == "96.5" && h2.phase == "91")) {
        return true;
    }

    // Phase 91 (IPO) + Phase 99 (Banking): Compatible
    if ((h1.phase == "91" && h2.phase == "99") ||
        (h1.phase == "99" && h2.phase == "91")) {
        return true;
    }

    // Phase 96.5 (Caching) + Phase 99 (Banking): Compatible
    if ((h1.phase == "96.5" && h2.phase == "99") ||
        (h1.phase == "99" && h2.phase == "96.5")) {
        return true;
    }

    // Same phase: depends on type
    if (h1.phase == h2.phase) {
        // Same phase usually compatible unless explicit conflict
        return h1.conflictsWith.count(h2.hintType) == 0 &&
               h2.conflictsWith.count(h1.hintType) == 0;
    }

    return false;
}

bool ConstraintResolver::checkTargetCompatibility(const OptimizationHint& h1,
                                                  const OptimizationHint& h2) const {
    // Different targets always compatible
    if (h1.targetFunction != h2.targetFunction &&
        h1.targetLoop != h2.targetLoop) {
        return true;
    }

    // Same target - check hint types for conflicts
    // Inlining + specialization: Compatible
    if ((h1.hintType == "inline" && h2.hintType == "specialize") ||
        (h1.hintType == "specialize" && h2.hintType == "inline")) {
        return true;
    }

    // Bank setup hoisting + register caching: Compatible
    if ((h1.hintType == "bank_hoist" && h2.hintType == "bank_cache") ||
        (h1.hintType == "bank_cache" && h2.hintType == "bank_hoist")) {
        return true;
    }

    // Field caching + bank optimization: Compatible
    if ((h1.hintType == "cache_field" && h2.hintType == "bank_hoist") ||
        (h1.hintType == "bank_hoist" && h2.hintType == "cache_field")) {
        return true;
    }

    return true;  // Default: compatible
}

bool ConstraintResolver::checkPropertyCompatibility(const OptimizationHint& h1,
                                                   const OptimizationHint& h2) const {
    // Same target variable: must be compatible properties
    if (h1.targetVariable == h2.targetVariable && !h1.targetVariable.empty()) {
        // Inlining same function twice: conflict
        if (h1.hintType == "inline" && h2.hintType == "inline") {
            return false;
        }
    }

    return true;
}

int ConstraintResolver::computeBenefit(const OptimizationHint& hint) const {
    return hint.estimatedBenefit;
}

} // namespace phase100
