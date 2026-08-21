// Phase 107.1: Online Learning Engine
// Learns during compilation and adapts optimization strategy in real-time

#pragma once

#include "Phase106FeedbackCoordinator.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>

namespace phase107 {

// Real-time profiling signal during compilation
struct CompilationSignal {
    std::string signalType;  // "code_size", "compile_time", "register_pressure", "branch_density"
    double value = 0.0;
    std::string phase;       // which compilation phase (lexer, parser, optimizer, etc)
    std::chrono::milliseconds timestamp;
    int priority = 5;        // 0-10, higher = more important
};

// Cost-benefit analysis for an optimization
struct OptimizationCostBenefit {
    std::string optimizationName;

    double expectedSpeedup = 0.0;        // % faster
    double expectedSizeReduction = 0.0;  // % smaller
    double compilationCostMs = 0.0;      // ms overhead

    double benefitScore = 0.0;           // (speedup + size_red) / cost
    double worthwhileThreshold = 1.5;    // must exceed for application

    bool shouldApply = false;
    std::string justification;
};

// Online learning state for a compilation session
struct OnlineLearningState {
    std::string compilationSession;
    std::vector<CompilationSignal> collectedSignals;
    std::vector<OptimizationCostBenefit> costBenefitAnalyses;

    std::map<std::string, double> optimizationEffectiveness;  // real-time scores
    std::map<std::string, int> optimizationApplicationCount;  // times applied

    double overallProgressScore = 0.0;   // 0-100, cumulative benefit
    int decisionsAdapted = 0;            // strategy changes mid-compilation
};

// Online learner coordinator
class Phase107OnlineLearner {
public:
    Phase107OnlineLearner() = default;

    // Initialize online learning for a compilation
    void initializeSession(const std::string& sessionId);

    // Collect signal during compilation
    void collectSignal(const CompilationSignal& signal);

    // Analyze cost-benefit for an optimization at decision point
    OptimizationCostBenefit analyzeCostBenefit(
        const std::string& optimizationName,
        double expectedSpeedup,
        double expectedSizeReduction,
        double compilationCostMs);

    // Decide whether to apply an optimization NOW (real-time decision)
    bool shouldApplyOptimizationNow(
        const OptimizationCostBenefit& analysis,
        double currentCompileTimeMs);

    // Update effectiveness based on early profiling results
    void updateOptimizationEffectiveness(
        const std::string& optimizationName,
        double observedSpeedup);

    // Get current session state
    const OnlineLearningState& getSessionState() const { return currentSession_; }

    // Generate online learning report
    std::string generateOnlineLearningReport();

    // Get learner statistics
    int getTotalSignalsCollected() const { return totalSignalsCollected_; }
    int getTotalDecisionsAdapted() const { return totalDecisionsAdapted_; }
    double getAverageBenefitScore() const { return avgBenefitScore_; }

private:
    OnlineLearningState currentSession_;
    std::vector<OnlineLearningState> allSessions_;

    int totalSignalsCollected_ = 0;
    int totalDecisionsAdapted_ = 0;
    double avgBenefitScore_ = 0.0;

    // Helper methods
    double computeBenefitScore(double speedup, double sizeReduction,
                              double compilationCost);
    std::string formatSignalRow(const CompilationSignal& signal);
    std::string formatCostBenefitRow(const OptimizationCostBenefit& analysis);
};

} // namespace phase107
