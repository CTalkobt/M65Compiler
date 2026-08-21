// Phase 105.2: Benchmark Executor
// Runs actual benchmarks with real compilation and optimization strategies

#pragma once

#include "Phase105CompilerIntegration.hpp"
#include "Phase102AdaptiveSelector.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace phase105 {

// Execution strategy for benchmark runs
struct ExecutionStrategy {
    std::string strategyName;
    std::vector<std::string> optimizationFlags;
    std::string description;
    bool isBaseline = false;
    int expectedSpeedup = 0;  // Estimated speedup percentage
    int expectedSizeReduction = 0;  // Estimated size reduction percentage
};

// Results from a single benchmark execution
struct ExecutionResult {
    std::string benchmarkName;
    ExecutionStrategy strategy;
    CompilationMetrics metrics;
    bool succeeded = false;
    std::string errorMessage;
    double actualSpeedup = 0.0;
    double actualSizeReduction = 0.0;
};

// Benchmark executor coordinator
class Phase105BenchmarkExecutor {
public:
    Phase105BenchmarkExecutor(Phase105CompilerIntegration& compiler,
                              phase102::Phase102AdaptiveSelector& selector)
        : compiler_(compiler), adaptiveSelector_(selector) {}

    // Define execution strategies
    void defineStrategies();

    // Add custom strategy
    void addStrategy(const ExecutionStrategy& strategy);

    // Get all strategies
    const std::vector<ExecutionStrategy>& getStrategies() const {
        return strategies_;
    }

    // Execute single benchmark with strategy
    ExecutionResult executeBenchmark(
        const phase104::BenchmarkProgram& program,
        const ExecutionStrategy& strategy);

    // Execute benchmark with all strategies
    std::vector<ExecutionResult> executeBenchmarkWithAllStrategies(
        const phase104::BenchmarkProgram& program);

    // Execute all benchmarks
    std::vector<ExecutionResult> executeAllBenchmarks(
        const std::vector<phase104::BenchmarkProgram>& benchmarks);

    // Generate execution report
    std::string generateExecutionReport(
        const std::vector<ExecutionResult>& results);

    // Get execution statistics
    int getTotalExecutions() const { return totalExecutions_; }
    int getSuccessfulExecutions() const { return successfulExecutions_; }
    double getSuccessRate() const;
    double getAverageSpeedup() const;

private:
    Phase105CompilerIntegration& compiler_;
    phase102::Phase102AdaptiveSelector& adaptiveSelector_;
    std::vector<ExecutionStrategy> strategies_;
    std::vector<ExecutionResult> allResults_;
    int totalExecutions_ = 0;
    int successfulExecutions_ = 0;

    // Helper methods
    void createBaselineStrategy();
    void createOptimizedStrategies();
    void createAdaptiveStrategies();
    std::string formatExecutionRow(const ExecutionResult& result);
};

} // namespace phase105
