#include "LearnerBasedOptimizationSelector.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>

LearnerBasedOptimizationSelector::LearnerBasedOptimizationSelector()
    : totalRecommendations_(0), accurateRecommendations_(0) {
}

LearnerBasedOptimizationSelector::~LearnerBasedOptimizationSelector() = default;

OptimizationStrategy LearnerBasedOptimizationSelector::getOptimizationStrategy(
    long fileSize,
    int functionCount,
    int loopCount,
    int branchDensity) {

    OptimizationStrategy strategy;
    strategy.recommendations.clear();

    // Classify the file
    FileCharacteristics fileChar;
    if (patternAnalyzer_) {
        fileChar = patternAnalyzer_->classifyFile(
            "unknown", fileSize, functionCount, loopCount, branchDensity);
    } else {
        fileChar.fileSize = fileSize;
        fileChar.functionCount = functionCount;
        fileChar.loopCount = loopCount;
        fileChar.branchDensity = branchDensity;
    }

    // Build context
    OptimizationContext context;
    context.fileSize = fileChar.fileSizeCategory;
    context.complexity = fileChar.complexityCategory;
    context.loopCount = loopCount;
    context.branchDensity = branchDensity;

    // Select optimization level
    strategy.optimizationLevel = selectOptimizationLevel(fileChar);

    // Get all profiles and generate recommendations
    if (profileDatabase_) {
        auto profiles = profileDatabase_->getAllProfiles();

        for (const auto& profile : profiles) {
            auto rec = generateRecommendation(
                profile.optimizationName, context);
            strategy.recommendations.push_back(rec);
        }
    }

    // Calculate expected benefits
    calculateExpectedBenefits(strategy);

    // Generate summary
    strategy.summary = generateStrategySummary(strategy);

    totalRecommendations_ += strategy.recommendations.size();

    return strategy;
}

OptimizationRecommendation LearnerBasedOptimizationSelector::getRecommendationFor(
    const std::string& optimizationName,
    const OptimizationContext& context) {

    return generateRecommendation(optimizationName, context);
}

void LearnerBasedOptimizationSelector::learnFromCompilation(
    const FileCharacteristics& fileChar,
    const OptimizationStrategy& appliedStrategy,
    int actualAssemblySize,
    double actualCompileTime) {

    // Simple learning: track accuracy of recommendations
    // (Full implementation would update profiles and patterns)
    int correct = 0;
    for (const auto& rec : appliedStrategy.recommendations) {
        if (rec.recommended && actualAssemblySize < 5000) {
            correct++;
        }
    }

    if (correct > 0) {
        accurateRecommendations_ += correct;
    }
}

std::string LearnerBasedOptimizationSelector::generateReport() const {
    std::stringstream ss;
    ss << "=== Phase 109.5 Adaptive Selector Report ===\n\n";

    ss << "Recommendations Made: " << totalRecommendations_ << "\n";
    ss << "Accurate Recommendations: " << accurateRecommendations_ << "\n";
    ss << "Accuracy: " << std::fixed << std::setprecision(1)
       << (getRecommendationAccuracy() * 100.0) << "%\n\n";

    return ss.str();
}

int LearnerBasedOptimizationSelector::selectOptimizationLevel(
    const FileCharacteristics& fileChar) const {

    // Strategy: Select optimization level based on file size
    // Small files: -O1 (minimal optimization to reduce compile time)
    // Medium files: -O2 (balanced)
    // Large files: -O3 (aggressive optimization)

    switch (fileChar.fileSizeCategory) {
        case OptimizationContext::TINY:
        case OptimizationContext::SMALL:
            return 1;  // -O1

        case OptimizationContext::MEDIUM:
            return 2;  // -O2

        case OptimizationContext::LARGE:
        case OptimizationContext::HUGE:
            return 3;  // -O3

        default:
            return 2;  // Default to -O2
    }
}

OptimizationRecommendation LearnerBasedOptimizationSelector::generateRecommendation(
    const std::string& optimizationName,
    const OptimizationContext& context) {

    OptimizationRecommendation rec;
    rec.optimizationName = optimizationName;
    rec.recommended = false;
    rec.effectivenessScore = 0.0;
    rec.compilationCost = 0.0;
    rec.expectedBenefit = 0.0;
    rec.confidenceScore = 0;
    rec.rationale = "Insufficient data";

    // Get pattern analysis
    OptimizationPattern pattern;
    if (patternAnalyzer_) {
        pattern = patternAnalyzer_->getContextualPattern(
            optimizationName, context);

        if (pattern.confidence > 0) {
            rec.effectivenessScore = pattern.averageEffectiveness;
            rec.confidenceScore = pattern.confidence;
            rec.rationale = pattern.recommendation;
        }
    }

    // Get threshold adjustment
    ThresholdAdjustment adjustment;
    if (thresholdAdjuster_) {
        adjustment = thresholdAdjuster_->getAdjustment(optimizationName);
    }

    // Score the recommendation
    rec.expectedBenefit = scoreRecommendation(pattern, adjustment, 100.0);

    // Decision: recommend if effectiveness score > adjusted threshold
    if (thresholdAdjuster_) {
        rec.recommended = thresholdAdjuster_->shouldApplyOptimization(
            optimizationName, rec.effectivenessScore);
    } else {
        rec.recommended = rec.effectivenessScore > 0.5;
    }

    // Confidence should be >= 60% to recommend
    if (rec.confidenceScore < 60) {
        rec.recommended = false;
        rec.rationale = "Low confidence - insufficient data";
    }

    return rec;
}

double LearnerBasedOptimizationSelector::scoreRecommendation(
    const OptimizationPattern& pattern,
    const ThresholdAdjustment& adjustment,
    double compilationBudget) const {

    // Score = effectiveness * confidence - compilation cost
    double score = pattern.averageEffectiveness
                 * (pattern.confidence / 100.0)
                 - (adjustment.currentThreshold * 0.1);

    return std::max(0.0, score);
}

std::string LearnerBasedOptimizationSelector::generateStrategySummary(
    const OptimizationStrategy& strategy) const {

    std::stringstream ss;
    ss << "Optimization Level: -O" << strategy.optimizationLevel << ", ";

    int enabledCount = 0;
    for (const auto& rec : strategy.recommendations) {
        if (rec.recommended) enabledCount++;
    }

    ss << enabledCount << "/" << strategy.recommendations.size()
       << " optimizations enabled";

    return ss.str();
}

void LearnerBasedOptimizationSelector::calculateExpectedBenefits(
    OptimizationStrategy& strategy) const {

    double totalSpeedup = 0.0;
    double totalSizeReduction = 0.0;
    double totalCost = 0.0;

    for (const auto& rec : strategy.recommendations) {
        if (rec.recommended) {
            totalSpeedup += rec.effectivenessScore * 2.0;  // Assume 2% per opt
            totalSizeReduction += rec.effectivenessScore
                                * 1.5;  // Assume 1.5% per opt
            totalCost += rec.compilationCost;
        }
    }

    strategy.expectedSpeedup = std::min(50.0, totalSpeedup);
    strategy.expectedSizeReduction = std::min(30.0, totalSizeReduction);
    strategy.estimatedCompileTime = 50.0 + totalCost;  // Base 50ms

    // Overall confidence
    if (strategy.recommendations.empty()) {
        strategy.confidenceScore = 0.0;
    } else {
        double totalConfidence = 0.0;
        for (const auto& rec : strategy.recommendations) {
            totalConfidence += rec.confidenceScore;
        }
        strategy.confidenceScore =
            totalConfidence / strategy.recommendations.size();
    }
}
