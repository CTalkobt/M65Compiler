// Phase 101.3: Comparative Performance Analysis
// Analyzes optimization effectiveness through detailed comparison reports

#pragma once

#include "Phase101HotSpotDetector.hpp"
#include <string>
#include <vector>
#include <map>
#include <utility>

namespace phase101 {

// Comparison metrics for before/after optimization
struct ComparisonMetrics {
    std::string optimizationName;
    std::string targetFunction;
    std::string targetLoop;

    // Size metrics
    int beforeSize = 0;
    int afterSize = 0;
    int sizeSavings = 0;
    double sizeReductionPercent = 0.0;

    // Performance metrics
    int beforeCycles = 0;
    int afterCycles = 0;
    int cyclesSaved = 0;
    double speedupPercent = 0.0;

    // Quality metrics
    double estimationAccuracy = 0.0;  // vs actual improvement
    int implementationCost = 0;       // Time to implement
    double roi = 0.0;                 // Benefit / cost
    bool isBeneficial = false;
};

// Trend analysis for optimization effectiveness
struct OptimizationTrend {
    std::string optimizationType;     // e.g., "loop_unrolling", "field_caching"
    int totalApplications = 0;
    double averageBenefit = 0.0;
    double minBenefit = 0.0;
    double maxBenefit = 0.0;
    double benefitStdDev = 0.0;
    int successCount = 0;             // Times benefit > 0
    double successRate = 0.0;         // % of beneficial applications
};

// Comparative analysis engine
class Phase101ComparativeAnalyzer {
public:
    explicit Phase101ComparativeAnalyzer(const Phase101HotSpotDetector& detector)
        : detector_(detector) {}

    // Compare optimization effectiveness across different targets
    std::vector<ComparisonMetrics> compareOptimizations(
        const std::vector<PerformanceCounter>& counters);

    // Analyze trends in optimization effectiveness
    std::vector<OptimizationTrend> analyzeTrends(
        const std::vector<PerformanceCounter>& counters);

    // Rank optimizations by effectiveness
    std::vector<std::pair<std::string, double>> rankByEffectiveness(
        const std::vector<ComparisonMetrics>& metrics);

    // Identify synergistic optimizations (work well together)
    std::vector<std::pair<std::string, std::string>> findSynergies(
        const std::vector<ComparisonMetrics>& metrics);

    // Generate comparative performance report
    std::string generateComparativeReport(
        const std::vector<ComparisonMetrics>& metrics);

    // Generate trend analysis report
    std::string generateTrendReport(
        const std::vector<OptimizationTrend>& trends);

    // Calculate optimization effectiveness score
    double calculateEffectivenessScore(const ComparisonMetrics& metric);

    // Predict optimization benefit for new code
    double predictBenefit(const std::string& optimizationType,
                         int targetSize);

private:
    const Phase101HotSpotDetector& detector_;

    std::vector<ComparisonMetrics> allComparisons_;
    std::map<std::string, std::vector<double>> benefitsByType_;

    // Helper methods
    double calculateSpeedup(int beforeCycles, int afterCycles);
    double calculateAccuracy(int estimated, int actual);
    std::string formatMetricRow(const ComparisonMetrics& metric);
    std::string formatTrendRow(const OptimizationTrend& trend);
};

} // namespace phase101
