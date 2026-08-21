// Phase 101.3: Comparative Analysis Implementation
#include "Phase101ComparativeAnalyzer.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <numeric>

namespace phase101 {

std::vector<ComparisonMetrics> Phase101ComparativeAnalyzer::compareOptimizations(
    const std::vector<PerformanceCounter>& counters) {
    allComparisons_.clear();

    for (const auto& counter : counters) {
        ComparisonMetrics metric;
        metric.optimizationName = counter.hintType;
        metric.beforeSize = counter.originalInstructions * 3;  // bytes
        metric.afterSize = counter.optimizedInstructions * 3;
        metric.sizeSavings = metric.beforeSize - metric.afterSize;

        if (metric.beforeSize > 0) {
            metric.sizeReductionPercent = 100.0 * metric.sizeSavings / metric.beforeSize;
        }

        // Performance metrics
        metric.beforeCycles = counter.cyclesPerCallOriginal;
        metric.afterCycles = counter.cyclesPerCallOptimized;
        metric.cyclesSaved = metric.beforeCycles - metric.afterCycles;
        metric.speedupPercent = calculateSpeedup(metric.beforeCycles, metric.afterCycles);

        // Quality metrics
        metric.estimationAccuracy = counter.estimationAccuracy;
        metric.implementationCost = counter.appliedTime.count() / 1000;  // ms
        metric.roi = calculateEffectivenessScore(metric);
        metric.isBeneficial = metric.sizeSavings > 0 || metric.cyclesSaved > 0;

        allComparisons_.push_back(metric);
        benefitsByType_[counter.hintType].push_back(metric.sizeReductionPercent);
    }

    return allComparisons_;
}

std::vector<OptimizationTrend> Phase101ComparativeAnalyzer::analyzeTrends(
    const std::vector<PerformanceCounter>& counters) {
    std::vector<OptimizationTrend> trends;

    // Group counters by hint type
    std::map<std::string, std::vector<const PerformanceCounter*>> byType;
    for (const auto& counter : counters) {
        byType[counter.hintType].push_back(&counter);
    }

    // Analyze each type
    for (const auto& [hintType, countersOfType] : byType) {
        OptimizationTrend trend;
        trend.optimizationType = hintType;
        trend.totalApplications = countersOfType.size();

        std::vector<double> benefits;
        for (const auto* counter : countersOfType) {
            int benefit = counter->savedBytes;
            benefits.push_back(benefit);

            if (benefit > 0) {
                trend.successCount++;
            }
        }

        // Calculate statistics
        if (!benefits.empty()) {
            trend.averageBenefit = std::accumulate(benefits.begin(), benefits.end(), 0.0) /
                                  benefits.size();
            trend.minBenefit = *std::min_element(benefits.begin(), benefits.end());
            trend.maxBenefit = *std::max_element(benefits.begin(), benefits.end());

            // Standard deviation
            double variance = 0.0;
            for (double b : benefits) {
                variance += (b - trend.averageBenefit) * (b - trend.averageBenefit);
            }
            variance /= benefits.size();
            trend.benefitStdDev = std::sqrt(variance);

            trend.successRate = 100.0 * trend.successCount / trend.totalApplications;
        }

        trends.push_back(trend);
    }

    return trends;
}

std::vector<std::pair<std::string, double>>
Phase101ComparativeAnalyzer::rankByEffectiveness(
    const std::vector<ComparisonMetrics>& metrics) {
    std::map<std::string, double> effectivenessMap;

    for (const auto& metric : metrics) {
        double score = calculateEffectivenessScore(metric);
        effectivenessMap[metric.optimizationName] += score;
    }

    // Sort by effectiveness
    std::vector<std::pair<std::string, double>> ranked(
        effectivenessMap.begin(), effectivenessMap.end());

    std::sort(ranked.begin(), ranked.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });

    return ranked;
}

