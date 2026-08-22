#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>

class OnlineLearner;

// Phase 113: Closes the learning feedback loop
// Records actual compilation results and updates learner's effectiveness database

struct CompilationMetrics {
    std::string sourceFile;
    int finalAssemblySize = 0;      // bytes
    double totalCompileTimeMs = 0.0; // milliseconds

    // Optimization application record
    std::vector<std::string> appliedOptimizations;
    std::map<std::string, bool> optimizationSuccessful;  // opt name → was beneficial

    // Early vs actual measurements
    std::map<std::string, double> estimatedSpeedup;      // opt name → % speedup
    std::map<std::string, double> actualSpeedup;         // measured
    std::map<std::string, double> estimatedSizeReduction;
    std::map<std::string, double> actualSizeReduction;

    // Metadata
    int optimizationLevel = 2;
    bool enabledInlining = false;
    bool enabledIROpts = false;
};

// Feedback result from a compilation
struct FeedbackResult {
    bool success = false;
    std::string error;
    double overallBenefitScore = 0.0;
    int optimizationsApplied = 0;
    int optimizationsBeneficial = 0;
};

class LearnerFeedbackRecorder {
public:
    explicit LearnerFeedbackRecorder(OnlineLearner* learner)
        : learner_(learner) {}

    // Record metrics collected during compilation
    void recordMetrics(const CompilationMetrics& metrics);

    // Finalize compilation and record feedback to learner
    FeedbackResult finalizeCompilation(const CompilationMetrics& metrics);

    // Access recorded compilation history
    const std::vector<CompilationMetrics>& getHistory() const { return history_; }

    // Statistics
    int getTotalCompilationsRecorded() const { return history_.size(); }
    double getAverageOverallBenefit() const { return avgOverallBenefit_; }

private:
    OnlineLearner* learner_;
    std::vector<CompilationMetrics> history_;
    double avgOverallBenefit_ = 0.0;

    // Compute benefit score for an optimization
    double computeOptimizationBenefit(
        const std::string& optName,
        double speedup,
        double sizeReduction);

    // Update learner's effectiveness database with actual results
    void recordOptimizationResults(const CompilationMetrics& metrics);

    // Analyze what worked and what didn't
    void analyzeEffectiveness(const CompilationMetrics& metrics);
};
