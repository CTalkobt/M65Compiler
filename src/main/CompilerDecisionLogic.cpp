#include "CompilerDecisionLogic.hpp"
#include "OnlineLearner.hpp"

CompilerDecisionLogic::CompilerDecisionLogic()
    : learner_(nullptr),
      totalDecisions_(0),
      decisionsChanged_(0) {
}

CompilerDecisionLogic::~CompilerDecisionLogic() = default;

HookDecision CompilerDecisionLogic::decideOptimizationSelection(
    const CompilationSignal& signal,
    bool currentOptimizationState,
    double budgetMs) {

    totalDecisions_++;

    // Check if we have budget for optimizations
    if (budgetMs < 50) {
        // Very tight budget: skip optimizations
        HookDecision decision;
        decision.changed = true;
        decision.enableOptimizations = false;
        decision.enableIROpts = false;
        decision.enableInlining = false;
        decision.skipExpensivePasses = true;
        decision.estimatedTimeRemaining = budgetMs;
        decision.rationale = "Phase108: Budget constraint (< 50ms)";
        decisionsChanged_++;
        return decision;
    }

    // Estimate optimization benefit based on compilation metrics
    double estimatedBenefit = estimateOptimizationBenefit(signal);

    // If we have learner, consult it
    if (learner_) {
        bool shouldOptimize = consultLearnerForOptimization(
            "optimization-selection",
            estimatedBenefit,
            signal.compileTimeSoFar,
            budgetMs);

        HookDecision decision;
        decision.changed = (shouldOptimize != currentOptimizationState);
        decision.enableOptimizations = shouldOptimize;
        decision.enableIROpts = shouldOptimize;
        decision.enableInlining = shouldOptimize && (budgetMs > 100);
        decision.skipExpensivePasses = !shouldOptimize;
        decision.estimatedTimeRemaining = budgetMs;
        decision.rationale = shouldOptimize ?
            "Phase108: Learner recommends optimization" :
            "Phase108: Learner suggests skipping optimization";

        if (decision.changed) decisionsChanged_++;
        return decision;
    }

    // Conservative decision without learner
    return makeConservativeDecision(signal, budgetMs);
}

HookDecision CompilerDecisionLogic::decideIROptimization(
    const CompilationSignal& signal,
    bool currentOptimizationState,
    double budgetMs) {

    totalDecisions_++;

    // Check IR complexity to decide on expensive passes
    int estimatedCost = 100;  // Estimate: IR optimization costs ~100ms
    if (budgetMs < estimatedCost + 50) {
        // Not enough budget for IR optimization + remaining compilation
        HookDecision decision;
        decision.changed = true;
        decision.enableOptimizations = false;
        decision.enableIROpts = false;
        decision.enableInlining = false;
        decision.skipExpensivePasses = true;
        decision.estimatedTimeRemaining = budgetMs;
        decision.rationale = "Phase108: Insufficient budget for IR optimization";
        decisionsChanged_++;
        return decision;
    }

    // Consult learner if available
    if (learner_) {
        double irBenefit = (signal.irNodeCount > 0) ?
            (10.0 / signal.irNodeCount) : 1.0;  // Small functions = less benefit

        bool shouldOptimize = consultLearnerForOptimization(
            "ir-optimization",
            irBenefit,
            signal.compileTimeSoFar,
            budgetMs);

        HookDecision decision;
        decision.changed = (shouldOptimize != currentOptimizationState);
        decision.enableOptimizations = shouldOptimize;
        decision.enableIROpts = shouldOptimize;
        decision.enableInlining = shouldOptimize && (budgetMs > 100);
        decision.skipExpensivePasses = !shouldOptimize;
        decision.estimatedTimeRemaining = budgetMs;
        decision.rationale = shouldOptimize ?
            "Phase108: Apply IR optimizations" :
            "Phase108: Skip expensive IR passes";

        if (decision.changed) decisionsChanged_++;
        return decision;
    }

    return makeConservativeDecision(signal, budgetMs);
}

HookDecision CompilerDecisionLogic::decideInlining(
    const CompilationSignal& signal,
    int inlineCandidates,
    double budgetMs) {

    totalDecisions_++;

    // Inlining is expensive: estimate 50-200ms
    int inlineCost = 50 + (inlineCandidates * 10);
    if (budgetMs < inlineCost + 100) {
        // Skip inlining if budget tight
        HookDecision decision;
        decision.changed = true;
        decision.enableOptimizations = true;
        decision.enableIROpts = true;
        decision.enableInlining = false;
        decision.skipExpensivePasses = true;
        decision.estimatedTimeRemaining = budgetMs;
        decision.rationale = "Phase108: Budget too tight for inlining";
        decisionsChanged_++;
        return decision;
    }

    HookDecision decision;
    decision.changed = false;
    decision.enableInlining = (budgetMs > 100) && (inlineCandidates > 0);
    decision.skipExpensivePasses = (budgetMs < 150);
    decision.estimatedTimeRemaining = budgetMs;
    decision.rationale = decision.enableInlining ?
        "Phase108: Enable inlining" :
        "Phase108: Defer inlining";

    return decision;
}

void CompilerDecisionLogic::applyOptimizationFlags(
    bool& enableOptimizations,
    bool& enableInlining,
    bool& skipExpensive,
    const HookDecision& decision) {

    enableOptimizations = decision.enableOptimizations;
    enableInlining = decision.enableInlining;
    skipExpensive = decision.skipExpensivePasses;
}

bool CompilerDecisionLogic::consultLearnerForOptimization(
    const std::string& optimizationName,
    double expectedBenefit,
    double currentCompileTime,
    double budgetMs) {

    if (!learner_) return true;  // Default: optimize if no learner

    // Use learner to decide
    // For now, simple heuristic: if benefit > 1.0% and budget available
    return (expectedBenefit > 1.0) && (budgetMs > 50);
}

double CompilerDecisionLogic::estimateOptimizationBenefit(
    const CompilationSignal& signal) {

    // Estimate benefit based on code metrics
    // Larger code benefits more from optimization
    double codeSize = signal.astNodeCount + signal.functionCount * 10;
    return (codeSize > 1000) ? 5.0 : (codeSize / 200.0);  // 0-5% benefit
}

HookDecision CompilerDecisionLogic::makeConservativeDecision(
    const CompilationSignal& signal,
    double budgetMs) {

    HookDecision decision;
    decision.changed = false;
    decision.enableOptimizations = (budgetMs > 100);
    decision.enableIROpts = (budgetMs > 150);
    decision.enableInlining = (budgetMs > 200);
    decision.skipExpensivePasses = (budgetMs < 100);
    decision.estimatedTimeRemaining = budgetMs;
    decision.rationale = "Phase108: Conservative decision (no learner)";

    return decision;
}
