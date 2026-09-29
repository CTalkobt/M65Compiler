// Phase 100.6: Link-Time Code Optimization Validation & Benchmarking
// Validates correctness and measures performance benefits of coordinated hints

#pragma once

#include "LinkTimeOptimizationCodeGenerator.hpp"
#include <string>
#include <vector>
#include <map>
#include <chrono>

namespace phase100 {

// Benchmark result for a single optimization
struct OptimizationBenchmark {
    std::string hintPhase;           // "91", "96.5", "99"
    std::string hintType;            // Specific hint name
    std::string targetName;          // Function or loop name

    // Metrics
    int originalSize = 0;            // Size without optimization
    int optimizedSize = 0;           // Size with optimization
    int estimatedBenefit = 0;        // Computed benefit
    int measuredBenefit = 0;         // Actual benefit observed
    double benefitRatio = 0.0;       // optimizedSize / originalSize (< 1.0)

    // Performance
    std::chrono::milliseconds appliedTime{0};  // Time to apply hint
    double accuracyPercent = 0.0;    // Accuracy of benefit estimation (%)
};

// Validation result for a coordinated hint set
struct CoordinationValidation {
    std::vector<std::shared_ptr<OptimizationHint>> hints;
    bool isValid = false;
    std::string validationNote;

    // Combined metrics
    int combinedBenefitEstimate = 0;
    int combinedBenefitMeasured = 0;
    double synergyBonus = 0.0;       // Actual vs. sum of individual
};

// Link-time code optimizer validator
class LinkTimeOptimizationValidator {
public:
    explicit LinkTimeOptimizationValidator(const LinkTimeOptimizationCodeGenerator& codeGen)
        : codeGen_(codeGen) {}

    // Validate coordinated hints produce correct code
    CoordinationValidation validateCoordination(
        const std::vector<std::shared_ptr<OptimizationHint>>& hints);

    // Validate individual hint correctness
    bool validateHint(const std::shared_ptr<OptimizationHint>& hint);

    // Measure benefit of single hint
    OptimizationBenchmark measureHintBenefit(
        const std::shared_ptr<OptimizationHint>& hint);

    // Measure combined benefit of multiple hints
    int measureCombinedBenefit(
        const std::vector<std::shared_ptr<OptimizationHint>>& hints);

    // Verify calling convention safety
    bool verifyCallingConventionSafety(
        const std::vector<std::shared_ptr<OptimizationHint>>& hints);

    // Verify memory access safety
    bool verifyMemorySafety(
        const std::vector<std::shared_ptr<OptimizationHint>>& hints);

    // Generate optimization report
    std::string generateOptimizationReport(
        const std::vector<OptimizationBenchmark>& benchmarks);

    // Statistics
    int getTotalValidHints() const { return validHintsCount_; }
    int getTotalInvalidHints() const { return invalidHintsCount_; }
    int getTotalMeasuredBenefit() const { return totalMeasuredBenefit_; }
    double getAverageSynergy() const;

private:
    const LinkTimeOptimizationCodeGenerator& codeGen_;

    int validHintsCount_ = 0;
    int invalidHintsCount_ = 0;
    int totalMeasuredBenefit_ = 0;
    std::vector<OptimizationBenchmark> benchmarks_;
    std::vector<double> synergyValues_;

    // Helper methods
    bool checkHintTargetExists(const std::shared_ptr<OptimizationHint>& hint);
    bool checkHintCompatibility(const std::shared_ptr<OptimizationHint>& h1,
                                const std::shared_ptr<OptimizationHint>& h2);
    int estimateBenefit(const std::shared_ptr<OptimizationHint>& hint);
    int measureActualBenefit(const std::shared_ptr<OptimizationHint>& hint);
    std::string formatBenchmarkRow(const OptimizationBenchmark& bench);
};

} // namespace phase100
