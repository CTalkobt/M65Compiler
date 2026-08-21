#pragma once
#include "Phase109ProfileDatabase.hpp"
#include "Phase109ThresholdAdjuster.hpp"
#include "Phase109PatternAnalyzer.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>

// Optimization recommendation for a file
struct OptimizationRecommendation {
    std::string optimizationName;
    bool recommended;              // Should this optimization be applied?
    double effectivenessScore;     // Expected effectiveness (0.0-1.0)
    double compilationCost;        // Expected time cost (ms)
    double expectedBenefit;        // Expected speedup + size reduction
    int confidenceScore;           // Confidence in this recommendation (0-100)
    std::string rationale;         // Why this recommendation
};

// Per-file optimization strategy
struct OptimizationStrategy {
    int optimizationLevel;         // -O0 to -O9
    std::vector<OptimizationRecommendation> recommendations;
    double expectedSpeedup;        // Total expected speedup %
    double expectedSizeReduction;  // Total expected size reduction %
    double estimatedCompileTime;   // Expected compilation time (ms)
    double confidenceScore;        // Overall strategy confidence (0-100)
    std::string summary;           // Human-readable summary
};

// Phase 109.5: Adaptive Selector
// Makes per-file optimization recommendations based on learned patterns
class Phase109AdaptiveSelector {
public:
    Phase109AdaptiveSelector();
    ~Phase109AdaptiveSelector();

    // Initialize with all sub-components
    void setProfileDatabase(
        const std::shared_ptr<Phase109ProfileDatabase>& db) {
        profileDatabase_ = db;
    }

    void setThresholdAdjuster(
        const std::shared_ptr<Phase109ThresholdAdjuster>& adj) {
        thresholdAdjuster_ = adj;
    }

    void setPatternAnalyzer(
        const std::shared_ptr<Phase109PatternAnalyzer>& analyzer) {
        patternAnalyzer_ = analyzer;
    }

    // Get optimization strategy for a file
    OptimizationStrategy getOptimizationStrategy(
        long fileSize,
        int functionCount,
        int loopCount,
        int branchDensity);

    // Get single recommendation for optimization
    OptimizationRecommendation getRecommendationFor(
        const std::string& optimizationName,
        const OptimizationContext& context);

    // Learn from compilation result
    void learnFromCompilation(
        const FileCharacteristics& fileChar,
        const OptimizationStrategy& appliedStrategy,
        int actualAssemblySize,
        double actualCompileTime);

    // Statistics
    int getTotalRecommendationsMade() const {
        return totalRecommendations_;
    }

    int getAccurateRecommendations() const {
        return accurateRecommendations_;
    }

    double getRecommendationAccuracy() const {
        if (totalRecommendations_ == 0) return 0.0;
        return static_cast<double>(accurateRecommendations_)
             / totalRecommendations_;
    }

    // Report generation
    std::string generateReport() const;

private:
    std::shared_ptr<Phase109ProfileDatabase> profileDatabase_;
    std::shared_ptr<Phase109ThresholdAdjuster> thresholdAdjuster_;
    std::shared_ptr<Phase109PatternAnalyzer> patternAnalyzer_;

    int totalRecommendations_;
    int accurateRecommendations_;

    // Helper: Select optimization level based on file characteristics
    int selectOptimizationLevel(
        const FileCharacteristics& fileChar) const;

    // Helper: Generate recommendation for one optimization
    OptimizationRecommendation generateRecommendation(
        const std::string& optimizationName,
        const OptimizationContext& context);

    // Helper: Score recommendation based on multiple factors
    double scoreRecommendation(
        const OptimizationPattern& pattern,
        const ThresholdAdjustment& adjustment,
        double compilationBudget) const;

    // Helper: Generate strategy summary
    std::string generateStrategySummary(
        const OptimizationStrategy& strategy) const;

    // Helper: Calculate expected benefits
    void calculateExpectedBenefits(
        OptimizationStrategy& strategy) const;
};
