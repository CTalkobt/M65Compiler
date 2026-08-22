// Phase 107.2: Compiler Tuning Hooks
// Dynamic integration points for strategy changes during compilation

#pragma once

#include "OnlineLearner.hpp"
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>

namespace phase107 {

// Hook invocation at a decision point in compilation
struct HookInvocation {
    std::string hookName;       // "optimizer_entry", "codegen_decision", etc
    std::string compilationPhase;
    double currentCompileTimeMs = 0.0;
    std::vector<std::string> availableOptimizations;
    std::vector<std::string> enabledOptimizations;
};

// Result from hook callback
struct HookDecision {
    std::vector<std::string> optimizationsToEnable;
    std::vector<std::string> optimizationsToDisable;
    std::string rationale;
    bool shouldProceed = true;  // can cancel phase if needed
};

// Compiler tuning hooks registry
class TuningHooks {
public:
    TuningHooks(OnlineLearner* learner = nullptr)
        : onlineLearner_(learner) {}

    // Register a hook at a decision point
    using HookCallback = std::function<HookDecision(const HookInvocation&)>;

    void registerHook(const std::string& hookName, HookCallback callback);

    // Invoke hook at a decision point (returns decisions)
    HookDecision invokeHook(const HookInvocation& invocation);

    // Get list of registered hooks
    const std::vector<std::string>& getRegisteredHooks() const {
        return registeredHooks_;
    }

    // Get hook statistics
    struct HookStats {
        std::string hookName;
        int invocationCount = 0;
        int decisionsChanged = 0;
        double avgDecisionTimeMs = 0.0;
    };

    std::vector<HookStats> getHookStatistics() const;

    // Generate tuning hooks report
    std::string generateTuningHooksReport();

private:
    OnlineLearner* onlineLearner_;
    std::map<std::string, HookCallback> hooks_;
    std::vector<std::string> registeredHooks_;
    std::map<std::string, HookStats> stats_;

    HookDecision invokeOnlineLearnerHook(const HookInvocation& invocation);
};

// Hook registration helper (RAII)
class HookRegistrar {
public:
    HookRegistrar(TuningHooks& registry,
                 const std::string& hookName,
                 TuningHooks::HookCallback callback)
        : registry_(registry) {
        registry_.registerHook(hookName, callback);
    }

private:
    TuningHooks& registry_;
};

} // namespace phase107
