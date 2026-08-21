// Phase 102.2: Adaptive Hint Selection Strategy
// Selects optimal hints based on code patterns and profiling data

#pragma once

#include "Phase102PatternAnalyzer.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace phase102 {

// Selection strategy configuration
struct SelectionStrategy {
    std::string strategyName;       // "aggressive", "conservative", "balanced"
    double confidenceThreshold = 0.75;  // Min confidence to select hint
    double benefitThreshold = 5.0;      // Min % benefit to apply
    bool respectConflicts = true;       // Don't apply conflicting hints
    bool enableSynergy = true;          // Consider multi-hint synergy
};

// Adaptive selection result
struct AdaptiveSelectionResult {
    std::string targetName;
    std::vector<std::shared_ptr<phase100::OptimizationHint>> selectedHints;
    std::vector<std::shared_ptr<phase100::OptimizationHint>> rejectedHints;
    double estimatedTotalBenefit = 0.0;
    std::string selectionReasoning;
    bool isOptimal = false;
};

// Adaptive hint selector
class Phase102AdaptiveSelector {
public:
    explicit Phase102AdaptiveSelector(
        const phase102::Phase102PatternAnalyzer& analyzer)
        : analyzer_(analyzer) {}

    // Select optimal hints for a code pattern
    AdaptiveSelectionResult selectHints(
        const CodePattern& pattern,
        const std::vector<OptimizationRecommendation>& recommendations,
        const SelectionStrategy& strategy);

    // Generate selection strategy based on constraints
    SelectionStrategy generateStrategy(const std::string& strategyType);

    // Filter recommendations by strategy
    std::vector<OptimizationRecommendation> filterRecommendations(
        const std::vector<OptimizationRecommendation>& recommendations,
        const SelectionStrategy& strategy);

    // Resolve conflicts between hints
    std::vector<std::shared_ptr<phase100::OptimizationHint>>
    resolveConflicts(const std::vector<std::shared_ptr<phase100::OptimizationHint>>& hints);

    // Estimate combined benefit
    double estimateCombinedBenefit(
        const std::vector<std::shared_ptr<phase100::OptimizationHint>>& hints,
        const CodePattern& pattern);

    // Generate selection report
    std::string generateSelectionReport(
        const AdaptiveSelectionResult& result);

    // Get selection statistics
    int getTotalSelectionsPerformed() const { return totalSelections_; }
    int getOptimalSelectionsCount() const { return optimalSelections_; }
    double getAverageConfidence() const;

private:
    const phase102::Phase102PatternAnalyzer& analyzer_;

    int totalSelections_ = 0;
    int optimalSelections_ = 0;
    std::vector<double> confidenceValues_;

    // Helper methods
    bool isConflicting(const std::shared_ptr<phase100::OptimizationHint>& h1,
                      const std::shared_ptr<phase100::OptimizationHint>& h2);
    double calculateSynergyBonus(
        const std::vector<std::shared_ptr<phase100::OptimizationHint>>& hints);
    std::string formatSelectionRow(
        const std::shared_ptr<phase100::OptimizationHint>& hint,
        double confidence);
};

} // namespace phase102
