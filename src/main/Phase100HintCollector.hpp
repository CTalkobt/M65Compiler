// Phase 100.1: Hint Collection & Analysis
// Collects and analyzes optimization hints from all phases

#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>

namespace phase100 {

// Base optimization hint structure
struct OptimizationHint {
    std::string phase;                    // "91", "96.5", "99"
    std::string hintType;                 // "inline", "cache", "bank_hoist", etc.
    std::string targetFunction;
    std::string targetVariable;
    std::string targetLoop;

    // Hint-specific properties
    std::map<std::string, std::string> properties;

    // Priority and impact
    int priority = 0;                     // Higher = apply first
    int estimatedBenefit = 0;             // Code size/speed savings

    // Constraints
    std::vector<std::string> dependsOn;   // Other hints to apply first
    std::set<std::string> conflictsWith;  // Mutually exclusive hints

    // Status
    bool isApplicable = true;
    std::string applicabilityReason;
};

// Hint graph node
struct HintGraphNode {
    std::shared_ptr<OptimizationHint> hint;
    std::vector<std::shared_ptr<HintGraphNode>> dependencies;
    std::vector<std::shared_ptr<HintGraphNode>> dependents;
    int appliedOrder = -1;
};

// Hint collector and analyzer
class HintCollector {
public:
    HintCollector() = default;

    // Collect hints from object files
    void collectFromObjectFiles(const std::vector<std::string>& o45FilePaths);

    // Add hint manually (for testing)
    void addHint(const OptimizationHint& hint);

    // Analyze hint dependencies
    void analyzeHintDependencies();

    // Get all collected hints
    std::vector<std::shared_ptr<OptimizationHint>> getAllHints() const;

    // Query hints by phase
    std::vector<std::shared_ptr<OptimizationHint>> getHintsByPhase(
        const std::string& phase) const;

    // Query hints by type
    std::vector<std::shared_ptr<OptimizationHint>> getHintsByType(
        const std::string& hintType) const;

    // Query hints for function
    std::vector<std::shared_ptr<OptimizationHint>> getHintsForFunction(
        const std::string& funcName) const;

    // Query hints for loop
    std::vector<std::shared_ptr<OptimizationHint>> getHintsForLoop(
        const std::string& loopLabel) const;

    // Get hint graph
    const std::vector<std::shared_ptr<HintGraphNode>>& getHintGraph() const {
        return hintGraph_;
    }

    // Check if hints can be applied together
    bool canApplyTogether(const std::shared_ptr<OptimizationHint>& h1,
                         const std::shared_ptr<OptimizationHint>& h2) const;

    // Get optimal application order
    std::vector<std::shared_ptr<OptimizationHint>> getOptimalApplicationOrder();

    // Statistics
    int getTotalHintCount() const { return hints_.size(); }
    int getApplicableHintCount() const;
    double getAverageEstimatedBenefit() const;

private:
    std::vector<std::shared_ptr<OptimizationHint>> hints_;
    std::vector<std::shared_ptr<HintGraphNode>> hintGraph_;

    // Helper methods
    std::vector<OptimizationHint> parseO45Hints(const std::string& o45FilePath);
    void buildDependencyGraph();
    void validateHints();
    bool areCompatible(const OptimizationHint& h1,
                      const OptimizationHint& h2) const;
};

} // namespace phase100

