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

    // For each applied optimization, record its actual effectiveness
    for (const auto& optName : metrics.appliedOptimizations) {
        auto speedIt = metrics.actualSpeedup.find(optName);
        auto sizeIt = metrics.actualSizeReduction.find(optName);

        if (speedIt != metrics.actualSpeedup.end()) {
            learner_->updateOptimizationEffectiveness(optName, speedIt->second);
        }
    }
}

void LearnerFeedbackRecorder::analyzeEffectiveness(
    const CompilationMetrics& metrics) {

    if (!learner_) return;

    // Collect signals about what worked for this compilation
    // These signals feed back into future compilations of similar files

    // Signal: file size vs optimization effectiveness
    phase107::CompilationSignal fileSizeSignal;
    fileSizeSignal.signalType = "file_size";
    fileSizeSignal.value = metrics.sourceFile.length();  // source file size
    fileSizeSignal.phase = "feedback";
    fileSizeSignal.priority = 3;

    learner_->collectSignal(fileSizeSignal);

    // Signal: optimization effectiveness
    double avgEffectiveness = 0.0;
    if (!metrics.appliedOptimizations.empty()) {
        double totalEffectiveness = 0.0;
        int beneficialCount = 0;

        for (const auto& optName : metrics.appliedOptimizations) {
            auto it = metrics.optimizationSuccessful.find(optName);
            if (it != metrics.optimizationSuccessful.end() && it->second) {
                beneficialCount++;
            }
        }

        avgEffectiveness = static_cast<double>(beneficialCount) /
                          metrics.appliedOptimizations.size() * 100.0;
    }

    phase107::CompilationSignal effectivenessSignal;
    effectivenessSignal.signalType = "optimization_effectiveness";
    effectivenessSignal.value = avgEffectiveness;
    effectivenessSignal.phase = "feedback";
    effectivenessSignal.priority = 7;  // High priority

    learner_->collectSignal(effectivenessSignal);
}
