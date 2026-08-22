#pragma once
#include "CompilerHookIntegrator.hpp"
#include "CompilerDecisionLogic.hpp"
#include <memory>
#include <vector>
#include <string>

// Forward declarations
class Lexer;
class Token;
class TranslationUnit;
class OnlineLearner;
namespace ir { struct Module; }

// Phase 108.2: Integrates hooks into cc45_main compilation flow
class HookIntegration {
public:
    HookIntegration();
    ~HookIntegration();

    // Initialize for a compilation session
    void initializeCompilation(const std::string& inputFile, bool verbose, double budgetMs = 500.0);

    // Invoke hooks at each compilation checkpoint
    void onPreParse();
    void onPostLex(const std::vector<Token>& tokens);
    void onPostParse(int astNodeCount = 0);
    void onPostConstFold(int constantsFolded, int deadCodeRemoved);
    void onPostFuncAnalysis(int functionsAnalyzed);
    void onPreOptSelect();
    void onPostInlineSelect(int inlineCandidates);
    void onPostCallGraph();
    void onPostIRBuild(const ir::Module& irModule);
    void onPreIROpt();
    void onPostCodeGen(int assemblySize);
    void onPostAsmOpt();

    // Query hook decisions for optimization control
    HookDecision getLastDecision() const { return lastDecision_; }
    bool shouldContinueOptimizing() const { return lastDecision_.enableOptimizations; }
    bool shouldSkipExpensivePasses() const { return lastDecision_.skipExpensivePasses; }

    // Statistics and reporting
    void printHookStatistics() const;
    void setHooksEnabled(bool enabled) { integrator_->enableHooks(enabled); }
    bool areHooksEnabled() const { return integrator_->areHooksEnabled(); }

    // Learner integration (Phase 108.3)
    void setLearner(OnlineLearner* learner);
    OnlineLearner* getLearner() const { return learner_; }

    // Decision logic (Phase 108.3)
    HookDecision makeOptimizationDecision(bool currentOptimizationState);
    HookDecision makeIROptimizationDecision(bool currentOptimizationState);

private:
    std::unique_ptr<CompilerHookIntegrator> integrator_;
    std::unique_ptr<CompilerDecisionLogic> decisionLogic_;
    HookDecision lastDecision_;
    OnlineLearner* learner_;
    bool verbose_;
};
