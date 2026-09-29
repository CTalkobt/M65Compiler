#pragma once
#include "OptimizationEffectivenessCollector.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>

// Context for optimization effectiveness
struct OptimizationContext {
    enum FileSize { TINY, SMALL, MEDIUM, LARGE, HUGE };
    enum Complexity { LOW, MODERATE, HIGH, VERY_HIGH };

    FileSize fileSize;
    Complexity complexity;
    int loopCount;
    int branchDensity;
};

// Effectiveness in a specific context
struct ContextualEffectiveness {
    OptimizationContext context;
    int applicationCount;
    int successCount;
    double successRate;
    double avgSpeedup;
    double avgSizeReduction;
    double avgCost;
    double benefitScore;
};

// Optimization profile with contextual data
struct OptimizationProfile {
    std::string optimizationName;

    // Global statistics
    int totalApplications;
    int totalSuccesses;
    double globalSuccessRate;
    double globalAvgSpeedup;
    double globalAvgSizeReduction;
    double globalAvgCost;
    double globalBenefitScore;

    // Contextual effectiveness
    std::vector<ContextualEffectiveness> contextualEffectiveness;

    // Trend analysis
    struct Trend {
        int dataPointCount;      // How many compilations
        double successRateTrend;  // Direction of trend
        double benefitTrend;
        double confidenceScore;   // 0-100 based on data points
    };
    Trend trend;

    // Recommendation
    std::string recommendation;  // "Always apply", "Apply if", "Skip if", etc.
};

// Profile database
class OptimizationProfileDatabase {
public:
    OptimizationProfileDatabase();
    ~OptimizationProfileDatabase();

    // Record a compilation result
    void recordCompilation(
        const CompilationRecord& record,
        const std::vector<OptimizationApplication>& applications);

    // Get profile for an optimization
    OptimizationProfile getProfile(
        const std::string& optimizationName) const;

    // Get all profiles
    std::vector<OptimizationProfile> getAllProfiles() const;

    // Get contextual effectiveness
    ContextualEffectiveness getContextualEffectiveness(
        const std::string& optimizationName,
        const OptimizationContext& context) const;

    // Analyze trends (improvement/degradation over time)
    void analyzeTrends();

    // Generate recommendations based on profiles
    std::string generateRecommendation(
        const std::string& optimizationName,
        const OptimizationProfile& profile) const;

    // Statistics
    int getTotalCompilationsRecorded() const {
        return totalCompilations_;
    }

    int getTotalOptimizations() const {
        return optimizationProfiles_.size();
    }

    // Confidence scoring
    int getConfidenceScore(const std::string& optimizationName) const;

    // Persistence
    void saveToFile(const std::string& filepath);
    void loadFromFile(const std::string& filepath);

    // Report generation
    std::string generateReport() const;

private:
    int totalCompilations_;
    std::map<std::string, std::vector<OptimizationApplication>>
        optimizationApplications_;
    std::map<std::string, OptimizationProfile> optimizationProfiles_;
    std::vector<CompilationRecord> compilationRecords_;

    // Helper: classify file context
    OptimizationContext classifyContext(
        const CompilationRecord& record) const;

    // Helper: compute global effectiveness
    OptimizationProfile computeGlobalEffectiveness(
        const std::string& optimizationName) const;

    // Helper: compute contextual effectiveness
    ContextualEffectiveness computeContextualEffectiveness(
        const std::string& optimizationName,
        const OptimizationContext& context) const;

    // Helper: compute trend
    OptimizationProfile::Trend computeTrend(
        const std::string& optimizationName) const;

    // Helper: generate JSON
    std::string profileToJson(const OptimizationProfile& profile) const;
    OptimizationProfile profileFromJson(const std::string& json) const;
};