std::vector<std::pair<std::string, std::string>>
Phase101ComparativeAnalyzer::findSynergies(
    const std::vector<ComparisonMetrics>& metrics) {
    std::vector<std::pair<std::string, std::string>> synergies;

    // Find pairs of optimizations that appear together with high combined benefit
    std::map<std::pair<std::string, std::string>, double> pairBenefits;

    for (size_t i = 0; i < metrics.size(); ++i) {
        for (size_t j = i + 1; j < metrics.size(); ++j) {
            const auto& m1 = metrics[i];
            const auto& m2 = metrics[j];

            // Check if targeting same function/loop
            if (m1.targetFunction == m2.targetFunction &&
                !m1.targetFunction.empty()) {
                // Check if combined benefit exceeds sum
                double expected = m1.roi + m2.roi;
                double actual = (m1.sizeSavings + m2.sizeSavings) /
                               (double)std::max(1, m1.beforeSize + m2.beforeSize);

                if (actual > expected * 1.15) {  // 15% synergy bonus
                    pairBenefits[{m1.optimizationName, m2.optimizationName}] = actual;
                }
            }
        }
    }

    // Extract high-synergy pairs
    for (const auto& [pair, benefit] : pairBenefits) {
        if (benefit > 0.05) {  // At least 5% combined benefit
            synergies.push_back(pair);
        }
    }

    return synergies;
}

std::string Phase101ComparativeAnalyzer::generateComparativeReport(
    const std::vector<ComparisonMetrics>& metrics) {
    std::stringstream report;

    report << "╔════════════════════════════════════════════════════════════════╗\n";
    report << "║     Phase 101.3: Comparative Performance Analysis Report        ║\n";
    report << "║     Optimization Effectiveness Comparison                      ║\n";
    report << "╚════════════════════════════════════════════════════════════════╝\n\n";

    // Summary
    report << "COMPARISON SUMMARY\n";
    report << "──────────────────────────────────────────────────────────────────\n";
    report << "Total Optimizations Analyzed: " << metrics.size() << "\n";

    int beneficialCount = 0;
    int totalSaved = 0;
    for (const auto& m : metrics) {
        if (m.isBeneficial) beneficialCount++;
        totalSaved += m.sizeSavings;
    }

    report << "Beneficial Optimizations:     " << beneficialCount << " / " << metrics.size() << "\n";
    report << "Total Size Saved:             " << totalSaved << " bytes\n";
    report << "Success Rate:                 " << std::fixed << std::setprecision(1)
           << (100.0 * beneficialCount / std::max(1, (int)metrics.size())) << "%\n\n";

    // Detailed comparison table
    report << "DETAILED COMPARISON\n";
    report << "──────────────────────────────────────────────────────────────────\n";
    report << "Optimization    Before  After   Saved  %Red   Speed   ROI   Acc\n";
    report << "──────────────────────────────────────────────────────────────────\n";

    for (const auto& metric : metrics) {
        report << formatMetricRow(metric);
    }

    report << "\n";

    // Ranking by effectiveness
    auto ranked = rankByEffectiveness(metrics);
    report << "EFFECTIVENESS RANKING\n";
    report << "──────────────────────────────────────────────────────────────────\n";
    for (size_t i = 0; i < ranked.size() && i < 5; ++i) {
        report << (i + 1) << ". " << std::left << std::setw(30) << ranked[i].first
               << " Score: " << std::fixed << std::setprecision(2) << ranked[i].second << "\n";
    }

    report << "\n";
    return report.str();
}

