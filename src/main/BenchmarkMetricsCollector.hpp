// Phase 105.3: Metrics Collector
// Gathers real-world measurements from benchmark executions

#pragma once

#include "BenchmarkExecutor.hpp"
#include "CompilationProfiler.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace phase105 {

// Performance metrics for a single optimization
struct OptimizationMetrics {
    std::string optimizationName;
    int applicationsCount = 0;
    double avgSpeedup = 0.0;
    double avgSizeReduction = 0.0;
    double successRate = 0.0;
    std::vector<std::string> affectedBenchmarks;
};

// Aggregate metrics across executions
struct AggregateMetrics {
    int totalExecutions = 0;
    int successfulExecutions = 0;
    double successRate = 0.0;

    double minSpeedup = 0.0;
    double maxSpeedup = 0.0;
    double avgSpeedup = 0.0;
    double medianSpeedup = 0.0;

    double minSizeReduction = 0.0;
    double maxSizeReduction = 0.0;
    double avgSizeReduction = 0.0;
    double medianSizeReduction = 0.0;

    double avgCompileTimeMs = 0.0;
    double avgBinarySizeBytes = 0.0;

    std::vector<OptimizationMetrics> optimizationStats;
};

// Metrics collector coordinator
class BenchmarkMetricsCollector {
public:
    BenchmarkMetricsCollector() = default;

    // Collect metrics from execution results
    AggregateMetrics collectMetrics(
        const std::vector<ExecutionResult>& results);

    // Per-optimization metrics
    OptimizationMetrics computeOptimizationMetrics(
        const std::string& optimizationName,
        const std::vector<ExecutionResult>& results);

    // Per-benchmark metrics
    struct BenchmarkMetrics {
        std::string benchmarkName;
        int executionCount = 0;
        double avgSpeedup = 0.0;
        double avgSizeReduction = 0.0;
        std::vector<std::string> bestStrategies;
    };

    BenchmarkMetrics computeBenchmarkMetrics(
        const std::string& benchmarkName,
        const std::vector<ExecutionResult>& results);

    // Correlation analysis: which optimizations work best together?
    struct OptimizationCorrelation {
        std::string opt1;
        std::string opt2;
        double synergy = 0.0;  // % additional benefit when combined
        int samples = 0;
    };

    std::vector<OptimizationCorrelation> analyzeOptimizationSynergy(
        const std::vector<ExecutionResult>& results);

    // Generate metrics report
    std::string generateMetricsReport(const AggregateMetrics& metrics);

    // Get collection statistics
    int getTotalCollections() const { return totalCollections_; }
    double getAverageMetricsQuality() const { return avgQuality_; }

private:
    std::vector<AggregateMetrics> allAggregates_;
    int totalCollections_ = 0;
    double avgQuality_ = 0.0;

    // Helper methods
    double computeMedian(std::vector<double> values);
    std::string formatMetricsRow(const OptimizationMetrics& metrics);
    std::string formatCorrelationRow(const OptimizationCorrelation& corr);
};

} // namespace phase105
