#include "CompilerHookIntegrator.hpp"
#include <iostream>
#include <chrono>
#include <algorithm>
#include <iomanip>

// Forward reference check - OnlineLearner and TuningHooks are optional
// (not included here to avoid circular dependency)

CompilerHookIntegrator::CompilerHookIntegrator()
    : learner_(nullptr),
      hooks_(nullptr),
      hooksEnabled_(true),
      verbose_(false),
      budgetMs_(500.0),
      totalHookInvocations_(0) {
    signalCollector_ = std::make_shared<SignalCollector>();
}

CompilerHookIntegrator::~CompilerHookIntegrator() = default;

void CompilerHookIntegrator::initializeSession() {
    signalCollector_->initializeSession();
    sessionStart_ = std::chrono::high_resolution_clock::now();
    totalHookInvocations_ = 0;
    statistics_.clear();

    if (verbose_) {
        std::cout << "[Phase108] Session initialized. Budget: " << budgetMs_ << "ms" << std::endl;
    }
}

double CompilerHookIntegrator::getElapsedTimeMs() const {
    auto now = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - sessionStart_);
    return elapsed.count() / 1000.0;
}

double CompilerHookIntegrator::getRemainingBudgetMs() const {
    double elapsed = getElapsedTimeMs();
    return std::max(0.0, budgetMs_ - elapsed);
}

HookDecision CompilerHookIntegrator::invokePreParseHook() {
    if (!hooksEnabled_) {
        return makeDefaultDecision(signalCollector_->collectPreParseSignal());
    }
    return invokeHooksInternal(HookPoint::PreParse);
}

HookDecision CompilerHookIntegrator::invokePostLexHook() {
    if (!hooksEnabled_) {
        return makeDefaultDecision(signalCollector_->collectPreParseSignal());
    }
    return invokeHooksInternal(HookPoint::PostLex);
}

HookDecision CompilerHookIntegrator::invokePostParseHook() {
    if (!hooksEnabled_) {
        return makeDefaultDecision(signalCollector_->collectPreParseSignal());
    }
    return invokeHooksInternal(HookPoint::PostParse);
}

HookDecision CompilerHookIntegrator::invokePostConstFoldHook() {
    if (!hooksEnabled_) {
        return makeDefaultDecision(signalCollector_->collectPreParseSignal());
    }
    return invokeHooksInternal(HookPoint::PostConstFold);
}

HookDecision CompilerHookIntegrator::invokePostFuncAnalysisHook() {
    if (!hooksEnabled_) {
        return makeDefaultDecision(signalCollector_->collectPreParseSignal());
    }
    return invokeHooksInternal(HookPoint::PostFuncAnalysis);
}

HookDecision CompilerHookIntegrator::invokePreOptSelectHook() {
    if (!hooksEnabled_) {
        return makeDefaultDecision(signalCollector_->collectPreOptSelectSignal());
    }
    return invokeHooksInternal(HookPoint::PreOptSelect);
}

HookDecision CompilerHookIntegrator::invokePostInlineSelectHook() {
    if (!hooksEnabled_) {
        return makeDefaultDecision(signalCollector_->collectPreParseSignal());
    }
    return invokeHooksInternal(HookPoint::PostInlineSelect);
}

HookDecision CompilerHookIntegrator::invokePostCallGraphHook() {
    if (!hooksEnabled_) {
        return makeDefaultDecision(signalCollector_->collectPreParseSignal());
    }
    return invokeHooksInternal(HookPoint::PostCallGraph);
}

HookDecision CompilerHookIntegrator::invokePostIRBuildHook() {
    if (!hooksEnabled_) {
        return makeDefaultDecision(signalCollector_->collectPreParseSignal());
    }
    return invokeHooksInternal(HookPoint::PostIRBuild);
}

HookDecision CompilerHookIntegrator::invokePreIROptHook() {
    if (!hooksEnabled_) {
        return makeDefaultDecision(signalCollector_->collectPreIROptSignal());
    }
    return invokeHooksInternal(HookPoint::PreIROpt);
}

HookDecision CompilerHookIntegrator::invokePostCodeGenHook() {
    if (!hooksEnabled_) {
        return makeDefaultDecision(signalCollector_->collectPreParseSignal());
    }
    return invokeHooksInternal(HookPoint::PostCodeGen);
}

HookDecision CompilerHookIntegrator::invokePostAsmOptHook() {
    if (!hooksEnabled_) {
        return makeDefaultDecision(signalCollector_->collectPreParseSignal());
    }
    return invokeHooksInternal(HookPoint::PostAsmOpt);
}

void CompilerHookIntegrator::collectLexerSignal(const std::vector<Token>& tokens) {
    signalCollector_->collectPostLexSignal(tokens);
}

void CompilerHookIntegrator::collectParseSignal(const std::shared_ptr<TranslationUnit>& ast) {
    signalCollector_->collectPostParseSignal(ast);
}

void CompilerHookIntegrator::collectOptFoldSignal(int folded, int deadCode) {
    signalCollector_->collectPostConstFoldSignal(folded, deadCode);
}

void CompilerHookIntegrator::collectFuncAnalysisSignal(int functionsAnalyzed) {
    signalCollector_->collectPostFuncAnalysisSignal(functionsAnalyzed);
}

void CompilerHookIntegrator::collectInlineSelectSignal(int candidates) {
    signalCollector_->collectPostInlineSelectSignal(candidates);
}

