// Phase 106.3: Feedback Coordinator Implementation
#include "Phase106FeedbackCoordinator.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <numeric>
#include <cmath>

using namespace phase105;

namespace phase106 {

void Phase106FeedbackCoordinator::collectFeedback(
    const IterationResult& iterationResult) {

    // Signal from metrics
    signalFromMetrics(iterationResult.metrics);

    // Signal from strategy comparisons
    signalFromComparisons(iterationResult.comparisons);

    // Signal from learned patterns
    for (const auto& pattern : learner_.getLearnedPatterns()) {
        signalFromLearning(pattern);
    }

    // Update statistics
    feedbackStats_.totalSignalsCollected = collectedSignals_.size();
    feedbackStats_.positiveSignals = 0;
    feedbackStats_.negativeSignals = 0;
    feedbackStats_.avgReinforcement = 0.0;
    feedbackStats_.signalsBySource.clear();

    double sumReinforcement = 0.0;
    for (const auto& signal : collectedSignals_) {
        if (signal.reinforcementValue > 0) {
            feedbackStats_.positiveSignals++;
        } else if (signal.reinforcementValue < 0) {
            feedbackStats_.negativeSignals++;
        }
        sumReinforcement += signal.reinforcementValue;
        feedbackStats_.signalsBySource[signal.source]++;
    }

    if (!collectedSignals_.empty()) {
        feedbackStats_.avgReinforcement = sumReinforcement / collectedSignals_.size();
    }
}

std::vector<FeedbackSignal> Phase106FeedbackCoordinator::analyzeFeedback() {
    // Sort by priority
    std::sort(collectedSignals_.begin(), collectedSignals_.end(),
        [](const FeedbackSignal& a, const FeedbackSignal& b) {
            return a.priority > b.priority;
        });

    return collectedSignals_;
}

StrategyAdjustment Phase106FeedbackCoordinator::generateStrategyAdjustment(
    const std::vector<FeedbackSignal>& feedback,
    const std::string& currentStrategy,
    const phase105::AggregateMetrics& metrics) {
    (void)metrics;  // Suppress unused warning

    StrategyAdjustment adjustment;
    adjustment.currentStrategy = currentStrategy;

    // Accumulate promotion and suppression signals
    std::map<std::string, double> reinforcementByOpt;
    double netReinforcement = 0.0;

    for (const auto& signal : feedback) {
        reinforcementByOpt[signal.targetOptimization] += signal.reinforcementValue;
        netReinforcement += signal.reinforcementValue;
    }

    // Generate recommendations
    for (const auto& [opt, reinforcement] : reinforcementByOpt) {
        if (reinforcement > 0.3) {
            adjustment.optimizationsToPromote.push_back(opt);
        } else if (reinforcement < -0.3) {
            adjustment.optimizationsToSuppress.push_back(opt);
        }
    }

    // Determine recommended change
    if (netReinforcement > 2.0) {
        adjustment.recommendedChange = "intensify_current_strategy";
        adjustment.expectedImprovementPercent = 10.0;
        adjustment.justification = "Strong positive signals across multiple sources";
    } else if (netReinforcement < -1.0) {
        adjustment.recommendedChange = "switch_to_alternative_strategy";
        adjustment.expectedImprovementPercent = 15.0;
        adjustment.justification = "Consistent negative signals detected";
    } else {
        adjustment.recommendedChange = "fine_tune_current_strategy";
        adjustment.expectedImprovementPercent = 5.0;
        adjustment.justification = "Mixed signals suggest targeted adjustments";
    }

    adjustmentsProposed_++;
    return adjustment;
}

void Phase106FeedbackCoordinator::applyFeedback(
    const std::vector<FeedbackSignal>& feedback) {

    for (const auto& signal : feedback) {
        // Feedback is marked for application in strategy selection
        // The actual strategy updates happen in Phase106IterationManager
    }
}

std::string Phase106FeedbackCoordinator::generateFeedbackReport() {

    std::stringstream report;

    report << "╔════════════════════════════════════════════════════════════════╗\n";
    report << "║     Phase 106.3: Feedback Coordination Report                  ║\n";
    report << "║     Strategy Adjustment & Optimization Signal Analysis        ║\n";
    report << "╚════════════════════════════════════════════════════════════════╝\n\n";

    report << "FEEDBACK SUMMARY\n";
    report << "──────────────────────────────────────────────────────────────────\n";
    report << "Total Signals:              " << feedbackStats_.totalSignalsCollected << "\n";
    report << "Positive Signals:           " << feedbackStats_.positiveSignals << "\n";
    report << "Negative Signals:           " << feedbackStats_.negativeSignals << "\n";
    report << "Net Reinforcement:          " << std::fixed << std::setprecision(2)
           << feedbackStats_.avgReinforcement << "\n\n";

    report << "SIGNALS BY SOURCE\n";
    report << "──────────────────────────────────────────────────────────────────\n";

    for (const auto& [source, count] : feedbackStats_.signalsBySource) {
        report << std::left << std::setw(20) << source
               << "  " << std::right << std::setw(5) << count << " signals\n";
    }

    report << "\nFEEDBACK SIGNALS ANALYSIS\n";
    report << "──────────────────────────────────────────────────────────────────\n";
    report << "Optimization          Source    Reinforcement  Priority  Reason\n";
    report << "──────────────────────────────────────────────────────────────────\n";

    for (const auto& signal : collectedSignals_) {
        report << formatFeedbackRow(signal);
    }

    report << "\nADJUSTMENTS PROPOSED\n";
    report << "──────────────────────────────────────────────────────────────────\n";
    report << "Total Adjustments:          " << getTotalAdjustmentsProposed() << "\n";

    report << "\n";
    return report.str();
}

void Phase106FeedbackCoordinator::signalFromMetrics(
    const phase105::AggregateMetrics& metrics) {

    // Strong positive signal if average speedup is good
    if (metrics.avgSpeedup > 20.0) {
        FeedbackSignal signal;
        signal.source = "metrics";
        signal.targetOptimization = "current_best";
        signal.reinforcementValue = 0.8;
        signal.reason = "High average speedup (" + std::to_string((int)metrics.avgSpeedup) + "%)";
        signal.priority = 8;
        collectedSignals_.push_back(signal);
    }

    // Negative signal if volatility is high
    double volatility = metrics.maxSpeedup - metrics.minSpeedup;
    if (volatility > 20.0) {
        FeedbackSignal signal;
        signal.source = "metrics";
        signal.targetOptimization = "high_variance_opt";
        signal.reinforcementValue = -0.5;
        signal.reason = "High volatility indicates inconsistent performance";
        signal.priority = 6;
        collectedSignals_.push_back(signal);
    }
}

void Phase106FeedbackCoordinator::signalFromComparisons(
    const std::vector<phase105::StrategyComparison>& comparisons) {

    for (const auto& comp : comparisons) {
        if (comp.speedupDifference > 10.0) {
            FeedbackSignal signal;
            signal.source = "comparison";
            signal.targetOptimization = comp.optimizedStrategy;
            signal.reinforcementValue = std::min(1.0, comp.speedupDifference / 20.0);
            signal.reason = comp.optimizedStrategy + " shows " +
                           std::to_string((int)comp.speedupDifference) + "% better speedup";
            signal.priority = 7;
            collectedSignals_.push_back(signal);
        }
    }
}

void Phase106FeedbackCoordinator::signalFromLearning(
    const OptimizationPattern& pattern) {

    if (pattern.confidence > 0.6) {
        FeedbackSignal signal;
        signal.source = "learner";
        signal.targetOptimization = (pattern.applicableOptimizations.empty() ?
                                    "unknown" :
                                    pattern.applicableOptimizations[0]);
        signal.reinforcementValue = pattern.confidence * 0.8;
        signal.reason = "Learned pattern: " + pattern.description;
        signal.priority = 5;
        collectedSignals_.push_back(signal);
    }
}

std::string Phase106FeedbackCoordinator::formatFeedbackRow(
    const FeedbackSignal& signal) {

    std::stringstream row;

    std::string optShort = signal.targetOptimization;
    if (optShort.length() > 20) {
        optShort = optShort.substr(0, 17) + "...";
    }

    row << std::left << std::setw(21) << optShort
        << "  " << std::setw(9) << signal.source
        << "  " << std::right << std::setw(13) << std::fixed << std::setprecision(2)
        << signal.reinforcementValue
        << "  " << std::setw(8) << signal.priority
        << "  " << signal.reason << "\n";

    return row.str();
}

std::string Phase106FeedbackCoordinator::formatAdjustmentRow(
    const StrategyAdjustment& adj) {

    std::stringstream row;

    row << "Adjustment: " << adj.recommendedChange << "\n";
    row << "  Expected Improvement: " << std::fixed << std::setprecision(1)
        << adj.expectedImprovementPercent << "%\n";
    row << "  Justification: " << adj.justification << "\n";

    return row.str();
}

} // namespace phase106
