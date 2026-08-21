// Phase 100.3: Link-Time Coordinator Implementation
#include "Phase100Coordinator.hpp"
#include <algorithm>

namespace phase100 {

CoordinationPlan LinkTimeCoordinator::coordinateHints() {
    CoordinationPlan plan;
    appliedHints_.clear();
    skippedHints_.clear();
    totalBenefit_ = 0;

    // Get all hints
    auto allHints = collector_.getAllHints();

    // Try to apply each hint in optimal order
    auto applicationOrder = collector_.getOptimalApplicationOrder();

    for (const auto& hint : applicationOrder) {
        if (!resolver_.isSafeToApply(*hint, appliedHints_)) {
            skippedHints_.push_back(hint);
            continue;
        }

        if (validateHintApplication(hint)) {
            applyHint(hint);
            plan.hintsToApply.push_back(hint);
            totalBenefit_ += hint->estimatedBenefit;
        } else {
            skippedHints_.push_back(hint);
        }
    }

    // Estimate total benefit with synergy
    plan.estimatedTotalBenefit = resolver_.estimateCombinedBenefit(appliedHints_);
    plan.hintsToDeferOrSkip = skippedHints_;

    // Choose strategy based on benefit/risk
    if (appliedHints_.size() > 3) {
        plan.strategy = "aggressive";
    } else if (appliedHints_.size() > 1) {
        plan.strategy = "moderate";
    } else {
        plan.strategy = "conservative";
    }

    return plan;
}

void LinkTimeCoordinator::applyIPOSpecialization() {
    // Apply Phase 91 specialization hints
    auto ipoHints = collector_.getHintsByPhase("91");

    for (const auto& hint : ipoHints) {
        if (hint->hintType == "specialize") {
            applyHint(hint);
        }
    }
}

void LinkTimeCoordinator::applyFieldCaching() {
    // Apply Phase 96.5 field caching hints
    auto cacheHints = collector_.getHintsByPhase("96.5");

    for (const auto& hint : cacheHints) {
        if (hint->hintType == "cache_field") {
            applyHint(hint);
        }
    }
}

void LinkTimeCoordinator::applyBankHoisting() {
    // Apply Phase 99 bank hoisting hints
    auto bankHints = collector_.getHintsByPhase("99");

    for (const auto& hint : bankHints) {
        if (hint->hintType == "bank_hoist" || hint->hintType == "bank_cache") {
            applyHint(hint);
        }
    }
}

void LinkTimeCoordinator::handleNestedLoopInteractions() {
    // Find hints targeting the same loop
    std::map<std::string, std::vector<std::shared_ptr<OptimizationHint>>> loopHints;

    for (const auto& hint : appliedHints_) {
        if (!hint->targetLoop.empty()) {
            loopHints[hint->targetLoop].push_back(hint);
        }
    }

    // For each loop with multiple hints, coordinate them
    for (auto& [loopLabel, hints] : loopHints) {
        if (hints.size() > 1) {
            // Coordinate multiple hints in same loop
            // Ensure proper ordering and interaction handling
            std::sort(hints.begin(), hints.end(),
                [](const std::shared_ptr<OptimizationHint>& a,
                   const std::shared_ptr<OptimizationHint>& b) {
                    return a->priority > b->priority;
                });
        }
    }
}

std::string LinkTimeCoordinator::generateCoordinatedCode() {
    std::string code;

    code += "; Phase 100.3: Link-Time Coordinated Optimizations\n";
    code += "; Applied " + std::to_string(appliedHints_.size()) + " hints\n";
    code += "; Estimated benefit: " + std::to_string(totalBenefit_) + " cycles\n\n";

    // Generate code for each applied hint
    for (const auto& hint : appliedHints_) {
        code += generateHintCode(hint);
    }

    return code;
}

void LinkTimeCoordinator::applyHint(const std::shared_ptr<OptimizationHint>& hint) {
    appliedHints_.push_back(hint);
    recordHintDependencies();
}

bool LinkTimeCoordinator::validateHintApplication(
    const std::shared_ptr<OptimizationHint>& hint) {
    // Check if hint can be safely applied in current context
    if (!hint->isApplicable) return false;

    // Check dependencies are satisfied
    for (const auto& dep : hint->dependsOn) {
        bool depSatisfied = false;
        for (const auto& applied : appliedHints_) {
            if (applied->hintType == dep) {
                depSatisfied = true;
                break;
            }
        }
        if (!depSatisfied) return false;
    }

    return true;
}

std::string LinkTimeCoordinator::generateHintCode(
    const std::shared_ptr<OptimizationHint>& hint) {
    std::string code;

    code += "; Hint: " + hint->hintType + " (" + hint->phase + ")\n";
    code += "; Target: " + hint->targetFunction + " / " + hint->targetLoop + "\n";
    code += "; Benefit: " + std::to_string(hint->estimatedBenefit) + " cycles\n";
    code += "\n";

    return code;
}

void LinkTimeCoordinator::recordHintDependencies() {
    // Build dependency graph for applied hints
    // Used for validation and ordering
}

} // namespace phase100
