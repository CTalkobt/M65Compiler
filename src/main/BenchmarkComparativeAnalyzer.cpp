// Phase 105.4: Comparative Analysis Implementation
#include "BenchmarkComparativeAnalyzer.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <numeric>
#include <cmath>
#include <set>

namespace phase105 {

StrategyComparison BenchmarkComparativeAnalyzer::compareStrategies(
    const std::string& baseline,
    const std::string& optimized,
    const std::vector<ExecutionResult>& results) {

    StrategyComparison comp;
    comp.baselineStrategy = baseline;
    comp.optimizedStrategy = optimized;

    std::vector<ExecutionResult> baselineResults, optimizedResults;

    for (const auto& result : results) {
        if (result.strategy.strategyName == baseline) {
            baselineResults.push_back(result);
        } else if (result.strategy.strategyName == optimized) {
            optimizedResults.push_back(result);
        }
    }

    // Compute average metrics
    double baselineAvgSpeedup = 0.0;
    double optimizedAvgSpeedup = 0.0;
    double baselineAvgReduction = 0.0;
    double optimizedAvgReduction = 0.0;
    double baselineAvgTime = 0.0;
    double optimizedAvgTime = 0.0;

    for (const auto& r : baselineResults) {
        if (r.succeeded) {
            baselineAvgSpeedup += r.actualSpeedup;
            baselineAvgReduction += r.actualSizeReduction;
            baselineAvgTime += r.metrics.totalCompileTime.count();
        }
    }

    for (const auto& r : optimizedResults) {
        if (r.succeeded) {
            optimizedAvgSpeedup += r.actualSpeedup;
            optimizedAvgReduction += r.actualSizeReduction;
            optimizedAvgTime += r.metrics.totalCompileTime.count();
        }
    }

    if (!baselineResults.empty()) {
        baselineAvgSpeedup /= baselineResults.size();
        baselineAvgReduction /= baselineResults.size();
        baselineAvgTime /= baselineResults.size();
    }

    if (!optimizedResults.empty()) {
        optimizedAvgSpeedup /= optimizedResults.size();
        optimizedAvgReduction /= optimizedResults.size();
        optimizedAvgTime /= optimizedResults.size();
    }

    comp.speedupDifference = optimizedAvgSpeedup - baselineAvgSpeedup;
    comp.sizeReductionDifference = optimizedAvgReduction - baselineAvgReduction;

    if (baselineAvgTime > 0) {
        comp.compileTimeRatio = optimizedAvgTime / baselineAvgTime;
    }

    // Compare per benchmark
    std::map<std::string, std::pair<double, double>> benchmarkComparison;

    for (const auto& r : baselineResults) {
        benchmarkComparison[r.benchmarkName].first = r.actualSpeedup;
    }

    for (const auto& r : optimizedResults) {
        benchmarkComparison[r.benchmarkName].second = r.actualSpeedup;
    }

    for (const auto& [bench, speedups] : benchmarkComparison) {
        if (speedups.second > speedups.first) {
            comp.benchmarksWhere2ndBetter++;
        } else if (speedups.second < speedups.first) {
            comp.benchmarksWhere1stBetter++;
        } else {
            comp.benchmarksEqual++;
        }
    }

    // Generate recommendation
    if (comp.speedupDifference > 5.0 && comp.compileTimeRatio < 1.5) {
        comp.recommendation = "Prefer " + optimized;
    } else if (comp.speedupDifference < -5.0 || comp.compileTimeRatio > 2.0) {
        comp.recommendation = "Prefer " + baseline;
    } else {
        comp.recommendation = "Comparable trade-offs";
    }

    totalComparisons_++;
    avgSpeedupImprovement_ = (avgSpeedupImprovement_ * (totalComparisons_ - 1) +
                              comp.speedupDifference) / totalComparisons_;

    return comp;
}

std::vector<StrategyComparison> BenchmarkComparativeAnalyzer::compareAllStrategies(
    const std::vector<ExecutionResult>& results) {

    // Extract unique strategy names
    std::set<std::string> strategies;
    for (const auto& result : results) {
        strategies.insert(result.strategy.strategyName);
    }

    std::vector<StrategyComparison> comparisons;

    // Compare all pairs
    std::vector<std::string> stratList(strategies.begin(), strategies.end());
    for (size_t i = 0; i < stratList.size(); ++i) {
        for (size_t j = i + 1; j < stratList.size(); ++j) {
            comparisons.push_back(compareStrategies(
                stratList[i], stratList[j], results));
        }
    }

    return comparisons;
}

MetricTrend BenchmarkComparativeAnalyzer::analyzeTrend(
    const std::string& metricName,
    const std::vector<double>& measurements) {

    MetricTrend trend;
    trend.metricName = metricName;
    trend.values = measurements;

    if (measurements.size() < 2) return trend;

    // Compute linear regression slope (simple)
    double sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;
    for (size_t i = 0; i < measurements.size(); ++i) {
        double x = i;
        double y = measurements[i];
        sumX += x;
        sumY += y;
        sumXY += x * y;
        sumX2 += x * x;
    }

    double n = measurements.size();
    if (n * sumX2 - sumX * sumX != 0) {
        trend.trend = (n * sumXY - sumX * sumY) / (n * sumX2 - sumX * sumX);
    }

    // Compute volatility (standard deviation)
    double mean = sumY / n;
    double sumSqDiff = 0;
    for (const auto& val : measurements) {
        sumSqDiff += (val - mean) * (val - mean);
    }
    trend.volatility = std::sqrt(sumSqDiff / n);

    trend.isImproving = trend.trend > 0.1;  // Threshold for improvement

    return trend;
}

std::vector<BenchmarkComparativeAnalyzer::BenchmarkBestStrategy>
BenchmarkComparativeAnalyzer::identifyBestStrategies(
    const std::vector<ExecutionResult>& results) {

    std::map<std::string, std::vector<ExecutionResult>> byBenchmark;
    for (const auto& result : results) {
        byBenchmark[result.benchmarkName].push_back(result);
    }

    std::vector<BenchmarkBestStrategy> best;

    for (const auto& [benchName, benchResults] : byBenchmark) {
        BenchmarkBestStrategy b;
        b.benchmarkName = benchName;

        double maxSpeedup = -1.0;
        for (const auto& result : benchResults) {
            if (result.succeeded && result.actualSpeedup > maxSpeedup) {
                maxSpeedup = result.actualSpeedup;
                b.bestStrategy = result.strategy.strategyName;
                b.achievedSpeedup = result.actualSpeedup;
                b.achievedSizeReduction = result.actualSizeReduction;
            }
        }

        if (!b.bestStrategy.empty()) {
            best.push_back(b);
        }
    }

    return best;
}

std::vector<Regression>
BenchmarkComparativeAnalyzer::detectRegressions(
    const std::vector<ExecutionResult>& results,
    double regressionThresholdPercent) {

    std::vector<Regression> regressions;

    // Group by strategy and benchmark
    std::map<std::string, std::map<std::string, ExecutionResult>> byStrategyBench;
    for (const auto& result : results) {
        if (result.succeeded) {
            byStrategyBench[result.strategy.strategyName][result.benchmarkName] = result;
        }
    }

    // For each strategy, compare against baseline
    auto it = byStrategyBench.find("baseline");
    if (it == byStrategyBench.end()) {
        it = byStrategyBench.find("O0");  // Fallback baseline
    }

    if (it != byStrategyBench.end()) {
        const auto& baseline = it->second;

        for (const auto& [stratName, stratResults] : byStrategyBench) {
            if (stratName == "baseline" || stratName == "O0") continue;

            for (const auto& [benchName, baselineResult] : baseline) {
                auto it2 = stratResults.find(benchName);
                if (it2 != stratResults.end()) {
                    const auto& optimizedResult = it2->second;

                    double regressionPercent = baselineResult.actualSpeedup > 0 ?
                        100.0 * (optimizedResult.actualSpeedup - baselineResult.actualSpeedup) /
                        baselineResult.actualSpeedup : 0.0;

                    if (regressionPercent < -regressionThresholdPercent) {
                        Regression reg;
                        reg.strategy = stratName;
                        reg.benchmark = benchName;
                        reg.expectedPerformance = baselineResult.actualSpeedup;
                        reg.actualPerformance = optimizedResult.actualSpeedup;
                        reg.regressionPercent = regressionPercent;
                        regressions.push_back(reg);
                    }
                }
            }
        }
    }

    return regressions;
}

std::string BenchmarkComparativeAnalyzer::generateComparativeReport(
    const std::vector<StrategyComparison>& comparisons,
    const std::vector<BenchmarkBestStrategy>& best) {

    std::stringstream report;

    report << "╔════════════════════════════════════════════════════════════════╗\n";
    report << "║     Phase 105.4: Comparative Analysis Report                   ║\n";
    report << "║     Strategy Comparison & Optimization Ranking                 ║\n";
    report << "╚════════════════════════════════════════════════════════════════╝\n\n";

    report << "STRATEGY COMPARISONS\n";
    report << "──────────────────────────────────────────────────────────────────\n";
    report << "Baseline           Optimized        Speedup Δ  Reduction Δ  Recommend\n";
    report << "──────────────────────────────────────────────────────────────────\n";

    for (const auto& comp : comparisons) {
        report << formatComparisonRow(comp);
    }

    report << "\nBEST STRATEGIES BY BENCHMARK\n";
    report << "──────────────────────────────────────────────────────────────────\n";
    report << "Benchmark            Best Strategy      Speedup  Size Reduction\n";
    report << "──────────────────────────────────────────────────────────────────\n";

    for (const auto& b : best) {
        report << std::left << std::setw(20) << b.benchmarkName
               << "  " << std::setw(18) << b.bestStrategy
               << "  " << std::right << std::setw(7) << std::fixed << std::setprecision(1)
               << b.achievedSpeedup << "%  "
               << std::setw(14) << b.achievedSizeReduction << "%\n";
    }

    report << "\n";
    return report.str();
}

std::string BenchmarkComparativeAnalyzer::formatComparisonRow(
    const StrategyComparison& comp) {

    std::stringstream row;

    row << std::left << std::setw(18) << comp.baselineStrategy
        << "  " << std::setw(16) << comp.optimizedStrategy
        << "  " << std::right << std::setw(9) << std::fixed << std::setprecision(1)
        << comp.speedupDifference << "%  "
        << std::setw(11) << comp.sizeReductionDifference << "%  "
        << std::left << comp.recommendation << "\n";

    return row.str();
}

std::string BenchmarkComparativeAnalyzer::formatTrendRow(
    const MetricTrend& trend) {

    std::stringstream row;

    row << std::left << std::setw(20) << trend.metricName
        << "  Trend: " << std::fixed << std::setprecision(3) << trend.trend
        << "  Volatility: " << trend.volatility
        << "  " << (trend.isImproving ? "IMPROVING" : "stable") << "\n";

    return row.str();
}

std::string BenchmarkComparativeAnalyzer::formatRegressionRow(
    const Regression& reg) {

    std::stringstream row;

    row << std::left << std::setw(18) << reg.strategy
        << "  " << std::setw(18) << reg.benchmark
        << "  " << std::right << std::setw(8) << std::fixed << std::setprecision(1)
        << reg.expectedPerformance << "  "
        << std::setw(8) << reg.actualPerformance << "  "
        << std::setw(8) << reg.regressionPercent << "%\n";

    return row.str();
}

} // namespace phase105
