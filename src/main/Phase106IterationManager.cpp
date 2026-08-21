// Phase 106.1: Iteration Manager Implementation
#include "Phase106IterationManager.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <numeric>
#include <cmath>

using namespace phase105;

namespace phase106 {

std::vector<IterationResult> Phase106IterationManager::runBenchmarkLoop(
    const std::vector<phase104::BenchmarkProgram>& benchmarks,
    const BenchmarkLoopConfig& config) {

    std::vector<IterationResult> results;
    std::vector<phase105::ExecutionStrategy> currentStrategies = executor_.getStrategies();

    IterationResult previous;
    previous.iterationNumber = 0;
    previous.avgSpeedup = 0.0;
    previous.avgSizeReduction = 0.0;

    for (int i = 1; i <= config.maxIterations; ++i) {
        // Select strategies for this iteration
        if (config.adaptiveStrategy && !results.empty()) {
            currentStrategies = selectStrategiesForNextIteration(
                results, executor_.getStrategies());
        }

        // Execute iteration
        IterationResult current = executeIteration(benchmarks, i, currentStrategies);

        // Compute improvement
        current.improvementOverPrevious = computeImprovement(current, previous);

        // Check for convergence
        results.push_back(current);
        if (detectConvergence(results, config.convergenceThreshold,
                             static_cast<int>(config.convergenceWindowSize))) {
            current.converged = true;
            converged_ = true;
            overallImprovement_ = current.avgSpeedup;
            totalIterations_ = i;
            results.back().converged = true;
            break;
        }

        previous = current;
        totalIterations_ = i;
        overallImprovement_ = current.avgSpeedup;
    }

    allResults_ = results;
    return results;
}

IterationResult Phase106IterationManager::executeIteration(
    const std::vector<phase104::BenchmarkProgram>& benchmarks,
    int iterationNumber,
    const std::vector<ExecutionStrategy>& strategies) {

    IterationResult result;
    result.iterationNumber = iterationNumber;
    result.timestamp = std::chrono::system_clock::now();

    // Execute benchmarks with strategies
    Phase105MetricsCollector collector;
    Phase105ReportGenerator generator;

    for (const auto& benchmark : benchmarks) {
        for (const auto& strategy : strategies) {
            result.benchmarkResults.push_back(
                executor_.executeBenchmark(benchmark, strategy));
        }
    }

    // Collect metrics
    result.metrics = collector.collectMetrics(result.benchmarkResults);

    // Compare strategies
    result.comparisons = analyzer_.compareAllStrategies(result.benchmarkResults);

    // Identify best strategy
    auto bestStrats = analyzer_.identifyBestStrategies(result.benchmarkResults);
    if (!bestStrats.empty()) {
        result.bestStrategy = bestStrats[0].bestStrategy;
    }

    result.avgSpeedup = result.metrics.avgSpeedup;
    result.avgSizeReduction = result.metrics.avgSizeReduction;

    return result;
}

double Phase106IterationManager::computeImprovement(
    const IterationResult& current,
    const IterationResult& previous) {

    if (previous.avgSpeedup == 0.0) {
        return current.avgSpeedup;
    }

    return 100.0 * (current.avgSpeedup - previous.avgSpeedup) / previous.avgSpeedup;
}

bool Phase106IterationManager::detectConvergence(
    const std::vector<IterationResult>& results,
    double threshold, int windowSize) {

    if (results.size() < static_cast<size_t>(windowSize)) {
        return false;
    }

    // Look at the last windowSize iterations
    double sumImprovement = 0.0;
    size_t startIdx = results.size() - windowSize;

    for (size_t i = startIdx; i < results.size(); ++i) {
        sumImprovement += results[i].improvementOverPrevious;
    }

    double avgImprovement = sumImprovement / windowSize;
    return avgImprovement < threshold;
}

std::vector<phase105::ExecutionStrategy> Phase106IterationManager::selectStrategiesForNextIteration(
    const std::vector<IterationResult>& previousResults,
    const std::vector<phase105::ExecutionStrategy>& availableStrategies) {

    if (previousResults.empty()) {
        return availableStrategies;
    }

    const auto& lastResult = previousResults.back();
    std::vector<ExecutionStrategy> selected;

    // Always include baseline
    for (const auto& strat : availableStrategies) {
        if (strat.isBaseline) {
            selected.push_back(strat);
            break;
        }
    }

    // Include best performing strategies
    std::vector<std::pair<double, ExecutionStrategy>> ranked;
    for (const auto& comp : lastResult.comparisons) {
        for (const auto& strat : availableStrategies) {
            if (strat.strategyName == comp.optimizedStrategy &&
                comp.speedupDifference > 5.0) {
                ranked.push_back({comp.speedupDifference, strat});
            }
        }
    }

    // Sort by speedup
    std::sort(ranked.begin(), ranked.end(),
        [](const auto& a, const auto& b) {
            return a.first > b.first;
        });

    // Take top 3
    for (size_t i = 0; i < std::min(size_t(3), ranked.size()); ++i) {
        selected.push_back(ranked[i].second);
    }

    return selected;
}

std::string Phase106IterationManager::generateLoopReport(
    const std::vector<IterationResult>& results) {

    std::stringstream report;

    report << "╔════════════════════════════════════════════════════════════════╗\n";
    report << "║     Phase 106.1: Continuous Benchmark Loop Report              ║\n";
    report << "║     Iterative Optimization Refinement & Convergence Analysis   ║\n";
    report << "╚════════════════════════════════════════════════════════════════╝\n\n";

    report << "LOOP SUMMARY\n";
    report << "──────────────────────────────────────────────────────────────────\n";
    report << "Total Iterations:           " << results.size() << "\n";
    report << "Converged:                  " << (converged_ ? "Yes" : "No") << "\n";
    report << "Overall Improvement:        " << std::fixed << std::setprecision(1)
           << overallImprovement_ << "%\n";
    if (!results.empty()) {
        report << "Best Iteration:             " << results.back().iterationNumber << "\n";
        report << "Best Strategy:              " << results.back().bestStrategy << "\n";
    }
    report << "\n";

    report << "ITERATION PROGRESSION\n";
    report << "──────────────────────────────────────────────────────────────────\n";
    report << "Iter  Speedup  Size Red.  Improvement  Converged  Best Strategy\n";
    report << "──────────────────────────────────────────────────────────────────\n";

    for (const auto& result : results) {
        report << formatIterationRow(result);
    }

    report << "\n";
    return report.str();
}

std::string Phase106IterationManager::generateConvergenceAnalysis(
    const std::vector<IterationResult>& results) {

    std::stringstream analysis;

    analysis << "CONVERGENCE ANALYSIS\n";
    analysis << "════════════════════════════════════════════════════════════════\n";
    analysis << "Analyzing optimization effectiveness and convergence behavior\n\n";

    if (results.size() < 2) {
        analysis << "Insufficient iterations for convergence analysis\n";
        return analysis.str();
    }

    // Compute trend
    std::vector<double> speedups;
    std::vector<int> iterations;

    for (size_t i = 0; i < results.size(); ++i) {
        speedups.push_back(results[i].avgSpeedup);
        iterations.push_back(results[i].iterationNumber);
    }

    // Linear regression for trend
    double sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;
    for (size_t i = 0; i < speedups.size(); ++i) {
        double x = iterations[i];
        double y = speedups[i];
        sumX += x;
        sumY += y;
        sumXY += x * y;
        sumX2 += x * x;
    }

    double n = speedups.size();
    double trend = (n * sumXY - sumX * sumY) / (n * sumX2 - sumX * sumX);
    double intercept = (sumY - trend * sumX) / n;

    analysis << "TREND ANALYSIS\n";
    analysis << "──────────────────────────────────────────────────────────────────\n";
    analysis << "Trend Slope:                " << std::fixed << std::setprecision(3)
             << trend << " % speedup per iteration\n";
    analysis << "Trend Direction:            " << (trend > 0.1 ? "IMPROVING" : "CONVERGED") << "\n";
    analysis << "Intercept:                  " << intercept << "%\n\n";

    // Volatility
    double mean = sumY / n;
    double sumSqDiff = 0;
    for (const auto& s : speedups) {
        sumSqDiff += (s - mean) * (s - mean);
    }
    double volatility = std::sqrt(sumSqDiff / n);

    analysis << "VOLATILITY METRICS\n";
    analysis << "──────────────────────────────────────────────────────────────────\n";
    analysis << "Average Speedup:            " << mean << "%\n";
    analysis << "Speedup Volatility (σ):     " << volatility << "%\n";
    analysis << "Coefficient of Variation:   " << (volatility / mean * 100.0) << "%\n\n";

    // Rate of improvement
    if (results.size() >= 3) {
        double firstHalf = std::accumulate(speedups.begin(),
                                          speedups.begin() + speedups.size() / 2,
                                          0.0) / (speedups.size() / 2);
        double secondHalf = std::accumulate(speedups.begin() + speedups.size() / 2,
                                           speedups.end(),
                                           0.0) / (speedups.size() - speedups.size() / 2);

        analysis << "IMPROVEMENT PHASES\n";
        analysis << "──────────────────────────────────────────────────────────────────\n";
        analysis << "First Half Avg:             " << std::fixed << std::setprecision(1)
                 << firstHalf << "%\n";
        analysis << "Second Half Avg:            " << secondHalf << "%\n";
        analysis << "Improvement Rate:           " << ((secondHalf - firstHalf) > 0 ? "Accelerating" : "Decelerating") << "\n";
    }

    analysis << "\n";
    return analysis.str();
}

std::string Phase106IterationManager::formatIterationRow(
    const IterationResult& result) {

    std::stringstream row;

    row << std::right << std::setw(4) << result.iterationNumber
        << "  " << std::setw(7) << std::fixed << std::setprecision(1)
        << result.avgSpeedup << "%"
        << "  " << std::setw(8) << result.avgSizeReduction << "%"
        << "  " << std::setw(11) << result.improvementOverPrevious << "%"
        << "  " << std::left << std::setw(9) << (result.converged ? "Yes" : "No")
        << "  " << result.bestStrategy << "\n";

    return row.str();
}

std::string Phase106IterationManager::formatConvergenceRow(
    int iteration, double improvement) {

    std::stringstream row;

    row << "Iteration " << iteration << ": "
        << std::fixed << std::setprecision(2) << improvement << "% improvement\n";

    return row.str();
}

} // namespace phase106
