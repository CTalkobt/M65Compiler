#pragma once
#include "Phase108HookPoints.hpp"
#include <memory>
#include <string>

// Forward declarations
class OnlineLearner;

// Phase 108.3: Decision Logic - Applies learner decisions to optimization flags
class CompilerDecisionLogic {
public:
    CompilerDecisionLogic();
    ~CompilerDecisionLogic();

    // Set the learner reference (takes non-owning pointer)
    void setLearner(OnlineLearner* learner) { learner_ = learner; }

    // Make decision for optimization selection phase
    HookDecision decideOptimizationSelection(
        const CompilationSignal& signal,
        bool currentOptimizationState,
        double budgetMs);

    // Make decision for IR optimization phase
    HookDecision decideIROptimization(
        const CompilationSignal& signal,
        bool currentOptimizationState,
        double budgetMs);

    // Make decision for inlining
    HookDecision decideInlining(
        const CompilationSignal& signal,
        int inlineCandidates,
        double budgetMs);

    // Apply decision by modifying optimization flags
    void applyOptimizationFlags(
        bool& enableOptimizations,
        bool& enableInlining,
        bool& skipExpensive,
        const HookDecision& decision);

    // Get decision rationale
    std::string getRationale(const HookDecision& decision) const {
        return decision.rationale;
    }

    // Statistics
    int getTotalDecisions() const { return totalDecisions_; }
    int getDecisionsChanged() const { return decisionsChanged_; }

private:
    OnlineLearner* learner_;
    int totalDecisions_;
    int decisionsChanged_;

    // Helper: consult learner for optimization decision
    bool consultLearnerForOptimization(
        const std::string& optimizationName,
        double expectedBenefit,
        double currentCompileTime,
        double budgetMs);

    // Helper: estimate optimization benefit
    double estimateOptimizationBenefit(const CompilationSignal& signal);

    // Helper: make conservative decision if learner unavailable
    HookDecision makeConservativeDecision(
        const CompilationSignal& signal,
        double budgetMs);
};
