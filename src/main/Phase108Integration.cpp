#include "Phase108Integration.hpp"
#include "Phase108FrontendIntegrator.hpp"
#include "Phase108DecisionLogic.hpp"
#include "Lexer.hpp"
#include "AST.hpp"
#include <iostream>

Phase108Integration::Phase108Integration()
    : verbose_(false),
      learner_(nullptr) {
    integrator_ = std::make_unique<Phase108FrontendIntegrator>();
    decisionLogic_ = std::make_unique<Phase108DecisionLogic>();
    lastDecision_ = makeDefaultDecision(CompilationSignal{});
}

Phase108Integration::~Phase108Integration() = default;

void Phase108Integration::initializeCompilation(const std::string& inputFile, bool verbose, double budgetMs) {
    verbose_ = verbose;
    integrator_->initializeSession();
    integrator_->setVerbose(verbose);
    integrator_->setCompilationBudgetMs(budgetMs);

    if (verbose_) {
        std::cout << "[Phase108] Starting compilation: " << inputFile << std::endl;
        std::cout << "[Phase108] Budget: " << budgetMs << "ms" << std::endl;
    }
}

void Phase108Integration::onPreParse() {
    if (!areHooksEnabled()) return;
    lastDecision_ = integrator_->invokePreParseHook();
    if (verbose_ && lastDecision_.changed) {
        std::cout << "[Phase108] PreParse decision: " << lastDecision_.rationale << std::endl;
    }
}

void Phase108Integration::onPostLex(const std::vector<Token>& tokens) {
    if (!areHooksEnabled()) return;
    integrator_->collectLexerSignal(tokens);
    lastDecision_ = integrator_->invokePostLexHook();
    if (verbose_ && lastDecision_.changed) {
        std::cout << "[Phase108] PostLex decision: tokens=" << tokens.size()
                  << ", budget=" << lastDecision_.estimatedTimeRemaining << "ms" << std::endl;
    }
}

void Phase108Integration::onPostParse(int astNodeCount) {
    if (!areHooksEnabled()) return;
    // Signal collector doesn't need the full AST, just metrics
    // astNodeCount can be used if needed for signal collection
    lastDecision_ = integrator_->invokePostParseHook();
    if (verbose_ && lastDecision_.changed) {
        std::cout << "[Phase108] PostParse decision: " << lastDecision_.rationale << std::endl;
    }
}

void Phase108Integration::onPostConstFold(int constantsFolded, int deadCodeRemoved) {
    if (!areHooksEnabled()) return;
    integrator_->collectOptFoldSignal(constantsFolded, deadCodeRemoved);
    lastDecision_ = integrator_->invokePostConstFoldHook();
    if (verbose_ && lastDecision_.changed) {
        std::cout << "[Phase108] PostConstFold: folded=" << constantsFolded
                  << ", deadCode=" << deadCodeRemoved << std::endl;
    }
}

void Phase108Integration::onPostFuncAnalysis(int functionsAnalyzed) {
    if (!areHooksEnabled()) return;
    integrator_->collectFuncAnalysisSignal(functionsAnalyzed);
    lastDecision_ = integrator_->invokePostFuncAnalysisHook();
    if (verbose_ && lastDecision_.changed) {
        std::cout << "[Phase108] PostFuncAnalysis: " << functionsAnalyzed
                  << " functions analyzed" << std::endl;
    }
}

void Phase108Integration::onPreOptSelect() {
    if (!areHooksEnabled()) return;
    lastDecision_ = integrator_->invokePreOptSelectHook();
    if (verbose_ && lastDecision_.changed) {
        std::cout << "[Phase108] PreOptSelect decision: "
                  << (lastDecision_.enableOptimizations ? "ENABLE" : "DISABLE")
                  << " optimizations (budget=" << lastDecision_.estimatedTimeRemaining << "ms)"
                  << std::endl;
    }
}

void Phase108Integration::onPostInlineSelect(int inlineCandidates) {
    if (!areHooksEnabled()) return;
    integrator_->collectInlineSelectSignal(inlineCandidates);
    lastDecision_ = integrator_->invokePostInlineSelectHook();
    if (verbose_ && lastDecision_.changed) {
        std::cout << "[Phase108] PostInlineSelect: " << inlineCandidates
                  << " candidates found" << std::endl;
    }
}

void Phase108Integration::onPostCallGraph() {
    if (!areHooksEnabled()) return;
    lastDecision_ = integrator_->invokePostCallGraphHook();
    if (verbose_ && lastDecision_.changed) {
        std::cout << "[Phase108] PostCallGraph analysis complete" << std::endl;
    }
}

void Phase108Integration::onPostIRBuild(const IRModule& irModule) {
    if (!areHooksEnabled()) return;
    integrator_->collectIRBuildSignal(irModule);
    lastDecision_ = integrator_->invokePostIRBuildHook();
    if (verbose_ && lastDecision_.changed) {
        std::cout << "[Phase108] PostIRBuild: IR building complete" << std::endl;
    }
}

void Phase108Integration::onPreIROpt() {
    if (!areHooksEnabled()) return;
    lastDecision_ = integrator_->invokePreIROptHook();
    if (verbose_ && lastDecision_.changed) {
        std::cout << "[Phase108] PreIROpt decision: "
                  << (lastDecision_.skipExpensivePasses ? "SKIP" : "RUN")
                  << " expensive passes" << std::endl;
    }
}

void Phase108Integration::onPostCodeGen(int assemblySize) {
    if (!areHooksEnabled()) return;
    integrator_->collectCodeGenSignal(assemblySize);
    lastDecision_ = integrator_->invokePostCodeGenHook();
    if (verbose_ && lastDecision_.changed) {
        std::cout << "[Phase108] PostCodeGen: " << assemblySize
                  << " bytes of assembly generated" << std::endl;
    }
}

void Phase108Integration::onPostAsmOpt() {
    if (!areHooksEnabled()) return;
    integrator_->collectAsmOptSignal();
    lastDecision_ = integrator_->invokePostAsmOptHook();
    if (verbose_ && lastDecision_.changed) {
        std::cout << "[Phase108] PostAsmOpt: final optimization complete" << std::endl;
    }
}

void Phase108Integration::printHookStatistics() const {
    integrator_->printStatistics(std::cout);
}

void Phase108Integration::setLearner(Phase107OnlineLearner* learner) {
    learner_ = learner;
    decisionLogic_->setLearner(learner);
    if (verbose_ && learner_) {
        std::cout << "[Phase108] OnlineLearner integrated for decision logic" << std::endl;
    }
}

HookDecision Phase108Integration::makeOptimizationDecision(bool currentOptimizationState) {
    if (!decisionLogic_) {
        return makeDefaultDecision(CompilationSignal{});
    }

    CompilationSignal signal{};  // Empty signal for now
    return decisionLogic_->decideOptimizationSelection(
        signal,
        currentOptimizationState,
        integrator_->getRemainingBudgetMs()
    );
}

HookDecision Phase108Integration::makeIROptimizationDecision(bool currentOptimizationState) {
    if (!decisionLogic_) {
        return makeDefaultDecision(CompilationSignal{});
    }

    CompilationSignal signal{};  // Empty signal for now
    return decisionLogic_->decideIROptimization(
        signal,
        currentOptimizationState,
        integrator_->getRemainingBudgetMs()
    );
}
