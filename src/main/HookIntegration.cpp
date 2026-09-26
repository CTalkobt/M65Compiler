#include "HookIntegration.hpp"
#include "CompilerHookIntegrator.hpp"
#include "CompilerDecisionLogic.hpp"
#include "LearnerFeedbackRecorder.hpp"
#include "Lexer.hpp"
#include "AST.hpp"
#include <iostream>

HookIntegration::HookIntegration()
    : verbose_(false),
      learner_(nullptr) {
    integrator_ = std::make_unique<CompilerHookIntegrator>();
    decisionLogic_ = std::make_unique<CompilerDecisionLogic>();
    lastDecision_ = makeDefaultDecision(CompilationSignal{});
}

HookIntegration::~HookIntegration() = default;

void HookIntegration::initializeCompilation(const std::string& inputFile, bool verbose, double budgetMs) {
    verbose_ = verbose;
    integrator_->initializeSession();
    integrator_->setVerbose(verbose);
    integrator_->setCompilationBudgetMs(budgetMs);

    if (verbose_) {
        std::cout << "[Phase108] Starting compilation: " << inputFile << std::endl;
        std::cout << "[Phase108] Budget: " << budgetMs << "ms" << std::endl;
    }
}

void HookIntegration::onPreParse() {
    if (!areHooksEnabled()) return;
    lastDecision_ = integrator_->invokePreParseHook();
    if (verbose_ && lastDecision_.changed) {
        std::cout << "[Phase108] PreParse decision: " << lastDecision_.rationale << std::endl;
    }
}

void HookIntegration::onPostLex(const std::vector<Token>& tokens) {
    if (!areHooksEnabled()) return;
    integrator_->collectLexerSignal(tokens);
    lastDecision_ = integrator_->invokePostLexHook();
    if (verbose_ && lastDecision_.changed) {
        std::cout << "[Phase108] PostLex decision: tokens=" << tokens.size()
                  << ", budget=" << lastDecision_.estimatedTimeRemaining << "ms" << std::endl;
    }
}

void HookIntegration::onPostParse(int astNodeCount) {
    if (!areHooksEnabled()) return;
    // Signal collector doesn't need the full AST, just metrics
    // astNodeCount can be used if needed for signal collection
    lastDecision_ = integrator_->invokePostParseHook();
    if (verbose_ && lastDecision_.changed) {
        std::cout << "[Phase108] PostParse decision: " << lastDecision_.rationale << std::endl;
    }
}

void HookIntegration::onPostConstFold(int constantsFolded, int deadCodeRemoved) {
    if (!areHooksEnabled()) return;
    integrator_->collectOptFoldSignal(constantsFolded, deadCodeRemoved);
    lastDecision_ = integrator_->invokePostConstFoldHook();
    if (verbose_ && lastDecision_.changed) {
        std::cout << "[Phase108] PostConstFold: folded=" << constantsFolded
                  << ", deadCode=" << deadCodeRemoved << std::endl;
    }
}

void HookIntegration::onPostFuncAnalysis(int functionsAnalyzed) {
    if (!areHooksEnabled()) return;
    integrator_->collectFuncAnalysisSignal(functionsAnalyzed);
    lastDecision_ = integrator_->invokePostFuncAnalysisHook();
    if (verbose_ && lastDecision_.changed) {
        std::cout << "[Phase108] PostFuncAnalysis: " << functionsAnalyzed
                  << " functions analyzed" << std::endl;
    }
}

void HookIntegration::onPreOptSelect() {
    if (!areHooksEnabled()) return;
    lastDecision_ = integrator_->invokePreOptSelectHook();
    if (verbose_ && lastDecision_.changed) {
        std::cout << "[Phase108] PreOptSelect decision: "
                  << (lastDecision_.enableOptimizations ? "ENABLE" : "DISABLE")
                  << " optimizations (budget=" << lastDecision_.estimatedTimeRemaining << "ms)"
                  << std::endl;
    }
}

void HookIntegration::onPostInlineSelect(int inlineCandidates) {
    if (!areHooksEnabled()) return;
    integrator_->collectInlineSelectSignal(inlineCandidates);
    lastDecision_ = integrator_->invokePostInlineSelectHook();
    if (verbose_ && lastDecision_.changed) {
        std::cout << "[Phase108] PostInlineSelect: " << inlineCandidates
                  << " candidates found" << std::endl;
    }
}

