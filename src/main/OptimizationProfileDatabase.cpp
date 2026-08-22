#include "OptimizationProfileDatabase.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <cmath>

OptimizationProfileDatabase::OptimizationProfileDatabase()
    : totalCompilations_(0) {
}

OptimizationProfileDatabase::~OptimizationProfileDatabase() = default;

void OptimizationProfileDatabase::recordCompilation(
    const CompilationRecord& record,
    const std::vector<OptimizationApplication>& applications) {

    totalCompilations_++;
    compilationRecords_.push_back(record);

    // Store applications by optimization name
    for (const auto& app : applications) {
        optimizationApplications_[app.optimizationName].push_back(app);
    }

    // Update profiles
    for (const auto& entry : optimizationApplications_) {
        optimizationProfiles_[entry.first] =
            computeGlobalEffectiveness(entry.first);
    }
}

OptimizationProfile OptimizationProfileDatabase::getProfile(
    const std::string& optimizationName) const {

    auto it = optimizationProfiles_.find(optimizationName);
    if (it != optimizationProfiles_.end()) {
        return it->second;
    }

    // Return empty profile if not found
    OptimizationProfile empty;
    empty.optimizationName = optimizationName;
    empty.totalApplications = 0;
    empty.globalSuccessRate = 0.0;
    return empty;
}

std::vector<OptimizationProfile> OptimizationProfileDatabase::getAllProfiles()
    const {

    std::vector<OptimizationProfile> profiles;
    for (const auto& entry : optimizationProfiles_) {
        profiles.push_back(entry.second);
    }
    return profiles;
}

ContextualEffectiveness OptimizationProfileDatabase::getContextualEffectiveness(
    const std::string& optimizationName,
    const OptimizationContext& context) const {

    return computeContextualEffectiveness(optimizationName, context);
}

void OptimizationProfileDatabase::analyzeTrends() {
    for (auto& entry : optimizationProfiles_) {
        entry.second.trend = computeTrend(entry.first);
        entry.second.recommendation =
            generateRecommendation(entry.first, entry.second);
    }
}

std::string OptimizationProfileDatabase::generateRecommendation(
    const std::string& optimizationName,
    const OptimizationProfile& profile) const {

    if (profile.totalApplications < 5) {
        return "Insufficient data - use default strategy";
    }

    if (profile.globalSuccessRate > 0.85) {
        return "Always apply - consistently beneficial";
    }

    if (profile.globalSuccessRate > 0.70) {
        return "Apply by default - usually beneficial";
    }

    if (profile.globalSuccessRate > 0.50) {
        return "Apply selectively - context-dependent";
    }

    if (profile.globalSuccessRate > 0.30) {
        return "Apply only if budget permits - often harmful";
    }

    return "Skip - rarely beneficial";
}

int OptimizationProfileDatabase::getConfidenceScore(
    const std::string& optimizationName) const {

    auto it = optimizationProfiles_.find(optimizationName);
    if (it == optimizationProfiles_.end()) {
        return 0;  // No data
    }

    // Confidence based on data points
    int dataPoints = it->second.totalApplications;
    if (dataPoints < 5) return 20;      // Very low confidence
    if (dataPoints < 10) return 40;     // Low confidence
    if (dataPoints < 20) return 60;     // Medium confidence
    if (dataPoints < 50) return 80;     // High confidence
    return 100;                          // Very high confidence
}

void OptimizationProfileDatabase::saveToFile(const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open profile database file: " << filepath
                  << std::endl;
        return;
    }

    file << "{\n";
    file << "  \"compilations\": " << totalCompilations_ << ",\n";
    file << "  \"optimizations\": " << optimizationProfiles_.size() << ",\n";
    file << "  \"profiles\": [\n";

    bool first = true;
    for (const auto& entry : optimizationProfiles_) {
        if (!first) file << ",\n";
        file << profileToJson(entry.second);
        first = false;
    }

    file << "  ]\n";
    file << "}\n";
    file.close();
}

