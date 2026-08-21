// Phase 100.2: Constraint Resolution for LTCO
// Determines which hints can be applied together safely

#pragma once

#include "Phase100HintCollector.hpp"
#include <string>
#include <vector>
#include <map>
#include <set>

namespace phase100 {

// Constraint analysis result
struct HintApplicability {
    std::shared_ptr<OptimizationHint> hint;
    bool canApply = false;
    std::vector<std::string> blockedBy;      // Conflicting hints
    std::vector<std::string> requires;       // Required hints
    int estimatedCombinedBenefit = 0;        // With dependencies
    std::string reason;                      // Why not applicable
};

// Constraint resolver
class ConstraintResolver {
public:
    explicit ConstraintResolver(const HintCollector& collector)
        : collector_(collector) {}

    // Analyze all hints for applicability
    std::vector<HintApplicability> resolveAll();

    // Check if two hints can apply together
    bool canApplyTogether(const OptimizationHint& h1,
                         const OptimizationHint& h2) const;

    // Get maximum compatible hint set
    std::vector<std::shared_ptr<OptimizationHint>>
        getMaximalCompatibleSet();

    // Get hints blocked by a given hint
    std::vector<std::shared_ptr<OptimizationHint>>
        getBlockedHints(const OptimizationHint& hint) const;

    // Estimate combined benefit
    int estimateCombinedBenefit(
        const std::vector<std::shared_ptr<OptimizationHint>>& hints) const;

    // Check safety constraints
    bool isSafeToApply(const OptimizationHint& hint,
                      const std::vector<std::shared_ptr<OptimizationHint>>&
                          alreadyApplied) const;

private:
    const HintCollector& collector_;

    // Safety rules
    bool checkPhaseCompatibility(const OptimizationHint& h1,
                                const OptimizationHint& h2) const;
    bool checkTargetCompatibility(const OptimizationHint& h1,
                                 const OptimizationHint& h2) const;
    bool checkPropertyCompatibility(const OptimizationHint& h1,
                                   const OptimizationHint& h2) const;

    // Utility
    int computeBenefit(const OptimizationHint& hint) const;
};

} // namespace phase100

