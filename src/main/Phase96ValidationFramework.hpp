#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>

// Phase 96.5.5: Validation Framework
// Comprehensive testing and benchmarking for Phase 96 extended striped arrays

namespace phase96_5 {

// Test result for a single test program
struct TestResult {
    std::string testName;
    bool passed;
    std::string errorMessage;
    int bytesCompiled;
    int bytesOptimized;
    double savingsPercent;
    std::chrono::milliseconds compilationTime;
    std::string assemblyFile;
    std::string objectFile;
};

// Benchmark result for performance measurement
struct BenchmarkResult {
    std::string benchmarkName;
    std::string description;
    int executionCount;
    double estimatedCycles;
    double actualCycles;      // From MEGA65 if available
    double speedupPercent;
    double codeReductionPercent;
    bool isHardwareValidated;
};

// Regression test for backward compatibility
struct RegressionTest {
    std::string testName;
    std::string testFile;
    bool passesWithoutOptimization;
    bool passesWithOptimization;
    bool regressionDetected;
    std::string failureReason;
};

// Phase 96.5.5 Validation Framework
class Phase96ValidationFramework {
public:
    Phase96ValidationFramework();
    ~Phase96ValidationFramework() = default;

    // Test execution phase
    void compileTestProgram(const std::string& sourceFile,
                            const std::string& testName);

    bool validateCompilationOutput(const std::string& testName);

    bool validateAssemblyQuality(const std::string& assemblyFile,
                                  const std::string& testName);

    // Regression testing
    void runRegressionTest(const std::string& testFile,
                          const std::string& testName);

    // Benchmarking phase
    void benchmarkCodeSize(const std::string& programName,
                          const std::string& withOptimization,
                          const std::string& withoutOptimization);

    void benchmarkPerformance(const std::string& programName,
                             int expectedCycles,
                             const std::string& description);

    // Validation metrics
    double computeTotalCodeReduction() const;
    double computeAverageCachingBenefit() const;
    int getTestsPassedCount() const;
    int getTestsFailedCount() const;

    // Query results
    std::vector<TestResult> getTestResults() const;
    std::vector<BenchmarkResult> getBenchmarkResults() const;
    std::vector<RegressionTest> getRegressionTests() const;

    // Hardware validation (if MEGA65 available)
    bool hasHardwareAccess() const;
    void setHardwareAccess(bool available);

    void validateOnHardware(const std::string& testName);

    // Report generation
    void generateValidationReport(const std::string& outputFile);
    void generateBenchmarkReport(const std::string& outputFile);
    void generateRegressionReport(const std::string& outputFile);
    void printSummary(std::ostream& out) const;

    // Statistics
    double getTotalEstimatedSavings() const;
    double getActualCodeReduction() const;
    int getTotalBytesOptimized() const;

    void clear();

private:
    // Test data
    std::vector<TestResult> testResults;
    std::vector<BenchmarkResult> benchmarkResults;
    std::vector<RegressionTest> regressionTests;

    bool hardwareAvailable;
    int testsRun;
    int testsPassed;
    int testsFailed;

    // Helper methods
    bool compileProgram(const std::string& sourceFile,
                        const std::string& outputFile);

    bool validateAssemblyFormat(const std::string& assemblyFile) const;
    bool validateObjectFormat(const std::string& objectFile) const;

    double estimateCycleSavings(const std::string& programName) const;
    double measureActualCycles(const std::string& programName) const;

    void analyzeAssemblyQuality(const std::string& assemblyFile,
                               TestResult& result);
};

// Benchmarking suite for comprehensive performance testing
class Phase96BenchmarkSuite {
public:
    Phase96BenchmarkSuite();
    ~Phase96BenchmarkSuite() = default;

    // Benchmark scenarios
    void benchmarkSequentialFieldAccess();
    void benchmarkLoopNestedAccess();
    void benchmarkFieldReuse();
    void benchmarkPointerDereference();
    void benchmarkAliasChains();
    void benchmarkMultiTUCoordination();

    // Run all benchmarks
    void runAllBenchmarks();

    // Get results
    std::vector<BenchmarkResult> getResults() const;
    double getTotalSpeedup() const;
    double getAverageSavings() const;

    // Report
    void generateSummaryReport(std::ostream& out) const;

private:
    std::vector<BenchmarkResult> results;

    // Individual benchmark methods
    void runBenchmark(const std::string& name,
                     const std::string& description,
                     int expectedCycles);
};

} // namespace phase96_5
