#include "OptimizationPatternAnalyzer.hpp"
#include "OptimizationTypeUtils.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>

OptimizationPatternAnalyzer::OptimizationPatternAnalyzer()
    : profileDatabase_(nullptr), thresholdAdjuster_(nullptr) {
}

OptimizationPatternAnalyzer::~OptimizationPatternAnalyzer() = default;

void OptimizationPatternAnalyzer::analyzePatterns() {
    if (!profileDatabase_) {
        return;
    }

    patterns_.clear();

    // Get all profiles
    auto profiles = profileDatabase_->getAllProfiles();

    // For each optimization, analyze contextual patterns
    for (const auto& profile : profiles) {
        analyzeContextualEffectiveness(profile.optimizationName);
    }
}

std::vector<OptimizationPattern>
OptimizationPatternAnalyzer::getPatternsForOptimization(
    const std::string& optimizationName) const {

    std::vector<OptimizationPattern> result;
    for (const auto& p : patterns_) {
        if (p.optimizationName == optimizationName) {
            result.push_back(p);
        }
    }
    return result;
}

std::vector<OptimizationPattern>
OptimizationPatternAnalyzer::getAllPatterns() const {
    return patterns_;
}

std::vector<OptimizationPattern>
OptimizationPatternAnalyzer::getPatternsForContext(
    const OptimizationContext& context) const {

    std::vector<OptimizationPattern> result;
    for (const auto& p : patterns_) {
        if (p.context.fileSize == context.fileSize
            && p.context.complexity == context.complexity) {
            result.push_back(p);
        }
    }
    return result;
}

OptimizationPattern OptimizationPatternAnalyzer::getContextualPattern(
    const std::string& optimizationName,
    const OptimizationContext& context) const {

    for (const auto& p : patterns_) {
        if (p.optimizationName == optimizationName
            && p.context.fileSize == context.fileSize
            && p.context.complexity == context.complexity) {
            return p;
        }
    }

    // Return empty pattern if not found
    OptimizationPattern empty;
    empty.optimizationName = optimizationName;
    empty.confidence = 0;
    empty.averageEffectiveness = 0.0;
    empty.recommendation = "No data for this context";
    return empty;
}

void OptimizationPatternAnalyzer::analyzeFileSizeImpact() {
    // Analyze patterns based on file size
    detectFileSizePatterns();
}

void OptimizationPatternAnalyzer::analyzeComplexityImpact() {
    // Analyze patterns based on complexity
    detectComplexityPatterns();
}

FileCharacteristics OptimizationPatternAnalyzer::classifyFile(
    const std::string& sourceFile,
    long fileSize,
    int functionCount,
    int loopCount,
    int branchDensity) const {

    FileCharacteristics fc;
    fc.fileSize = fileSize;
    fc.functionCount = functionCount;
    fc.loopCount = loopCount;
    fc.branchDensity = branchDensity;

    // Classify file size
    if (fileSize < 1000) {
        fc.fileSizeCategory = OptimizationContext::TINY;
    } else if (fileSize < 5000) {
        fc.fileSizeCategory = OptimizationContext::SMALL;
    } else if (fileSize < 20000) {
        fc.fileSizeCategory = OptimizationContext::MEDIUM;
    } else if (fileSize < 100000) {
        fc.fileSizeCategory = OptimizationContext::LARGE;
    } else {
        fc.fileSizeCategory = OptimizationContext::HUGE;
    }

    // Classify complexity
    int complexity = functionCount + loopCount / 2 + branchDensity / 10;
    if (complexity < 10) {
        fc.complexityCategory = OptimizationContext::LOW;
    } else if (complexity < 25) {
        fc.complexityCategory = OptimizationContext::MODERATE;
    } else if (complexity < 50) {
        fc.complexityCategory = OptimizationContext::HIGH;
    } else {
        fc.complexityCategory = OptimizationContext::VERY_HIGH;
    }

    return fc;
}

std::string OptimizationPatternAnalyzer::generateReport() const {
    std::stringstream ss;
    ss << "=== Phase 109.4 Pattern Analyzer Report ===\n\n";

    ss << "Total Patterns Discovered: " << patterns_.size() << "\n";
    ss << "High Confidence Patterns (80%+): "
       << getHighConfidencePatterns() << "\n\n";

    // Group patterns by optimization
    std::map<std::string, std::vector<OptimizationPattern>> byOpt;
    for (const auto& p : patterns_) {
        byOpt[p.optimizationName].push_back(p);
    }

    ss << "Optimization Patterns:\n";
    ss << "----------------------\n";

    for (const auto& entry : byOpt) {
        ss << "\n" << entry.first << ":\n";

        for (const auto& pattern : entry.second) {
            ss << "  Context: "
               << OptimizationTypeUtils::fileSizeToString(pattern.context.fileSize)
               << " / ";
            // Map MODERATE to MEDIUM for display consistency
            if (pattern.context.complexity == OptimizationContext::Complexity::MODERATE) {
                ss << "MEDIUM";
            } else {
                ss << OptimizationTypeUtils::complexityToString(pattern.context.complexity);
            }
            ss << "\n";

            ss << "    Effectiveness: " << std::fixed
               << std::setprecision(1)
               << (pattern.averageEffectiveness * 100.0) << "%\n";
            ss << "    Data Points: " << pattern.dataPointCount << "\n";
            ss << "    Confidence: " << pattern.confidence << "%\n";
            ss << "    Recommendation: " << pattern.recommendation << "\n";
        }
    }

    return ss.str();
}

