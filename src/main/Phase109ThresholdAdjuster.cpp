#include "Phase109ThresholdAdjuster.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>

Phase109ThresholdAdjuster::Phase109ThresholdAdjuster()
    : profileDatabase_(nullptr) {
}

Phase109ThresholdAdjuster::~Phase109ThresholdAdjuster() = default;

void Phase109ThresholdAdjuster::analyzeTrends() {
    if (!profileDatabase_) {
        return;
    }

    adjustments_.clear();

    // Get all profiles from database
    auto profiles = profileDatabase_->getAllProfiles();

    for (const auto& profile : profiles) {
        // Current threshold is 0.5 by default (50% effectiveness score)
        double currentThreshold = 0.5;

        // Compute recommended threshold
        double recommendedThreshold =
            computeRecommendedThreshold(profile, currentThreshold);

        // Build adjustment
        ThresholdAdjustment adjustment;
        adjustment.optimizationName = profile.optimizationName;
        adjustment.currentThreshold = currentThreshold;
        adjustment.recommendedThreshold = recommendedThreshold;
        adjustment.confidenceScore = computeConfidenceScore(profile);
        adjustment.rationale = generateRationale(profile,
                                                 profile.globalSuccessRate);
        adjustment.shouldApply = (adjustment.confidenceScore >= 60);

        adjustments_[profile.optimizationName] = adjustment;
    }
}

double Phase109ThresholdAdjuster::getRecommendedThreshold(
    const std::string& optimizationName) const {

    auto it = adjustments_.find(optimizationName);
    if (it != adjustments_.end()) {
        return it->second.recommendedThreshold;
    }

    // Default threshold if not found
    return 0.5;
}

std::vector<ThresholdAdjustment>
Phase109ThresholdAdjuster::getAllAdjustments() const {

    std::vector<ThresholdAdjustment> result;
    for (const auto& entry : adjustments_) {
        result.push_back(entry.second);
    }
    return result;
}

ThresholdAdjustment Phase109ThresholdAdjuster::getAdjustment(
    const std::string& optimizationName) const {

    auto it = adjustments_.find(optimizationName);
    if (it != adjustments_.end()) {
        return it->second;
    }

    // Return default adjustment if not found
    ThresholdAdjustment default_adj;
    default_adj.optimizationName = optimizationName;
    default_adj.currentThreshold = 0.5;
    default_adj.recommendedThreshold = 0.5;
    default_adj.confidenceScore = 0;
    default_adj.rationale = "No data available";
    default_adj.shouldApply = false;

    return default_adj;
}

bool Phase109ThresholdAdjuster::shouldApplyOptimization(
    const std::string& optimizationName,
    double effectivenessScore) const {

    double threshold = getRecommendedThreshold(optimizationName);
    return effectivenessScore >= threshold;
}

std::string Phase109ThresholdAdjuster::getAdjustmentRationale(
    const std::string& optimizationName) const {

    auto it = adjustments_.find(optimizationName);
    if (it != adjustments_.end()) {
        return it->second.rationale;
    }

    return "No adjustment data available";
}

std::string Phase109ThresholdAdjuster::generateReport() const {
    std::stringstream ss;
    ss << "=== Phase 109.3 Threshold Adjuster Report ===\n\n";

    ss << "Total Optimizations: " << adjustments_.size() << "\n";
    ss << "Applied Adjustments: " << getAppliedAdjustments() << "\n\n";

    ss << "Threshold Adjustments:\n";
    ss << "----------------------\n";

    for (const auto& entry : adjustments_) {
        const auto& adj = entry.second;

        ss << "\n" << adj.optimizationName << ":\n";
        ss << "  Current Threshold: " << std::fixed
           << std::setprecision(2) << adj.currentThreshold << "\n";
        ss << "  Recommended: " << std::fixed << std::setprecision(2)
           << adj.recommendedThreshold << "\n";
        ss << "  Change: ";

        double change = adj.recommendedThreshold - adj.currentThreshold;
        if (change > 0.01) {
            ss << "Increase by " << std::fixed << std::setprecision(1)
               << (change * 100.0) << "%\n";
        } else if (change < -0.01) {
            ss << "Decrease by " << std::fixed << std::setprecision(1)
               << (-change * 100.0) << "%\n";
        } else {
            ss << "No change\n";
        }

        ss << "  Confidence: " << std::fixed << std::setprecision(0)
           << adj.confidenceScore << "%\n";
        ss << "  Status: " << (adj.shouldApply ? "APPLY" : "HOLD") << "\n";
        ss << "  Rationale: " << adj.rationale << "\n";
    }

    return ss.str();
}

double Phase109ThresholdAdjuster::computeRecommendedThreshold(
    const OptimizationProfile& profile,
    double currentThreshold) const {

    double successRate = profile.globalSuccessRate;

    // Strategy based on success rate
    if (successRate > 0.85) {
        // Very successful: lower threshold to apply more
        // Reduce by 20%
        return currentThreshold * 0.80;
    } else if (successRate > 0.70) {
        // Successful: lower threshold slightly
        // Reduce by 10%
        return currentThreshold * 0.90;
    } else if (successRate > 0.50) {
        // Moderately successful: keep stable
        return currentThreshold;
    } else if (successRate > 0.30) {
        // Rarely successful: raise threshold (apply more cautiously)
        // Increase by 15%
        return currentThreshold * 1.15;
    } else {
        // Very rarely successful: raise threshold significantly
        // Increase by 30%
        return currentThreshold * 1.30;
    }
}

std::string Phase109ThresholdAdjuster::generateRationale(
    const OptimizationProfile& profile,
    double successRate) const {

    std::stringstream ss;

    if (profile.totalApplications < 5) {
        ss << "Insufficient data (" << profile.totalApplications
           << " applications) - use default threshold";
        return ss.str();
    }

    if (successRate > 0.85) {
        ss << "High success rate (" << std::fixed << std::setprecision(1)
           << (successRate * 100.0) << "%) - apply more aggressively";
        return ss.str();
    }

    if (successRate > 0.70) {
        ss << "Good success rate (" << std::fixed << std::setprecision(1)
           << (successRate * 100.0) << "%) - apply by default";
        return ss.str();
    }

    if (successRate > 0.50) {
        ss << "Moderate success rate (" << std::fixed
           << std::setprecision(1) << (successRate * 100.0)
           << "%) - apply selectively";
        return ss.str();
    }

    if (successRate > 0.30) {
        ss << "Low success rate (" << std::fixed << std::setprecision(1)
           << (successRate * 100.0)
           << "%) - apply only if budget permits";
        return ss.str();
    }

    ss << "Very low success rate (" << std::fixed << std::setprecision(1)
       << (successRate * 100.0) << "%) - skip this optimization";
    return ss.str();
}

int Phase109ThresholdAdjuster::computeConfidenceScore(
    const OptimizationProfile& profile) const {

    int dataPoints = profile.totalApplications;

    if (dataPoints < 5) return 20;
    if (dataPoints < 10) return 40;
    if (dataPoints < 20) return 60;
    if (dataPoints < 50) return 80;
    return 100;
}
