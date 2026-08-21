// Phase 100.3: Link-Time Coordinator
// Orchestrates application of multiple optimization hints together

#pragma once

#include "Phase100ConstraintResolver.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace phase100 {

// Coordination decision for a set of hints
struct CoordinationPlan {
    std::vector<std::shared_ptr<OptimizationHint>> hintsToApply;
    std::vector<std::shared_ptr<OptimizationHint>> hintsToDeferOrSkip;
    int estimatedTotalBenefit = 0;
    std::string strategy;  // e.g., "aggressive", "conservative"
};

// Link-time coordinator
class LinkTimeCoordinator {
public:
    explicit LinkTimeCoordinator(const HintCollector& collector,
                                const ConstraintResolver& resolver)
        : collector_(collector), resolver_(resolver) {}

    // Compute optimal coordination plan
    CoordinationPlan coordinateHints();

    // Apply IPO specialization hints
    void applyIPOSpecialization();

    // Apply field caching hints
    void applyFieldCaching();

    // Apply bank setup hoisting
    void applyBankHoisting();

    // Handle nested loop interactions
    void handleNestedLoopInteractions();

    // Generate coordinated code
    std::string generateCoordinatedCode();

    // Get coordination statistics
    int getTotalBenefitAchieved() const { return totalBenefit_; }
    int getHintsApplied() const { return appliedHints_.size(); }
    int getHintsSkipped() const { return skippedHints_.size(); }

private:
    const HintCollector& collector_;
    const ConstraintResolver& resolver_;

    std::vector<std::shared_ptr<OptimizationHint>> appliedHints_;
    std::vector<std::shared_ptr<OptimizationHint>> skippedHints_;
    int totalBenefit_ = 0;

    // Helper methods
    void applyHint(const std::shared_ptr<OptimizationHint>& hint);
    bool validateHintApplication(const std::shared_ptr<OptimizationHint>& hint);
    std::string generateHintCode(const std::shared_ptr<OptimizationHint>& hint);
    void recordHintDependencies();
};

} // namespace phase100
