// Phase 107.1: Online Learning Engine Implementation
#include "OnlineLearner.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <numeric>
#include <cmath>

namespace phase107 {

void OnlineLearner::initializeSession(const std::string& sessionId) {
    currentSession_ = OnlineLearningState();
    currentSession_.compilationSession = sessionId;
    currentSession_.collectedSignals.clear();
    currentSession_.costBenefitAnalyses.clear();
}

void OnlineLearner::collectSignal(const CompilationSignal& signal) {
    currentSession_.collectedSignals.push_back(signal);
    totalSignalsCollected_++;
}

OptimizationCostBenefit OnlineLearner::analyzeCostBenefit(
    const std::string& optimizationName,
    double expectedSpeedup,
    double expectedSizeReduction,
    double compilationCostMs) {

    OptimizationCostBenefit analysis;
    analysis.optimizationName = optimizationName;
    analysis.expectedSpeedup = expectedSpeedup;
    analysis.expectedSizeReduction = expectedSizeReduction;
    analysis.compilationCostMs = compilationCostMs;

    // Compute benefit score
    analysis.benefitScore = computeBenefitScore(expectedSpeedup,
                                               expectedSizeReduction,
                                               compilationCostMs);

    // Decide if worth applying
    analysis.shouldApply = analysis.benefitScore >= analysis.worthwhileThreshold;

    if (analysis.shouldApply) {
        analysis.justification = optimizationName + ": benefit score " +
                                std::to_string((int)analysis.benefitScore) +
                                " exceeds threshold " +
                                std::to_string((int)analysis.worthwhileThreshold);
    } else {
        analysis.justification = optimizationName + ": cost overhead (" +
                                std::to_string((int)compilationCostMs) +
                                " ms) not justified by benefit";
    }

    currentSession_.costBenefitAnalyses.push_back(analysis);
    return analysis;
}

bool OnlineLearner::shouldApplyOptimizationNow(
    const OptimizationCostBenefit& analysis,
    double currentCompileTimeMs) {

    // Don't apply if benefit score is too low
    if (analysis.benefitScore < analysis.worthwhileThreshold) {
        return false;
    }

    // Don't apply if we're already taking too long compiling
    if (currentCompileTimeMs > 500.0) {  // 500ms threshold
        return false;  // Skip expensive optimization if compile time is high
    }

    // Apply if benefit exceeds cost
    return analysis.shouldApply;
}

void OnlineLearner::updateOptimizationEffectiveness(
    const std::string& optimizationName,
    double observedSpeedup) {

    // Update running effectiveness score for this optimization
    auto it = currentSession_.optimizationEffectiveness.find(optimizationName);
    if (it != currentSession_.optimizationEffectiveness.end()) {
        // Running average
        double oldScore = it->second;
        int count = currentSession_.optimizationApplicationCount[optimizationName];
        double newScore = (oldScore * count + observedSpeedup) / (count + 1);
        it->second = newScore;
    } else {
        currentSession_.optimizationEffectiveness[optimizationName] = observedSpeedup;
    }

    currentSession_.optimizationApplicationCount[optimizationName]++;
}

std::string OnlineLearner::generateOnlineLearningReport() {

    std::stringstream report;

    report << "╔════════════════════════════════════════════════════════════════╗\n";
    report << "║     Phase 107.1: Online Learning Engine Report                 ║\n";
    report << "║     Real-Time Adaptation During Compilation                    ║\n";
    report << "╚════════════════════════════════════════════════════════════════╝\n\n";

    report << "SESSION SUMMARY\n";
    report << "──────────────────────────────────────────────────────────────────\n";
    report << "Session ID:                 " << currentSession_.compilationSession << "\n";
    report << "Signals Collected:          " << currentSession_.collectedSignals.size() << "\n";
    report << "Cost-Benefit Analyses:      " << currentSession_.costBenefitAnalyses.size() << "\n";
    report << "Decisions Adapted:          " << currentSession_.decisionsAdapted << "\n";
    report << "Overall Progress Score:     " << std::fixed << std::setprecision(1)
           << currentSession_.overallProgressScore << "\n\n";

    report << "COLLECTED SIGNALS\n";
    report << "──────────────────────────────────────────────────────────────────\n";
    report << "Type              Phase      Value     Priority  Timestamp\n";
    report << "──────────────────────────────────────────────────────────────────\n";

    for (const auto& signal : currentSession_.collectedSignals) {
        report << formatSignalRow(signal);
    }

    report << "\nCOST-BENEFIT ANALYSES\n";
    report << "──────────────────────────────────────────────────────────────────\n";
    report << "Optimization         Speedup  Size Red  Compile Cost  Score  Apply\n";
    report << "──────────────────────────────────────────────────────────────────\n";

    for (const auto& analysis : currentSession_.costBenefitAnalyses) {
        report << formatCostBenefitRow(analysis);
    }

    report << "\nOPTIMIZATION EFFECTIVENESS\n";
    report << "──────────────────────────────────────────────────────────────────\n";
    report << "Optimization         Avg Effectiveness  Applied Count\n";
    report << "──────────────────────────────────────────────────────────────────\n";

    for (const auto& [optName, effectiveness] : currentSession_.optimizationEffectiveness) {
        int count = currentSession_.optimizationApplicationCount[optName];
        report << std::left << std::setw(20) << optName
               << "  " << std::right << std::setw(17) << std::fixed << std::setprecision(1)
               << effectiveness << "%  "
               << std::setw(14) << count << "\n";
    }

    report << "\n";
    return report.str();
}

double OnlineLearner::computeBenefitScore(double speedup,
                                                 double sizeReduction,
                                                 double compilationCost) {
    // Score = (speedup + sizeReduction) / compilationCost
    // If cost is 0, return infinite score (free optimization)
    if (compilationCost <= 0.1) {
        return 100.0;  // Cap at 100 for free optimizations
    }

    double totalBenefit = speedup + sizeReduction;
    return totalBenefit / compilationCost;
}

std::string OnlineLearner::formatSignalRow(const CompilationSignal& signal) {
    std::stringstream row;

    row << std::left << std::setw(17) << signal.signalType
        << "  " << std::setw(10) << signal.phase
        << "  " << std::right << std::setw(8) << std::fixed << std::setprecision(1)
        << signal.value
        << "  " << std::setw(9) << signal.priority
        << "  " << signal.timestamp.count() << " ms\n";

    return row.str();
}

std::string OnlineLearner::formatCostBenefitRow(
    const OptimizationCostBenefit& analysis) {

    std::stringstream row;

    std::string optShort = analysis.optimizationName;
    if (optShort.length() > 20) {
        optShort = optShort.substr(0, 17) + "...";
    }

    row << std::left << std::setw(20) << optShort
        << "  " << std::right << std::setw(7) << std::fixed << std::setprecision(1)
        << analysis.expectedSpeedup << "%"
        << "  " << std::setw(8) << analysis.expectedSizeReduction << "%"
        << "  " << std::setw(12) << analysis.compilationCostMs << " ms"
        << "  " << std::setw(5) << analysis.benefitScore
        << "  " << (analysis.shouldApply ? "YES" : "NO") << "\n";

    return row.str();
}

} // namespace phase107