void CompilerHookIntegrator::collectCallGraphSignal(int callDensity) {
    signalCollector_->collectPostCallGraphSignal(callDensity);
}

void CompilerHookIntegrator::collectIRBuildSignal(const IRModule& irModule) {
    signalCollector_->collectPostIRBuildSignal(irModule);
}

void CompilerHookIntegrator::collectCodeGenSignal(int asmSize) {
    signalCollector_->collectPostCodeGenSignal(asmSize);
}

void CompilerHookIntegrator::collectAsmOptSignal() {
    signalCollector_->collectPostAsmOptSignal();
}

void CompilerHookIntegrator::registerCustomHook(HookPoint phase, HookCallback callback) {
    customHooks_[phase].push_back(callback);
}

HookDecision CompilerHookIntegrator::invokeHooksInternal(HookPoint phase) {
    auto startTime = std::chrono::high_resolution_clock::now();
    HookDecision decision = makeDefaultDecision(signalCollector_->getCurrentSignal());

    // Consult learner for decision
    decision = consultLearner(signalCollector_->getCurrentSignal());

    // Invoke custom hooks if registered
    if (customHooks_.find(phase) != customHooks_.end()) {
        for (const auto& callback : customHooks_[phase]) {
            HookDecision customDecision = callback(signalCollector_->getCurrentSignal());
            // Merge decisions (custom hooks can override learner decision)
            if (customDecision.changed) {
                decision = customDecision;
            }
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    double elapsedMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();

    recordHookInvocation(phase, decision, elapsedMs);
    totalHookInvocations_++;

    if (verbose_) {
        std::cout << "[Phase108] " << hookPointToString(phase)
                  << " -> " << decisionToString(decision)
                  << " (" << std::fixed << std::setprecision(2) << elapsedMs << "ms)"
                  << std::endl;
    }

    return decision;
}

HookDecision CompilerHookIntegrator::consultLearner(const CompilationSignal& signal) {
    if (!learner_) {
        return makeDefaultDecision(signal);
    }

    // TODO: Consult online learner for optimization decision when OnlineLearner is fully integrated
    // For now, use simple heuristic based on budget
    double remainingBudget = getRemainingBudgetMs();
    bool shouldOptimize = remainingBudget > 50;  // Continue optimizing if > 50ms budget remains

    HookDecision decision;
    decision.changed = shouldOptimize != signal.isOptimizing;
    decision.enableOptimizations = shouldOptimize;
    decision.enableIROpts = shouldOptimize;
    decision.enableInlining = shouldOptimize && (remainingBudget > 100);
    decision.skipExpensivePasses = !shouldOptimize || (remainingBudget < 50);
    decision.estimatedTimeRemaining = remainingBudget;
    decision.rationale = shouldOptimize ? "learner: continue optimizing" : "learner: budget constraint";

    return decision;
}

void CompilerHookIntegrator::applyDecision(const HookDecision&) {
    // This would be called by the compiler frontend to apply the decision
    // Implementation depends on how flags are managed in cc45_main
    // For Phase 108.1, this is a placeholder for Phase 108.2 integration
}

void CompilerHookIntegrator::recordHookInvocation(HookPoint phase, const HookDecision& decision, double timeMs) {
    // Find or create statistics entry for this hook
    auto it = std::find_if(statistics_.begin(), statistics_.end(),
        [phase](const HookStatistics& hs) { return hs.phase == phase; });

    if (it != statistics_.end()) {
        it->invocationCount++;
        if (decision.changed) it->decisionsChanged++;
        it->totalTimeMs += timeMs;
    } else {
        HookStatistics stats;
        stats.phase = phase;
        stats.invocationCount = 1;
        stats.decisionsChanged = decision.changed ? 1 : 0;
        stats.averageDecisionTimeMs = timeMs;
        stats.totalTimeMs = timeMs;
        statistics_.push_back(stats);
    }
}

std::string CompilerHookIntegrator::decisionToString(const HookDecision& decision) const {
    if (decision.skipExpensivePasses) {
        return "SKIP_EXPENSIVE";
    } else if (decision.enableOptimizations) {
        return "OPTIMIZE";
    } else {
        return "NO_OPT";
    }
}

void CompilerHookIntegrator::printStatistics(std::ostream& os) const {
    os << "\n=== Phase 108 Hook Statistics ===" << std::endl;
    os << "Total Hook Invocations: " << totalHookInvocations_ << std::endl;
    os << "Compilation Time: " << std::fixed << std::setprecision(2) << getElapsedTimeMs() << "ms" << std::endl;
    os << "Budget: " << budgetMs_ << "ms" << std::endl;
    os << std::endl;

    os << "Hook Invocations:" << std::endl;
    os << std::left << std::setw(25) << "Phase"
       << std::setw(12) << "Invocations"
       << std::setw(12) << "Decisions"
       << std::setw(12) << "Avg Time(ms)" << std::endl;
    os << std::string(61, '-') << std::endl;

    for (const auto& stat : statistics_) {
        os << std::left << std::setw(25) << hookPointToString(stat.phase)
           << std::setw(12) << stat.invocationCount
           << std::setw(12) << stat.decisionsChanged
           << std::setw(12) << std::fixed << std::setprecision(3) << stat.getAverageTime() << std::endl;
    }
}
