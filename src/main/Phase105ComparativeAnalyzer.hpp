// Phase 105.4: Comparative Analysis
// Analyzes and compares optimization strategies with trend analysis

#pragma once

#include "Phase105MetricsCollector.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace phase105 {

// Regression detection result
struct Regression {
    std::string strategy;
    std::string benchmark;
    double expectedPerformance = 0.0;
    double actualPerformance = 0.0;
    double regressionPercent = 0.0;  // Negative value
};

// Comparison between two strategies
struct StrategyComparison {
    std::string baselineStrategy;
    std::string optimizedStrategy;

    double speedupDifference = 0.0;       // % absolute difference
    double sizeReductionDifference = 0.0; // % absolute difference
    double compileTimeRatio = 0.0;        // Multiplier (1.0 = same)

    int benchmarksWhere1stBetter = 0;
    int benchmarksWhere2ndBetter = 0;
    int benchmarksEqual = 0;

    std::string recommendation;           // Which strategy to prefer
};

// Trend data for a metric over multiple runs
struct MetricTrend {
    std::string metricName;
    std::vector<double> values;
    double trend = 0.0;                   // Slope: positive = improving
    double volatility = 0.0;              // Standard deviation
    bool isImproving = false;
};

// Comparative analyzer coordinator
class Phase105ComparativeAnalyzer {
public:
    Phase105ComparativeAnalyzer() = default;

    // Compare two optimization strategies
    StrategyComparison compareStrategies(
        const std::string& baseline,
        const std::string& optimized,
        const std::vector<ExecutionResult>& results);

    // Compare all pairs of strategies
    std::vector<StrategyComparison> compareAllStrategies(
        const std::vector<ExecutionResult>& results);

    // Analyze trends in metrics over time
    MetricTrend analyzeTrend(
        const std::string& metricName,
        const std::vector<double>& measurements);

    // Identify best strategy per benchmark
    struct BenchmarkBestStrategy {
        std::string benchmarkName;
        std::string bestStrategy;
        double achievedSpeedup = 0.0;
        double achievedSizeReduction = 0.0;
    };

    std::vector<BenchmarkBestStrategy> identifyBestStrategies(
        const std::vector<ExecutionResult>& results);

    // Regression detection: identify strategies that regressed
    std::vector<Regression> detectRegressions(
        const std::vector<ExecutionResult>& results,
        double regressionThresholdPercent = 5.0);

    // Generate comparative analysis report
    std::string generateComparativeReport(
        const std::vector<StrategyComparison>& comparisons,
        const std::vector<BenchmarkBestStrategy>& best);

    // Get analysis statistics
    int getTotalComparisons() const { return totalComparisons_; }
    double getAverageSpeedupImprovement() const { return avgSpeedupImprovement_; }

private:
    int totalComparisons_ = 0;
    double avgSpeedupImprovement_ = 0.0;

    // Helper methods
    std::string formatComparisonRow(const StrategyComparison& comp);
    std::string formatTrendRow(const MetricTrend& trend);
    std::string formatRegressionRow(const Regression& reg);
};

} // namespace phase105