void HookIntegration::onPostCallGraph() {
    if (!areHooksEnabled()) return;
    lastDecision_ = integrator_->invokePostCallGraphHook();
    if (verbose_ && lastDecision_.changed) {
        std::cout << "[Phase108] PostCallGraph analysis complete" << std::endl;
    }
}

void HookIntegration::onPostIRBuild(const ir::Module& irModule) {
    if (!areHooksEnabled()) return;
    // Note: irModule is passed for potential signal collection but not used in current implementation
    // as signal collection focuses on high-level compilation metrics rather than IR structure
    lastDecision_ = integrator_->invokePostIRBuildHook();
    if (verbose_ && lastDecision_.changed) {
        std::cout << "[Phase108] PostIRBuild: IR building complete" << std::endl;
    }
}

void HookIntegration::onPreIROpt() {
    if (!areHooksEnabled()) return;
    lastDecision_ = integrator_->invokePreIROptHook();
    if (verbose_ && lastDecision_.changed) {
        std::cout << "[Phase108] PreIROpt decision: "
                  << (lastDecision_.skipExpensivePasses ? "SKIP" : "RUN")
                  << " expensive passes" << std::endl;
    }
}

void HookIntegration::onPostCodeGen(int assemblySize) {
    if (!areHooksEnabled()) return;
    integrator_->collectCodeGenSignal(assemblySize);
    lastDecision_ = integrator_->invokePostCodeGenHook();
    if (verbose_ && lastDecision_.changed) {
        std::cout << "[Phase108] PostCodeGen: " << assemblySize
                  << " bytes of assembly generated" << std::endl;
    }
}

void HookIntegration::onPostAsmOpt() {
    if (!areHooksEnabled()) return;
    integrator_->collectAsmOptSignal();
    lastDecision_ = integrator_->invokePostAsmOptHook();
    if (verbose_ && lastDecision_.changed) {
        std::cout << "[Phase108] PostAsmOpt: final optimization complete" << std::endl;
    }
}

void HookIntegration::printHookStatistics() const {
    integrator_->printStatistics(std::cout);
}

void HookIntegration::setLearner(OnlineLearner* learner) {
    learner_ = learner;
    decisionLogic_->setLearner(learner);

    // Phase 113: Initialize feedback recorder with new learner
    if (learner_) {
        feedbackRecorder_ = std::make_unique<LearnerFeedbackRecorder>(learner_);
        if (verbose_) {
            std::cout << "[Phase108] OnlineLearner integrated for decision logic" << std::endl;
            std::cout << "[Phase113] Feedback recorder initialized for learning loop" << std::endl;
        }
    }
}

HookDecision HookIntegration::makeOptimizationDecision(bool currentOptimizationState) {
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

HookDecision HookIntegration::makeIROptimizationDecision(bool currentOptimizationState) {
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

// Phase 113: Learning feedback loop methods
void HookIntegration::recordCompilationMetrics(const CompilationMetrics& metrics) {
    // Initialize feedback recorder if needed
    if (!feedbackRecorder_) {
        feedbackRecorder_ = std::make_unique<LearnerFeedbackRecorder>(learner_);
    }

    feedbackRecorder_->recordMetrics(metrics);

    if (verbose_) {
        std::cout << "[Phase113] Recorded metrics: " << metrics.appliedOptimizations.size()
                  << " optimizations applied, final size=" << metrics.finalAssemblySize
                  << " bytes" << std::endl;
    }
}

FeedbackResult HookIntegration::finalizeCompilationFeedback(const CompilationMetrics& metrics) {
    if (!feedbackRecorder_) {
        feedbackRecorder_ = std::make_unique<LearnerFeedbackRecorder>(learner_);
    }

    FeedbackResult result = feedbackRecorder_->finalizeCompilation(metrics);

    if (verbose_) {
        std::cout << "[Phase113] Finalized feedback: applied=" << result.optimizationsApplied
                  << ", beneficial=" << result.optimizationsBeneficial
                  << ", benefit_score=" << result.overallBenefitScore << std::endl;
    }

    return result;
}
