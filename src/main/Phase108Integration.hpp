#pragma once
#include "Phase108FrontendIntegrator.hpp"
#include <memory>
#include <vector>
#include <string>

// Forward declarations
class Lexer;
class Token;
class TranslationUnit;
class IRModule;

// Phase 108.2: Integrates hooks into cc45_main compilation flow
class Phase108Integration {
public:
    Phase108Integration();
    ~Phase108Integration();

    // Initialize for a compilation session
    void initializeCompilation(const std::string& inputFile, bool verbose, double budgetMs = 500.0);

    // Invoke hooks at each compilation checkpoint
    void onPreParse();
    void onPostLex(const std::vector<Token>& tokens);
    void onPostParse(const std::shared_ptr<TranslationUnit>& ast);
    void onPostConstFold(int constantsFolded, int deadCodeRemoved);
    void onPostFuncAnalysis(int functionsAnalyzed);
    void onPreOptSelect();
    void onPostInlineSelect(int inlineCandidates);
    void onPostCallGraph();
    void onPostIRBuild(const IRModule& irModule);
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

private:
    std::unique_ptr<Phase108FrontendIntegrator> integrator_;
    HookDecision lastDecision_;
    bool verbose_;
};
