// Phase 105.1: Compiler Integration for Benchmarking
// Hooks into cc45 compilation pipeline to emit profiling data

#pragma once

#include "Phase104BenchmarkSuite.hpp"
#include "Phase101Profiler.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include <functional>

namespace phase105 {

// Delta between two compilations
struct CompilationDelta {
    double sizeReduction = 0.0;      // % reduction
    double speedupEstimate = 0.0;    // % faster (estimated)
    double compileTimeChange = 0.0;  // % change in compile time
    int bytesRemoved = 0;
    int cyclesSaved = 0;
    std::vector<std::string> optimizationsBenefit;
};

// Compilation metrics captured during a benchmark run
struct CompilationMetrics {
    std::string programName;
    std::string sourceFile;

    // Timing
    std::chrono::milliseconds totalCompileTime{0};
    std::chrono::milliseconds lexerTime{0};
    std::chrono::milliseconds parserTime{0};
    std::chrono::milliseconds validationTime{0};
    std::chrono::milliseconds optimizationTime{0};
    std::chrono::milliseconds codegenTime{0};
    std::chrono::milliseconds assemblationTime{0};

    // Code metrics
    size_t sourceLines = 0;
    size_t functionCount = 0;
    size_t loopCount = 0;
    size_t callSiteCount = 0;
    size_t structTypeCount = 0;

    // Output metrics
    size_t assemblyLines = 0;
    size_t binarySize = 0;
    size_t symbolCount = 0;
    size_t relocationCount = 0;

    // Optimization metrics
    std::vector<std::string> appliedOptimizations;
    int constantsEliminated = 0;
    int deadCodeEliminated = 0;
    int inlinedFunctions = 0;
    int loopsUnrolled = 0;
};

// Compiler interface for capturing metrics
class Phase105CompilerIntegration {
public:
    Phase105CompilerIntegration() = default;

    // Initialize compiler hooks for profiling
    void initializeCompilerHooks();

    // Register optimization phase callback
    void registerOptimizationCallback(const std::string& phaseName,
        std::function<void(const std::string&)> callback);

    // Compile program with metrics capture
    CompilationMetrics compileProgramWithMetrics(
        const std::string& sourceFile,
        const std::string& outputFile,
        const std::vector<std::string>& optimizationFlags);

    // Get metrics for last compilation
    const CompilationMetrics& getLastMetrics() const { return lastMetrics_; }

    // Compare two compilations (baseline vs optimized)
    CompilationDelta compareCompilations(
        const CompilationMetrics& baseline,
        const CompilationMetrics& optimized);

    // Generate compilation report
    std::string generateCompilationReport(
        const std::vector<CompilationMetrics>& metrics);

    // Get compiler integration statistics
    int getTotalCompilations() const { return totalCompilations_; }
    double getAverageCompileTime() const;
    double getAverageBinarySize() const;

private:
    CompilationMetrics lastMetrics_;
    std::vector<CompilationMetrics> allMetrics_;
    int totalCompilations_ = 0;

    // Optimization callbacks
    std::map<std::string, std::function<void(const std::string&)>> callbacks_;

    // Helper methods
    void captureTimingMetrics(CompilationMetrics& metrics);
    void captureCodeMetrics(const std::string& sourceFile,
        CompilationMetrics& metrics);
    void captureOutputMetrics(const std::string& binaryFile,
        CompilationMetrics& metrics);
    std::string formatMetricsReport(const CompilationMetrics& metrics);
};

} // namespace phase105
