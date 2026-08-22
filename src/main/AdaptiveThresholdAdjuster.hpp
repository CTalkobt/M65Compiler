#pragma once
#include "OptimizationProfileDatabase.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>

// Threshold adjustment strategy
struct ThresholdAdjustment {
    std::string optimizationName;
    double currentThreshold;
    double recommendedThreshold;
    double confidenceScore;     // 0-100
    std::string rationale;      // "High success rate", "Low effectiveness", etc.
    bool shouldApply;           // Whether to apply this adjustment
};

// Phase 109.3: Threshold Adjuster
// Dynamically adjusts optimization thresholds based on effectiveness data
class AdaptiveThresholdAdjuster {
public:
    AdaptiveThresholdAdjuster();
    ~AdaptiveThresholdAdjuster();

    // Initialize with profile database
    void setProfileDatabase(
        const std::shared_ptr<OptimizationProfileDatabase>& db) {
        profileDatabase_ = db;
    }

    // Analyze trends and compute recommended adjustments
    void analyzeTrends();

    // Get recommended threshold for an optimization
    double getRecommendedThreshold(
        const std::string& optimizationName) const;

    // Get all recommended adjustments
    std::vector<ThresholdAdjustment> getAllAdjustments() const;

    // Get single adjustment
    ThresholdAdjustment getAdjustment(
        const std::string& optimizationName) const;

    // Apply threshold to a decision (score >= threshold means apply)
    bool shouldApplyOptimization(
        const std::string& optimizationName,
        double effectivenessScore) const;

    // Get adjustment rationale
    std::string getAdjustmentRationale(
        const std::string& optimizationName) const;

    // Statistics
    int getTotalAdjustmentsComputed() const {
        return adjustments_.size();
    }

    int getAppliedAdjustments() const {
        int count = 0;
        for (const auto& adj : adjustments_) {
            if (adj.second.shouldApply) count++;
        }
        return count;
    }

    // Report generation
    std::string generateReport() const;

private:
    std::shared_ptr<OptimizationProfileDatabase> profileDatabase_;
    std::map<std::string, ThresholdAdjustment> adjustments_;

    // Strategy: Compute recommended threshold based on success rate
    // > 85% success: Reduce threshold by 15-25% (apply more aggressively)
    // 70-85% success: Reduce threshold by 5-10%
    // 50-70% success: Keep threshold stable
    // 30-50% success: Increase threshold by 10-20% (apply more conservatively)
    // < 30% success: Increase threshold by 25-40% (rarely apply)
    double computeRecommendedThreshold(
        const OptimizationProfile& profile,
        double currentThreshold) const;

    // Generate human-readable rationale
    std::string generateRationale(
        const OptimizationProfile& profile,
        double successRate) const;

    // Compute confidence score based on data points
    int computeConfidenceScore(
        const OptimizationProfile& profile) const;

    // Confidence scoring: more data = higher confidence
    // 0-4 apps: 20% confidence
    // 5-9 apps: 40% confidence
    // 10-19 apps: 60% confidence
    // 20-49 apps: 80% confidence
    // 50+ apps: 100% confidence
};
