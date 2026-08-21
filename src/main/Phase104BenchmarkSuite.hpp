// Phase 104: Real-World Benchmarking Suite
// Comprehensive benchmark collection and execution framework

#pragma once

#include "Phase103DependencyTracker.hpp"
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <memory>

namespace phase104 {

// Benchmark program specification
struct BenchmarkProgram {
    std::string name;              // Benchmark identifier
    std::string description;       // Human-readable description
    std::string sourceFile;        // Path to .c source
    std::string category;          // "loops", "structs", "memory", "mixed"
    int expectedSize = 0;          // Baseline size in bytes
    int expectedCycles = 0;        // Baseline cycles (estimated)
    std::vector<std::string> optimizationPhases;  // Which phases apply
    int priority = 0;              // Execution priority (higher = first)
};

// Benchmark execution result
struct BenchmarkResult {
    std::string programName;
    std::string compiledSize = 0;     // Actual compiled size
    int compiledCycles = 0;           // Actual cycles (simulated)
    std::chrono::milliseconds compileTime{0};
    double sizeReduction = 0.0;       // % reduction vs baseline
    double speedup = 0.0;             // % speedup vs baseline
    bool passed = false;
    std::string output;               // Program output/logs
    std::vector<std::string> appliedOptimizations;
};

// Benchmark suite coordinator
class Phase104BenchmarkSuite {
public:
    Phase104BenchmarkSuite() = default;

    // Define benchmark programs
    void defineBenchmarks();

    // Add custom benchmark
    void addBenchmark(const BenchmarkProgram& program);

    // Get all benchmarks
    const std::vector<BenchmarkProgram>& getBenchmarks() const {
        return benchmarks_;
    }

    // Get benchmark by name
    const BenchmarkProgram* findBenchmark(const std::string& name) const;

    // Get benchmarks by category
    std::vector<BenchmarkProgram> getBenchmarksByCategory(
        const std::string& category) const;

    // Run single benchmark
    BenchmarkResult runBenchmark(const BenchmarkProgram& program);

    // Run all benchmarks
    std::vector<BenchmarkResult> runAllBenchmarks();

    // Run benchmarks by category
    std::vector<BenchmarkResult> runBenchmarksByCategory(
        const std::string& category);

    // Generate comprehensive report
    std::string generateBenchmarkReport(
        const std::vector<BenchmarkResult>& results);

    // Get statistics
    int getTotalBenchmarks() const { return benchmarks_.size(); }
    int getCompletedBenchmarks() const { return completedBenchmarks_; }
    double getAverageSizeReduction() const;
    double getAverageSpeedup() const;

private:
    std::vector<BenchmarkProgram> benchmarks_;
    std::vector<BenchmarkResult> results_;
    int completedBenchmarks_ = 0;

    // Helper methods
    void createLoopBenchmarks();
    void createStructBenchmarks();
    void createMemoryBenchmarks();
    void createMixedBenchmarks();
    BenchmarkResult simulateBenchmarkRun(const BenchmarkProgram& program);
    std::string formatResultRow(const BenchmarkResult& result);
};

} // namespace phase104
