#pragma once
#include "Phase108HookPoints.hpp"
#include <chrono>
#include <vector>
#include <memory>

// Forward declarations
class Token;
class ASTNode;
class TranslationUnit;
class IRModule;
class IRNode;
class IRBuilder;

// Collects compilation signals at each hook point
class SignalCollector {
public:
    SignalCollector();
    ~SignalCollector();

    // Initialize signal collection for a compilation session
    void initializeSession();

    // Signal collection methods for each phase
    CompilationSignal collectPreParseSignal();
    CompilationSignal collectPostLexSignal(const std::vector<Token>& tokens);
    CompilationSignal collectPostParseSignal(const std::shared_ptr<TranslationUnit>& ast);
    CompilationSignal collectPostConstFoldSignal(int constantsFolded, int deadCodeRemoved);
    CompilationSignal collectPostFuncAnalysisSignal(int functionsAnalyzed);
    CompilationSignal collectPreOptSelectSignal();
    CompilationSignal collectPostInlineSelectSignal(int inlineCandidates);
    CompilationSignal collectPostCallGraphSignal(int callDensity);
    CompilationSignal collectPostIRBuildSignal(const IRModule& irModule);
    CompilationSignal collectPreIROptSignal();
    CompilationSignal collectPostCodeGenSignal(int assemblySize);
    CompilationSignal collectPostAsmOptSignal();

    // Get current signal
    const CompilationSignal& getCurrentSignal() const { return currentSignal_; }

    // Set compilation state
    void setOptimizingState(bool optimizing) { isOptimizing_ = optimizing; }
    void reportError(const std::string&) {
        currentSignal_.hasErrors = true;
    }

    // Get metrics summary
    void printSignalSummary(std::ostream& os) const;

private:
    CompilationSignal currentSignal_;
    std::chrono::high_resolution_clock::time_point sessionStart_;
    bool isOptimizing_;

    // Helper to get elapsed time since session start
    double getElapsedTimeMs() const;

    // Helper to count AST nodes
    int countASTNodes(const std::shared_ptr<ASTNode>& node);

    // Helper to count IR nodes
    int countIRNodes(const IRModule& irModule);

    // Helper to estimate register pressure
    int estimateRegisterPressure(const IRModule& irModule);

    // Helper to calculate branch density
    int calculateBranchDensity(const IRModule& irModule);
};