void OptimizationPatternAnalyzer::analyzeContextualEffectiveness(
    const std::string& optimizationName) {

    if (!profileDatabase_) {
        return;
    }

    auto profile = profileDatabase_->getProfile(optimizationName);

    if (profile.totalApplications < 3) {
        return;  // Not enough data
    }

    // Analyze across all contexts
    // (TINY to HUGE) × (LOW to VERY_HIGH)
    std::vector<OptimizationContext::FileSize> fileSizes = {
        OptimizationContext::TINY,
        OptimizationContext::SMALL,
        OptimizationContext::MEDIUM,
        OptimizationContext::LARGE,
        OptimizationContext::HUGE};

    std::vector<OptimizationContext::Complexity> complexities = {
        OptimizationContext::LOW,
        OptimizationContext::MODERATE,
        OptimizationContext::HIGH,
        OptimizationContext::VERY_HIGH};

    for (auto fileSize : fileSizes) {
        for (auto complexity : complexities) {
            OptimizationContext context;
            context.fileSize = fileSize;
            context.complexity = complexity;
            context.loopCount = 0;
            context.branchDensity = 0;

            auto effectiveness = profileDatabase_->getContextualEffectiveness(
                optimizationName, context);

            if (effectiveness.applicationCount < 2) {
                continue;  // Not enough data for this context
            }

            // Build pattern
            OptimizationPattern pattern;
            pattern.optimizationName = optimizationName;
            pattern.context = context;
            pattern.averageEffectiveness = effectiveness.successRate;
            pattern.dataPointCount = effectiveness.applicationCount;
            pattern.confidence = computeConfidence(
                effectiveness.applicationCount);
            pattern.recommendation = generateRecommendation(
                pattern.averageEffectiveness, pattern.confidence);

            // Build condition string
            std::stringstream ss;
            ss << OptimizationTypeUtils::fileSizeToString(fileSize) << " file, ";
            // Map MODERATE to MEDIUM for display consistency
            if (complexity == OptimizationContext::Complexity::MODERATE) {
                ss << "MEDIUM";
            } else {
                ss << OptimizationTypeUtils::complexityToString(complexity);
            }
            ss << " complexity";
            pattern.condition = ss.str();

            patterns_.push_back(pattern);
        }
    }
}

std::string OptimizationPatternAnalyzer::classifyEffectiveness(
    double avgEffectiveness) const {

    if (avgEffectiveness > 0.80) return "Highly Effective";
    if (avgEffectiveness > 0.60) return "Effective";
    if (avgEffectiveness > 0.40) return "Moderately Effective";
    if (avgEffectiveness > 0.20) return "Rarely Effective";
    return "Ineffective";
}

int OptimizationPatternAnalyzer::computeConfidence(int dataPoints) const {
    if (dataPoints < 3) return 20;
    if (dataPoints < 5) return 40;
    if (dataPoints < 10) return 60;
    if (dataPoints < 20) return 80;
    return 100;
}

void OptimizationPatternAnalyzer::detectFileSizePatterns() {
    // Additional pattern analysis by file size
    // This is called by analyzeFileSizeImpact()
}

void OptimizationPatternAnalyzer::detectComplexityPatterns() {
    // Additional pattern analysis by complexity
    // This is called by analyzeComplexityImpact()
}

std::string OptimizationPatternAnalyzer::generateRecommendation(
    double avgEffectiveness,
    int confidence) const {

    if (confidence < 40) {
        return "Insufficient data - use default";
    }

    if (avgEffectiveness > 0.80) {
        return "Always apply - highly effective";
    }

    if (avgEffectiveness > 0.60) {
        return "Apply by default - effective";
    }

    if (avgEffectiveness > 0.40) {
        return "Apply selectively - moderate effectiveness";
    }

    if (avgEffectiveness > 0.20) {
        return "Apply only if budget permits - rarely effective";
    }

    return "Skip - ineffective in this context";
}

std::string OptimizationPatternAnalyzer::generatePatternRecommendation(
    double effectiveness,
    int confidence) const {

    return generateRecommendation(effectiveness, confidence);
}
