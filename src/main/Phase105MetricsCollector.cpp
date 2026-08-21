// Phase 105.3: Metrics Collector Implementation
#include "Phase105MetricsCollector.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <numeric>
#include <cmath>

namespace phase105 {

AggregateMetrics Phase105MetricsCollector::collectMetrics(
    const std::vector<ExecutionResult>& results) {

    AggregateMetrics metrics;
    metrics.totalExecutions = results.size();

    std::vector<double> speedups;
    std::vector<double> reductions;
    std::vector<double> compileTimes;
    std::vector<double> binarySizes;

    for (const auto& result : results) {
        if (result.succeeded) {
            metrics.successfulExecutions++;
            speedups.push_back(result.actualSpeedup);
            reductions.push_back(result.actualSizeReduction);
            compileTimes.push_back(result.metrics.totalCompileTime.count());
            binarySizes.push_back(result.metrics.binarySize);
        }
    }

    metrics.successRate = metrics.totalExecutions > 0 ?
        (double)metrics.successfulExecutions / metrics.totalExecutions : 0.0;

    // Speedup statistics
    if (!speedups.empty()) {
        std::sort(speedups.begin(), speedups.end());
        metrics.minSpeedup = speedups.front();
        metrics.maxSpeedup = speedups.back();
        metrics.avgSpeedup = std::accumulate(speedups.begin(),
                                           speedups.end(), 0.0) / speedups.size();
        metrics.medianSpeedup = computeMedian(speedups);
    }

    // Size reduction statistics
    if (!reductions.empty()) {
        std::sort(reductions.begin(), reductions.end());
        metrics.minSizeReduction = reductions.front();
        metrics.maxSizeReduction = reductions.back();
        metrics.avgSizeReduction = std::accumulate(reductions.begin(),
                                                  reductions.end(), 0.0) / reductions.size();
        metrics.medianSizeReduction = computeMedian(reductions);
    }

    // Compile time and binary size averages
    if (!compileTimes.empty()) {
        metrics.avgCompileTimeMs = std::accumulate(compileTimes.begin(),
                                                  compileTimes.end(), 0.0) / compileTimes.size();
    }
    if (!binarySizes.empty()) {
        metrics.avgBinarySizeBytes = std::accumulate(binarySizes.begin(),
                                                    binarySizes.end(), 0.0) / binarySizes.size();
    }

    // Per-optimization metrics
    std::map<std::string, std::vector<ExecutionResult>> byOpt;
    for (const auto& result : results) {
        for (const auto& opt : result.strategy.optimizationFlags) {
            byOpt[opt].push_back(result);
        }
    }

    for (const auto& [optName, optResults] : byOpt) {
        metrics.optimizationStats.push_back(
            computeOptimizationMetrics(optName, optResults));
    }

    allAggregates_.push_back(metrics);
    totalCollections_++;

    return metrics;
}

OptimizationMetrics Phase105MetricsCollector::computeOptimizationMetrics(
    const std::string& optimizationName,
    const std::vector<ExecutionResult>& results) {

    OptimizationMetrics metrics;
    metrics.optimizationName = optimizationName;
    metrics.applicationsCount = results.size();

    double sumSpeedup = 0.0;
    double sumReduction = 0.0;
    int successCount = 0;

    for (const auto& result : results) {
        if (result.succeeded) {
            sumSpeedup += result.actualSpeedup;
            sumReduction += result.actualSizeReduction;
            successCount++;
            metrics.affectedBenchmarks.push_back(result.benchmarkName);
        }
    }

    metrics.avgSpeedup = metrics.applicationsCount > 0 ?
        sumSpeedup / metrics.applicationsCount : 0.0;
    metrics.avgSizeReduction = metrics.applicationsCount > 0 ?
        sumReduction / metrics.applicationsCount : 0.0;
    metrics.successRate = metrics.applicationsCount > 0 ?
        (double)successCount / metrics.applicationsCount : 0.0;

    return metrics;
}

Phase105MetricsCollector::BenchmarkMetrics Phase105MetricsCollector::computeBenchmarkMetrics(
    const std::string& benchmarkName,
    const std::vector<ExecutionResult>& results) {

    BenchmarkMetrics metrics;
    metrics.benchmarkName = benchmarkName;

    std::vector<ExecutionResult> benchResults;
    for (const auto& result : results) {
        if (result.benchmarkName == benchmarkName) {
            benchResults.push_back(result);
        }
    }

    metrics.executionCount = benchResults.size();

    double sumSpeedup = 0.0;
    double sumReduction = 0.0;
    double maxSpeedup = 0.0;

    for (const auto& result : benchResults) {
        if (result.succeeded) {
            sumSpeedup += result.actualSpeedup;
            sumReduction += result.actualSizeReduction;

            if (result.actualSpeedup > maxSpeedup) {
                maxSpeedup = result.actualSpeedup;
                if (!result.strategy.strategyName.empty()) {
                    metrics.bestStrategies.clear();
                    metrics.bestStrategies.push_back(result.strategy.strategyName);
                }
            }
        }
    }

    metrics.avgSpeedup = metrics.executionCount > 0 ?
        sumSpeedup / metrics.executionCount : 0.0;
    metrics.avgSizeReduction = metrics.executionCount > 0 ?
        sumReduction / metrics.executionCount : 0.0;

    return metrics;
}

std::vector<Phase105MetricsCollector::OptimizationCorrelation>
Phase105MetricsCollector::analyzeOptimizationSynergy(
    const std::vector<ExecutionResult>& results) {

    std::vector<OptimizationCorrelation> correlations;

    // Map strategy names to their component optimizations
    std::map<std::string, std::vector<std::string>> strategyOpts;
    std::map<std::pair<std::string, std::string>, std::vector<double>> synergySamples;

    for (const auto& result : results) {
        if (result.succeeded && !result.strategy.isBaseline) {
            strategyOpts[result.strategy.strategyName] = result.strategy.optimizationFlags;
        }
    }

    // Analyze pairwise synergy
    for (const auto& [strat1, opts1] : strategyOpts) {
        for (const auto& [strat2, opts2] : strategyOpts) {
            if (strat1 < strat2) {  // Avoid duplicates
                // Count common optimizations
                int commonCount = 0;
                for (const auto& o1 : opts1) {
                    if (std::find(opts2.begin(), opts2.end(), o1) != opts2.end()) {
                        commonCount++;
                    }
                }

                if (commonCount > 0) {
                    OptimizationCorrelation corr;
                    corr.opt1 = strat1;
                    corr.opt2 = strat2;
                    corr.synergy = commonCount * 5.0;  // Synergy bonus per shared opt
                    corr.samples = commonCount;
                    correlations.push_back(corr);
                }
            }
        }
    }

    return correlations;
}

std::string Phase105MetricsCollector::generateMetricsReport(
    const AggregateMetrics& metrics) {

    std::stringstream report;

    report << "╔════════════════════════════════════════════════════════════════╗\n";
    report << "║     Phase 105.3: Metrics Collection Report                     ║\n";
    report << "║     Comprehensive Performance Measurement Analysis             ║\n";
    report << "╚════════════════════════════════════════════════════════════════╝\n\n";

    report << "COLLECTION SUMMARY\n";
    report << "──────────────────────────────────────────────────────────────────\n";
    report << "Total Executions:           " << metrics.totalExecutions << "\n";
    report << "Successful:                 " << metrics.successfulExecutions << "\n";
    report << "Success Rate:               " << std::fixed << std::setprecision(1)
           << (metrics.successRate * 100.0) << "%\n\n";

    report << "SPEEDUP STATISTICS (% improvement)\n";
    report << "──────────────────────────────────────────────────────────────────\n";
    report << "Minimum:                    " << std::fixed << std::setprecision(1)
           << metrics.minSpeedup << "%\n";
    report << "Maximum:                    " << metrics.maxSpeedup << "%\n";
    report << "Average:                    " << metrics.avgSpeedup << "%\n";
    report << "Median:                     " << metrics.medianSpeedup << "%\n\n";

    report << "SIZE REDUCTION STATISTICS (% reduction)\n";
    report << "──────────────────────────────────────────────────────────────────\n";
    report << "Minimum:                    " << std::fixed << std::setprecision(1)
           << metrics.minSizeReduction << "%\n";
    report << "Maximum:                    " << metrics.maxSizeReduction << "%\n";
    report << "Average:                    " << metrics.avgSizeReduction << "%\n";
    report << "Median:                     " << metrics.medianSizeReduction << "%\n\n";

    report << "COMPILATION OVERHEAD\n";
    report << "──────────────────────────────────────────────────────────────────\n";
    report << "Average Compile Time:       " << std::fixed << std::setprecision(0)
           << metrics.avgCompileTimeMs << " ms\n";
    report << "Average Binary Size:        " << std::fixed << std::setprecision(0)
           << metrics.avgBinarySizeBytes << " bytes\n\n";

    report << "PER-OPTIMIZATION METRICS\n";
    report << "──────────────────────────────────────────────────────────────────\n";
    report << "Optimization         Count  Avg Speedup  Avg Reduction  Success\n";
    report << "──────────────────────────────────────────────────────────────────\n";

    for (const auto& opt : metrics.optimizationStats) {
        report << formatMetricsRow(opt);
    }

    report << "\n";
    return report.str();
}

double Phase105MetricsCollector::computeMedian(std::vector<double> values) {
    if (values.empty()) return 0.0;

    std::sort(values.begin(), values.end());
    size_t mid = values.size() / 2;

    if (values.size() % 2 == 0) {
        return (values[mid - 1] + values[mid]) / 2.0;
    } else {
        return values[mid];
    }
}

std::string Phase105MetricsCollector::formatMetricsRow(
    const OptimizationMetrics& metrics) {

    std::stringstream row;

    row << std::left << std::setw(20) << metrics.optimizationName
        << "  " << std::right << std::setw(5) << metrics.applicationsCount
        << "  " << std::setw(11) << std::fixed << std::setprecision(1)
        << metrics.avgSpeedup << "%  "
        << std::setw(13) << metrics.avgSizeReduction << "%  "
        << std::setw(7) << (metrics.successRate * 100.0) << "%\n";

    return row.str();
}

std::string Phase105MetricsCollector::formatCorrelationRow(
    const OptimizationCorrelation& corr) {

    std::stringstream row;

    row << std::left << std::setw(15) << corr.opt1
        << "  " << std::setw(15) << corr.opt2
        << "  " << std::right << std::setw(8) << std::fixed << std::setprecision(1)
        << corr.synergy << "%  "
        << std::setw(7) << corr.samples << "\n";

    return row.str();
}

} // namespace phase105
