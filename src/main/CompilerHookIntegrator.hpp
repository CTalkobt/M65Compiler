#pragma once
#include "Phase108HookPoints.hpp"
#include "CompilationSignalCollector.hpp"
#include <functional>
#include <map>
#include <vector>
#include <memory>

// Forward declarations
class OnlineLearner;
class TuningHooks;

// Frontend integrator: wires Phase 107 hooks into compiler pipeline
class CompilerHookIntegrator {
public:
    CompilerHookIntegrator();
    ~CompilerHookIntegrator();

    // Initialize integration for a compilation session
    void initializeSession();

    // Invoke hooks at each compilation phase
    HookDecision invokePreParseHook();
    HookDecision invokePostLexHook();
    HookDecision invokePostParseHook();
    HookDecision invokePostConstFoldHook();
    HookDecision invokePostFuncAnalysisHook();
    HookDecision invokePreOptSelectHook();
    HookDecision invokePostInlineSelectHook();
    HookDecision invokePostCallGraphHook();
    HookDecision invokePostIRBuildHook();
    HookDecision invokePreIROptHook();
    HookDecision invokePostCodeGenHook();
    HookDecision invokePostAsmOptHook();

    // Signal collection interface
    void collectSignal(const CompilationSignal& signal, HookPoint phase);
    void collectLexerSignal(const std::vector<Token>& tokens);
    void collectParseSignal(const std::shared_ptr<TranslationUnit>& ast);
    void collectOptFoldSignal(int folded, int deadCode);
    void collectFuncAnalysisSignal(int functionsAnalyzed);
    void collectInlineSelectSignal(int candidates);
    void collectCallGraphSignal(int callDensity);
    void collectIRBuildSignal(const IRModule& irModule);
    void collectCodeGenSignal(int asmSize);
    void collectAsmOptSignal();

    // Configuration
    void setLearnerRef(OnlineLearner* learner) { learner_ = learner; }
    void setHooksRef(TuningHooks* hooks) { hooks_ = hooks; }
    void enableHooks(bool enable) { hooksEnabled_ = enable; }
    void setVerbose(bool verbose) { verbose_ = verbose; }
    void setCompilationBudgetMs(double budgetMs) { budgetMs_ = budgetMs; }

    // Query current state
    bool areHooksEnabled() const { return hooksEnabled_; }
    double getElapsedTimeMs() const;
    double getRemainingBudgetMs() const;
    int getTotalHookInvocations() const { return totalHookInvocations_; }

    // Statistics
    void printStatistics(std::ostream& os) const;
    const std::vector<HookStatistics>& getStatistics() const { return statistics_; }

    // Hook registration for custom behaviors (advanced)
    using HookCallback = std::function<HookDecision(const CompilationSignal&)>;
    void registerCustomHook(HookPoint phase, HookCallback callback);

private:
    std::shared_ptr<SignalCollector> signalCollector_;
    OnlineLearner* learner_;       // Not owned
    TuningHooks* hooks_;           // Not owned
    bool hooksEnabled_;
    bool verbose_;
    double budgetMs_;

    std::chrono::high_resolution_clock::time_point sessionStart_;
    int totalHookInvocations_;
    std::vector<HookStatistics> statistics_;

    // Custom hook registry
    std::map<HookPoint, std::vector<HookCallback>> customHooks_;

    // Helper: invoke hooks and collect decision
    HookDecision invokeHooksInternal(HookPoint phase);

    // Helper: consult learner for decision
    HookDecision consultLearner(const CompilationSignal& signal);

    // Helper: apply hook decision
    void applyDecision(const HookDecision& decision);

    // Helper: record statistics
    void recordHookInvocation(HookPoint phase, const HookDecision& decision, double timeMs);

    // Helper: convert decision to debug string
    std::string decisionToString(const HookDecision& decision) const;
};