void OptimizationProfileDatabase::loadFromFile(const std::string& filepath) {
    // Placeholder: would implement JSON parsing
    std::cout << "[Phase109] Loading database from: " << filepath
              << std::endl;
}

std::string OptimizationProfileDatabase::generateReport() const {
    std::string report = "=== Phase 109 Profile Database Report ===\n\n";

    report += "Total Compilations: " + std::to_string(totalCompilations_)
             + "\n";
    report += "Optimizations Tracked: "
             + std::to_string(optimizationProfiles_.size()) + "\n\n";

    report += "Optimization Profiles:\n";
    report += "----------------------\n";

    for (const auto& entry : optimizationProfiles_) {
        const auto& profile = entry.second;
        report += "\n" + profile.optimizationName + ":\n";
        report += "  Applications: " + std::to_string(profile.totalApplications)
                 + "\n";
        report += "  Success Rate: "
                 + std::to_string(profile.globalSuccessRate * 100.0)
                 + "%\n";
        report += "  Avg Speedup: "
                 + std::to_string(profile.globalAvgSpeedup) + "%\n";
        report += "  Avg Size Reduction: "
                 + std::to_string(profile.globalAvgSizeReduction) + "%\n";
        report += "  Recommendation: " + profile.recommendation + "\n";
        report += "  Confidence: " + std::to_string(getConfidenceScore(
                                        profile.optimizationName))
                 + "%\n";
    }

    return report;
}

OptimizationContext OptimizationProfileDatabase::classifyContext(
    const CompilationRecord& record) const {

    OptimizationContext context;

    // Classify by file size
    if (record.fileSize < 1000) {
        context.fileSize = OptimizationContext::TINY;
    } else if (record.fileSize < 5000) {
        context.fileSize = OptimizationContext::SMALL;
    } else if (record.fileSize < 20000) {
        context.fileSize = OptimizationContext::MEDIUM;
    } else if (record.fileSize < 100000) {
        context.fileSize = OptimizationContext::LARGE;
    } else {
        context.fileSize = OptimizationContext::HUGE;
    }

    // Classify by complexity
    int complexity = record.functionCount + record.loopCount / 2
                   + record.branchDensity / 10;
    if (complexity < 10) {
        context.complexity = OptimizationContext::LOW;
    } else if (complexity < 25) {
        context.complexity = OptimizationContext::MODERATE;
    } else if (complexity < 50) {
        context.complexity = OptimizationContext::HIGH;
    } else {
        context.complexity = OptimizationContext::VERY_HIGH;
    }

    context.loopCount = record.loopCount;
    context.branchDensity = record.branchDensity;

    return context;
}

OptimizationProfile OptimizationProfileDatabase::computeGlobalEffectiveness(
    const std::string& optimizationName) const {

    OptimizationProfile profile;
    profile.optimizationName = optimizationName;

    auto it = optimizationApplications_.find(optimizationName);
    if (it == optimizationApplications_.end()
        || it->second.empty()) {

        profile.totalApplications = 0;
        profile.globalSuccessRate = 0.0;
        return profile;
    }

    const auto& applications = it->second;
    profile.totalApplications = applications.size();

    double totalSpeedup = 0.0;
    double totalReduction = 0.0;
    double totalCost = 0.0;
    int successCount = 0;

    for (const auto& app : applications) {
        if (app.successful) {
            successCount++;
        }
        totalSpeedup += app.actualSpeedup;
        totalReduction += app.actualSizeReduction;
        totalCost += app.compilationCost;
    }

    profile.totalSuccesses = successCount;
    profile.globalSuccessRate = static_cast<double>(successCount)
                               / applications.size();
    profile.globalAvgSpeedup = totalSpeedup / applications.size();
    profile.globalAvgSizeReduction = totalReduction / applications.size();
    profile.globalAvgCost = totalCost / applications.size();

    if (profile.globalAvgCost > 0.0) {
        profile.globalBenefitScore = (profile.globalAvgSpeedup
                                      + profile.globalAvgSizeReduction)
                                    / profile.globalAvgCost;
    } else {
        profile.globalBenefitScore = profile.globalAvgSpeedup
                                    + profile.globalAvgSizeReduction;
    }

    return profile;
}

