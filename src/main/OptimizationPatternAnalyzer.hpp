#pragma once
#include "OptimizationProfileDatabase.hpp"
#include "AdaptiveThresholdAdjuster.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>

// Detected pattern in optimization effectiveness
struct OptimizationPattern {
    std::string optimizationName;
    std::string condition;           // "file_size == SMALL", "complexity >= HIGH"
    OptimizationContext context;     // The specific context
    double averageEffectiveness;     // Avg success rate
    int dataPointCount;              // How many compilations in this pattern
    int confidence;                  // 0-100 based on data points
    std::string recommendation;      // "Always apply", "Avoid", "Apply selectively"
};

// File characteristics for pattern matching
struct FileCharacteristics {
    long fileSize;
    int functionCount;
    int loopCount;
    int branchDensity;
    OptimizationContext::FileSize fileSizeCategory;
    OptimizationContext::Complexity complexityCategory;
};

// Phase 109.4: Pattern Analyzer
// Identifies patterns in optimization effectiveness based on file characteristics
class OptimizationPatternAnalyzer {
public:
    OptimizationPatternAnalyzer();
    ~OptimizationPatternAnalyzer();

    // Initialize with profile database and threshold adjuster
    void setProfileDatabase(
        const std::shared_ptr<OptimizationProfileDatabase>& db) {
        profileDatabase_ = db;
    }

    void setThresholdAdjuster(
        const std::shared_ptr<AdaptiveThresholdAdjuster>& adj) {
        thresholdAdjuster_ = adj;
    }

    // Analyze all patterns
    void analyzePatterns();

    // Get patterns for an optimization
    std::vector<OptimizationPattern> getPatternsForOptimization(
        const std::string& optimizationName) const;

    // Get all patterns
    std::vector<OptimizationPattern> getAllPatterns() const;

    // Get patterns for a specific context
    std::vector<OptimizationPattern> getPatternsForContext(
        const OptimizationContext& context) const;

    // Get recommendation for optimization in specific context
    OptimizationPattern getContextualPattern(
        const std::string& optimizationName,
        const OptimizationContext& context) const;

    // Analyze file size impact on optimization
    void analyzeFileSizeImpact();

    // Analyze complexity impact on optimization
    void analyzeComplexityImpact();

    // Generate file characteristics from source analysis
    FileCharacteristics classifyFile(
        const std::string& sourceFile,
        long fileSize,
        int functionCount,
        int loopCount,
        int branchDensity) const;

    // Statistics
    int getTotalPatternsDiscovered() const {
        return patterns_.size();
    }

    int getHighConfidencePatterns() const {
        int count = 0;
        for (const auto& p : patterns_) {
            if (p.confidence >= 80) count++;
        }
        return count;
    }

    // Report generation
    std::string generateReport() const;

    // Pattern discovery helper
    std::string generatePatternRecommendation(
        double effectiveness,
        int confidence) const;

private:
    std::shared_ptr<OptimizationProfileDatabase> profileDatabase_;
    std::shared_ptr<AdaptiveThresholdAdjuster> thresholdAdjuster_;
    std::vector<OptimizationPattern> patterns_;

    // Helper: analyze effectiveness in specific context
    void analyzeContextualEffectiveness(
        const std::string& optimizationName);

    // Helper: classify effectiveness level
    std::string classifyEffectiveness(
        double avgEffectiveness) const;

    // Helper: compute confidence from data points
    int computeConfidence(int dataPoints) const;

    // Pattern detection helpers
    void detectFileSizePatterns();
    void detectComplexityPatterns();

    // Helper: generate recommendation based on effectiveness
    std::string generateRecommendation(
        double avgEffectiveness,
        int confidence) const;
};
