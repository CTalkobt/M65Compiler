// Phase 106.3: Feedback Coordinator
// Coordinates feedback loops between profiling and optimization strategy selection

#pragma once

#include "Phase106OptimizationLearner.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace phase106 {

// Feedback signal from optimization effectiveness
struct FeedbackSignal {
    std::string source;           // "profiler", "metrics", "learner"
    std::string targetOptimization;
    double reinforcementValue = 0.0;  // -1.0 to +1.0 (negative = reduce, positive = increase)
    std::string reason;
    int priority = 0;             // 0-10, higher = more important
};

// Strategy adjustment recommendation
struct StrategyAdjustment {
    std::string currentStrategy;
    std::string recommendedChange;
    std::vector<std::string> optimizationsToPromote;
    std::vector<std::string> optimizationsToSuppress;
    double expectedImprovementPercent = 0.0;
    std::string justification;
};

// Feedback coordinator
class Phase106FeedbackCoordinator {
public:
    Phase106FeedbackCoordinator(Phase106OptimizationLearner& learner)
        : learner_(learner) {}

    // Collect feedback signals from multiple sources
    void collectFeedback(const IterationResult& iterationResult);

    // Analyze collected feedback
    std::vector<FeedbackSignal> analyzeFeedback();

    // Generate strategy adjustments from feedback
    StrategyAdjustment generateStrategyAdjustment(
        const std::vector<FeedbackSignal>& feedback,
        const std::string& currentStrategy,
        const phase105::AggregateMetrics& metrics);

    // Apply feedback to influence future iterations
    void applyFeedback(const std::vector<FeedbackSignal>& feedback);

    // Get feedback statistics
    struct FeedbackStats {
        int totalSignalsCollected = 0;
        int positiveSignals = 0;
        int negativeSignals = 0;
        double avgReinforcement = 0.0;
        std::map<std::string, int> signalsBySource;
    };

    FeedbackStats getFeedbackStatistics() const { return feedbackStats_; }

    // Generate feedback coordination report
    std::string generateFeedbackReport();

    // Get coordination statistics
    int getTotalAdjustmentsProposed() const { return adjustmentsProposed_; }

private:
    Phase106OptimizationLearner& learner_;
    std::vector<FeedbackSignal> collectedSignals_;
    FeedbackStats feedbackStats_;
    int adjustmentsProposed_ = 0;

    // Helper methods
    void signalFromMetrics(const phase105::AggregateMetrics& metrics);
    void signalFromComparisons(const std::vector<phase105::StrategyComparison>& comparisons);
    void signalFromLearning(const OptimizationPattern& pattern);
    std::string formatFeedbackRow(const FeedbackSignal& signal);
    std::string formatAdjustmentRow(const StrategyAdjustment& adj);
};

} // namespace phase106
