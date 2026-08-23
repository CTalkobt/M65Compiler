#include "LearnerFeedbackRecorder.hpp"
#include "OnlineLearner.hpp"
#include <algorithm>
#include <cmath>

void LearnerFeedbackRecorder::recordMetrics(const CompilationMetrics& metrics) {
    history_.push_back(metrics);
}

FeedbackResult LearnerFeedbackRecorder::finalizeCompilation(
    const CompilationMetrics& metrics) {

    FeedbackResult result;
    result.success = true;

    if (!learner_) {
        result.error = "No learner available for feedback recording";
        return result;
    }

    // Record optimization results to learner
    recordOptimizationResults(metrics);

    // Analyze what worked
    analyzeEffectiveness(metrics);

    // Update statistics
    result.optimizationsApplied = metrics.appliedOptimizations.size();
    int beneficialCount = 0;
    double totalBenefit = 0.0;

    for (const auto& optName : metrics.appliedOptimizations) {
        auto it = metrics.optimizationSuccessful.find(optName);
        if (it != metrics.optimizationSuccessful.end() && it->second) {
            beneficialCount++;

            // Compute benefit for this optimization
            auto speedIt = metrics.actualSpeedup.find(optName);
            auto sizeIt = metrics.actualSizeReduction.find(optName);
            double speedup = (speedIt != metrics.actualSpeedup.end()) ? speedIt->second : 0.0;
            double sizeRed = (sizeIt != metrics.actualSizeReduction.end()) ? sizeIt->second : 0.0;

            totalBenefit += computeOptimizationBenefit(optName, speedup, sizeRed);
        }
    }

    result.optimizationsBeneficial = beneficialCount;
    result.overallBenefitScore = (result.optimizationsApplied > 0) ?
        (totalBenefit / result.optimizationsApplied) : 0.0;

    // Update running average
    if (history_.size() > 0) {
        avgOverallBenefit_ = (avgOverallBenefit_ * (history_.size() - 1) +
                             result.overallBenefitScore) / history_.size();
    }

    return result;
}

double LearnerFeedbackRecorder::computeOptimizationBenefit(
    const std::string& optName,
    double speedup,
    double sizeReduction) {

    // Benefit score: weighted combination of speedup and size reduction
    // Speedup counts for 40%, size reduction for 60% (typical embedded/space-constrained)
    const double speedupWeight = 0.4;
    const double sizeWeight = 0.6;

    double benefit = (speedup * speedupWeight) + (sizeReduction * sizeWeight);
    return std::max(0.0, benefit);
}

void LearnerFeedbackRecorder::recordOptimizationResults(
    const CompilationMetrics& metrics) {

    if (!learner_) return;

    // Phase 113 stub: Record optimization effectiveness to learner
    // Full learner integration deferred to Phase 113
    // For each applied optimization, record its actual effectiveness
    for (const auto& optName : metrics.appliedOptimizations) {
        auto speedIt = metrics.actualSpeedup.find(optName);
        auto sizeIt = metrics.actualSizeReduction.find(optName);

        if (speedIt != metrics.actualSpeedup.end()) {
            // Learner update deferred - OnlineLearner header not included
            // This will be implemented in Phase 113
        }
    }
}

void LearnerFeedbackRecorder::analyzeEffectiveness(
    const CompilationMetrics& metrics) {

    if (!learner_) return;

    // Phase 113 stub: Deferred effectiveness analysis
    // Full signal collection deferred to Phase 113 implementation
}
