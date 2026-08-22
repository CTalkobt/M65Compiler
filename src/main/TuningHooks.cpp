// Phase 107.2: Compiler Tuning Hooks Implementation
#include "TuningHooks.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <chrono>

namespace phase107 {

void TuningHooks::registerHook(const std::string& hookName,
                                      HookCallback callback) {
    hooks_[hookName] = callback;
    registeredHooks_.push_back(hookName);
    stats_[hookName] = {hookName, 0, 0, 0.0};
}

HookDecision TuningHooks::invokeHook(const HookInvocation& invocation) {
    auto startTime = std::chrono::high_resolution_clock::now();

    HookDecision decision;

    // Try online learner hook first if available
    if (onlineLearner_) {
        decision = invokeOnlineLearnerHook(invocation);
    }

    // Check for registered callbacks
    auto it = hooks_.find(invocation.hookName);
    if (it != hooks_.end()) {
        decision = it->second(invocation);
    }

    // Update statistics
    auto& hookStats = stats_[invocation.hookName];
    hookStats.invocationCount++;
    if (!decision.optimizationsToEnable.empty() ||
        !decision.optimizationsToDisable.empty()) {
        hookStats.decisionsChanged++;
    }

    auto elapsed = std::chrono::high_resolution_clock::now() - startTime;
    double elapsedMs = std::chrono::duration<double, std::milli>(elapsed).count();
    hookStats.avgDecisionTimeMs = (hookStats.avgDecisionTimeMs * (hookStats.invocationCount - 1) +
                                  elapsedMs) / hookStats.invocationCount;

    return decision;
}

HookDecision TuningHooks::invokeOnlineLearnerHook(
    const HookInvocation& invocation) {

    HookDecision decision;

    // Ask online learner about each available optimization
    for (const auto& optName : invocation.availableOptimizations) {
        // Estimate cost-benefit for this optimization
        OptimizationCostBenefit analysis = onlineLearner_->analyzeCostBenefit(
            optName, 15.0, 20.0, 50.0);  // Estimated values; real ones from profiler

        // Decide: should apply based on online learning decision
        if (onlineLearner_->shouldApplyOptimizationNow(analysis,
                                                      invocation.currentCompileTimeMs)) {
            decision.optimizationsToEnable.push_back(optName);
        } else {
            decision.optimizationsToDisable.push_back(optName);
        }
    }

    decision.rationale = "Online learner decision based on compile budget and effectiveness";
    return decision;
}

std::vector<TuningHooks::HookStats>
TuningHooks::getHookStatistics() const {
    std::vector<HookStats> result;
    for (const auto& [name, stat] : stats_) {
        result.push_back(stat);
    }
    return result;
}

std::string TuningHooks::generateTuningHooksReport() {
    std::stringstream report;

    report << "╔════════════════════════════════════════════════════════════════╗\n";
    report << "║     Phase 107.2: Compiler Tuning Hooks Report                  ║\n";
    report << "║     Dynamic Integration Points & Decision Statistics           ║\n";
    report << "╚════════════════════════════════════════════════════════════════╝\n\n";

    report << "REGISTERED HOOKS\n";
    report << "──────────────────────────────────────────────────────────────────\n";
    report << "Total Hooks:                " << registeredHooks_.size() << "\n";
    report << "Hooks: ";
    for (size_t i = 0; i < registeredHooks_.size(); ++i) {
        report << registeredHooks_[i];
        if (i < registeredHooks_.size() - 1) report << ", ";
    }
    report << "\n\n";

    report << "HOOK INVOCATION STATISTICS\n";
    report << "──────────────────────────────────────────────────────────────────\n";
    report << "Hook Name                 Invocations  Decisions Changed  Avg Time\n";
    report << "──────────────────────────────────────────────────────────────────\n";

    for (const auto& hookStats : getHookStatistics()) {
        report << std::left << std::setw(25) << hookStats.hookName
               << "  " << std::right << std::setw(11) << hookStats.invocationCount
               << "  " << std::setw(17) << hookStats.decisionsChanged
               << "  " << std::setw(8) << std::fixed << std::setprecision(2)
               << hookStats.avgDecisionTimeMs << " ms\n";
    }

    report << "\n";
    return report.str();
}

} // namespace phase107