ContextualEffectiveness OptimizationProfileDatabase::computeContextualEffectiveness(
    const std::string& optimizationName,
    const OptimizationContext& context) const {

    ContextualEffectiveness result;
    result.context = context;
    result.applicationCount = 0;
    result.successCount = 0;

    auto it = optimizationApplications_.find(optimizationName);
    if (it == optimizationApplications_.end()) {
        result.successRate = 0.0;
        return result;
    }

    double totalSpeedup = 0.0;
    double totalReduction = 0.0;
    double totalCost = 0.0;

    // Find compilations matching context
    for (size_t i = 0; i < compilationRecords_.size(); i++) {
        auto ctx = classifyContext(compilationRecords_[i]);

        if (ctx.fileSize == context.fileSize
            && ctx.complexity == context.complexity) {

            result.applicationCount++;

            // Find corresponding application
            for (const auto& app : it->second) {
                if (app.successful) {
                    result.successCount++;
                }
                totalSpeedup += app.actualSpeedup;
                totalReduction += app.actualSizeReduction;
                totalCost += app.compilationCost;
            }
        }
    }

    if (result.applicationCount > 0) {
        result.successRate = static_cast<double>(result.successCount)
                           / result.applicationCount;
        result.avgSpeedup = totalSpeedup / result.applicationCount;
        result.avgSizeReduction = totalReduction / result.applicationCount;
        result.avgCost = totalCost / result.applicationCount;

        if (result.avgCost > 0.0) {
            result.benefitScore = (result.avgSpeedup
                                  + result.avgSizeReduction)
                                / result.avgCost;
        } else {
            result.benefitScore = result.avgSpeedup
                                + result.avgSizeReduction;
        }
    }

    return result;
}

OptimizationProfile::Trend OptimizationProfileDatabase::computeTrend(
    const std::string& optimizationName) const {

    OptimizationProfile::Trend trend;
    trend.dataPointCount = 0;
    trend.successRateTrend = 0.0;
    trend.benefitTrend = 0.0;
    trend.confidenceScore = 0.0;

    auto it = optimizationApplications_.find(optimizationName);
    if (it == optimizationApplications_.end()) {
        return trend;
    }

    trend.dataPointCount = it->second.size();

    // Confidence increases with data points
    if (trend.dataPointCount < 5) {
        trend.confidenceScore = 20.0;
    } else if (trend.dataPointCount < 10) {
        trend.confidenceScore = 40.0;
    } else if (trend.dataPointCount < 20) {
        trend.confidenceScore = 60.0;
    } else if (trend.dataPointCount < 50) {
        trend.confidenceScore = 80.0;
    } else {
        trend.confidenceScore = 100.0;
    }

    // Placeholder: would analyze chronological trend
    return trend;
}

std::string OptimizationProfileDatabase::profileToJson(
    const OptimizationProfile& profile) const {

    std::string json = "    {\n";
    json += "      \"name\": \"" + profile.optimizationName + "\",\n";
    json += "      \"applications\": " + std::to_string(
                                            profile.totalApplications)
            + ",\n";
    json += "      \"successRate\": "
            + std::to_string(profile.globalSuccessRate) + ",\n";
    json += "      \"speedup\": " + std::to_string(
                                      profile.globalAvgSpeedup)
            + ",\n";
    json += "      \"sizeReduction\": "
            + std::to_string(profile.globalAvgSizeReduction) + ",\n";
    json += "      \"recommendation\": \"" + profile.recommendation
            + "\"\n";
    json += "    }";

    return json;
}

OptimizationProfile OptimizationProfileDatabase::profileFromJson(
    const std::string& json) const {

    // Placeholder: would implement JSON parsing
    return OptimizationProfile{};
}
