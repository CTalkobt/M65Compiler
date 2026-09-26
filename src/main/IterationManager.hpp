// Phase 106.1: Iteration Manager
// Orchestrates continuous benchmark runs with iterative optimization refinement

#pragma once

#include "BenchmarkExecutor.hpp"
#include "BenchmarkComparativeAnalyzer.hpp"
#include "BenchmarkReportGenerator.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>

namespace phase106 {

// Single iteration result
struct IterationResult {
    int iterationNumber = 0;
    std::chrono::system_clock::time_point timestamp;

    std::vector<phase105::ExecutionResult> benchmarkResults;
    phase105::AggregateMetrics metrics;
    std::vector<phase105::StrategyComparison> comparisons;

    double avgSpeedup = 0.0;
    double avgSizeReduction = 0.0;
    double improvementOverPrevious = 0.0;  // % improvement from last iteration

    std::string bestStrategy;
    std::vector<std::string> appliedOptimizations;
    bool converged = false;
};

// Configuration for continuous benchmarking
struct BenchmarkLoopConfig {
    int maxIterations = 10;
    int benchmarksPerIteration = 5;
    double convergenceThreshold = 0.5;  // % improvement needed to continue
    double convergenceWindowSize = 3;   // Number of iterations to average for convergence
    bool adaptiveStrategy = true;       // Dynamically select strategies
    bool reportPerIteration = true;
    std::string outputDirectory = "./benchmark_loops";
};

// Iteration manager coordinator
class IterationManager {
public:
    IterationManager(phase105::BenchmarkExecutor& executor,
                            phase105::BenchmarkComparativeAnalyzer& analyzer)
        : executor_(executor), analyzer_(analyzer) {}

    // Run continuous benchmark loop
    std::vector<IterationResult> runBenchmarkLoop(
        const std::vector<phase104::BenchmarkProgram>& benchmarks,
        const BenchmarkLoopConfig& config);

    // Execute single iteration
    IterationResult executeIteration(
        const std::vector<phase104::BenchmarkProgram>& benchmarks,
        int iterationNumber,
        const std::vector<phase105::ExecutionStrategy>& strategies);

    // Compute improvement over previous iteration
    double computeImprovement(const IterationResult& current,
                             const IterationResult& previous);

    // Detect convergence
    bool detectConvergence(const std::vector<IterationResult>& results,
                          double threshold, int windowSize);

    // Select strategies for next iteration (adaptive)
    std::vector<phase105::ExecutionStrategy> selectStrategiesForNextIteration(
        const std::vector<IterationResult>& previousResults,
        const std::vector<phase105::ExecutionStrategy>& availableStrategies);

    // Generate iteration loop report
    std::string generateLoopReport(const std::vector<IterationResult>& results);

    // Generate convergence analysis
    std::string generateConvergenceAnalysis(const std::vector<IterationResult>& results);

    // Get loop statistics
    int getTotalIterations() const { return totalIterations_; }
    double getOverallImprovement() const { return overallImprovement_; }
    bool hasConverged() const { return converged_; }

private:
    phase105::BenchmarkExecutor& executor_;
    phase105::BenchmarkComparativeAnalyzer& analyzer_;

    int totalIterations_ = 0;
    double overallImprovement_ = 0.0;
    bool converged_ = false;
    std::vector<IterationResult> allResults_;

    // Helper methods
    std::string formatIterationRow(const IterationResult& result);
    std::string formatConvergenceRow(int iteration, double improvement);
};

} // namespace phase106