std::string Phase101ComparativeAnalyzer::generateTrendReport(
    const std::vector<OptimizationTrend>& trends) {
    std::stringstream report;

    report << "╔════════════════════════════════════════════════════════════════╗\n";
    report << "║        Phase 101.3: Optimization Effectiveness Trends          ║\n";
    report << "║        Pattern Analysis Across Optimization Types              ║\n";
    report << "╚════════════════════════════════════════════════════════════════╝\n\n";

    report << "TREND ANALYSIS\n";
    report << "──────────────────────────────────────────────────────────────────\n";
    report << "Type              Count  Avg     Min    Max    Success ROI\n";
    report << "──────────────────────────────────────────────────────────────────\n";

    for (const auto& trend : trends) {
        report << formatTrendRow(trend);
    }

    report << "\n";

    // Identify high-performing optimization types
    report << "HIGH-PERFORMING OPTIMIZATION TYPES\n";
    report << "──────────────────────────────────────────────────────────────────\n";

    std::vector<const OptimizationTrend*> ranked;
    for (const auto& trend : trends) {
        ranked.push_back(&trend);
    }

    std::sort(ranked.begin(), ranked.end(),
        [](const auto* a, const auto* b) {
            return a->averageBenefit > b->averageBenefit;
        });

    for (size_t i = 0; i < ranked.size() && i < 3; ++i) {
        const auto* trend = ranked[i];
        report << (i + 1) << ". " << trend->optimizationType << "\n";
        report << "   Average Benefit: " << std::fixed << std::setprecision(1)
               << trend->averageBenefit << " bytes\n";
        report << "   Success Rate:    " << std::setprecision(1)
               << trend->successRate << "%\n";
    }

    report << "\n";
    return report.str();
}

double Phase101ComparativeAnalyzer::calculateEffectivenessScore(
    const ComparisonMetrics& metric) {
    if (metric.beforeSize == 0) return 0.0;

    // Score = (size reduction % + speedup % + accuracy%) / 3
    double score = metric.sizeReductionPercent * 0.5 +
                   metric.speedupPercent * 0.3 +
                   metric.estimationAccuracy * 100.0 * 0.2;

    return score / 100.0;
}

double Phase101ComparativeAnalyzer::predictBenefit(
    const std::string& optimizationType,
    int targetSize) {
    // Use historical data to predict benefit
    auto it = benefitsByType_.find(optimizationType);
    if (it == benefitsByType_.end() || it->second.empty()) {
        return 0.0;
    }

    double avgBenefit = std::accumulate(it->second.begin(), it->second.end(), 0.0) /
                       it->second.size();

    // Scale benefit by target size
    return (avgBenefit / 100.0) * targetSize;
}

double Phase101ComparativeAnalyzer::calculateSpeedup(
    int beforeCycles, int afterCycles) {
    if (beforeCycles == 0) return 0.0;
    return 100.0 * (beforeCycles - afterCycles) / beforeCycles;
}

double Phase101ComparativeAnalyzer::calculateAccuracy(
    int estimated, int actual) {
    if (estimated == 0) return 0.0;
    return std::min(1.0, (double)actual / estimated);
}

std::string Phase101ComparativeAnalyzer::formatMetricRow(
    const ComparisonMetrics& metric) {
    std::stringstream row;

    row << std::left << std::setw(15) << metric.optimizationName
        << "  " << std::right << std::setw(6) << metric.beforeSize
        << "  " << std::setw(5) << metric.afterSize
        << "  " << std::setw(5) << metric.sizeSavings
        << "  " << std::setw(4) << std::fixed << std::setprecision(1)
        << metric.sizeReductionPercent
        << "%  " << std::setw(6) << std::setprecision(1) << metric.speedupPercent
        << "%  " << std::setw(5) << std::setprecision(2) << metric.roi
        << "  " << std::setprecision(0) << metric.estimationAccuracy << "%\n";

    return row.str();
}

std::string Phase101ComparativeAnalyzer::formatTrendRow(
    const OptimizationTrend& trend) {
    std::stringstream row;

    row << std::left << std::setw(17) << trend.optimizationType
        << "  " << std::right << std::setw(5) << trend.totalApplications
        << "  " << std::setw(7) << std::fixed << std::setprecision(1) << trend.averageBenefit
        << "  " << std::setw(6) << trend.minBenefit
        << "  " << std::setw(6) << trend.maxBenefit
        << "  " << std::setw(6) << std::setprecision(1) << trend.successRate
        << "%  " << std::setprecision(2) << trend.benefitStdDev << "\n";

    return row.str();
}

} // namespace phase101
