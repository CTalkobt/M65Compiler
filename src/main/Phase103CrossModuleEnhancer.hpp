// Phase 103: Cross-Module Enhancement
// Improves cross-module analysis with advanced co-location and inter-module optimization

#pragma once

#include "Phase102AdaptiveSelector.hpp"
#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>

namespace phase103 {

// Inter-module variable relationship
struct InterModuleRelationship {
    std::string sourceModule;
    std::string targetModule;
    std::string variableName;
    int accessCount = 0;           // Accesses across module boundary
    int callFrequency = 0;         // When called from source
    bool isHotPath = false;        // Frequently accessed
    double benefit = 0.0;          // Estimated benefit of co-location
};

// Cross-module co-location group
struct CoLocationGroup {
    std::string groupId;
    std::vector<std::string> variables;
    std::vector<std::string> affectedModules;
    int estimatedSize = 0;
    double expectedBenefit = 0.0;  // Combined benefit
    int preferredBank = -1;        // Target bank if assigned
    bool isSafe = true;
    std::vector<std::string> constraints;
};

// Cross-module enhancement analyzer
class Phase103CrossModuleEnhancer {
public:
    explicit Phase103CrossModuleEnhancer(
        const phase102::Phase102AdaptiveSelector& selector)
        : selector_(selector) {}

    // Analyze inter-module relationships
    std::vector<InterModuleRelationship> analyzeInterModuleAccess(
        const std::vector<std::string>& moduleNames);

    // Identify co-location opportunities across modules
    std::vector<CoLocationGroup> findCoLocationGroups(
        const std::vector<InterModuleRelationship>& relationships);

    // Group variables for optimal bank assignment
    std::vector<CoLocationGroup> groupVariablesForBanking(
        const std::vector<CoLocationGroup>& groups);

    // Validate cross-module constraints
    bool validateCrossModuleConstraints(const CoLocationGroup& group);

    // Calculate inter-module benefit
    double calculateInterModuleBenefit(
        const InterModuleRelationship& relationship);

    // Generate cross-module optimization hints
    std::vector<std::shared_ptr<phase100::OptimizationHint>>
    generateCrossModuleHints(const CoLocationGroup& group);

    // Generate enhancement report
    std::string generateEnhancementReport(
        const std::vector<CoLocationGroup>& groups);

    // Get enhancement statistics
    int getTotalCrossModuleAccesses() const { return totalCrossModuleAccesses_; }
    int getIdentifiedGroups() const { return identifiedGroups_; }
    double getTotalEstimatedBenefit() const { return totalBenefit_; }

private:
    const phase102::Phase102AdaptiveSelector& selector_;

    int totalCrossModuleAccesses_ = 0;
    int identifiedGroups_ = 0;
    double totalBenefit_ = 0.0;
    std::map<std::string, std::vector<InterModuleRelationship>> relationshipsByModule_;

    // Helper methods
    bool canCoLocate(const InterModuleRelationship& r1,
                    const InterModuleRelationship& r2);
    int estimateGroupSize(const CoLocationGroup& group);
    std::string formatGroupRow(const CoLocationGroup& group);
};

} // namespace phase103
