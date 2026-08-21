#include "Phase108SignalCollector.hpp"
#include "Lexer.hpp"
#include "AST.hpp"
#include <iostream>
#include <algorithm>

SignalCollector::SignalCollector()
    : isOptimizing_(true) {
    currentSignal_ = CompilationSignal{
        HookPoint::PreParse,
        0.0,
        0, 0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0,
        true, false, "PreParse"
    };
}

SignalCollector::~SignalCollector() = default;

void SignalCollector::initializeSession() {
    sessionStart_ = std::chrono::high_resolution_clock::now();
    currentSignal_.compileTimeSoFar = 0.0;
}

double SignalCollector::getElapsedTimeMs() const {
    auto now = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - sessionStart_);
    return elapsed.count() / 1000.0;
}

CompilationSignal SignalCollector::collectPreParseSignal() {
    currentSignal_.phase = HookPoint::PreParse;
    currentSignal_.compileTimeSoFar = getElapsedTimeMs();
    currentSignal_.currentPhase = "PreParse";
    currentSignal_.isOptimizing = isOptimizing_;
    return currentSignal_;
}

CompilationSignal SignalCollector::collectPostLexSignal(const std::vector<Token>& tokens) {
    currentSignal_.phase = HookPoint::PostLex;
    currentSignal_.compileTimeSoFar = getElapsedTimeMs();
    currentSignal_.tokenCount = static_cast<int>(tokens.size());
    currentSignal_.currentPhase = "PostLex";
    currentSignal_.isOptimizing = isOptimizing_;
    return currentSignal_;
}

CompilationSignal SignalCollector::collectPostParseSignal(const std::shared_ptr<TranslationUnit>& ast) {
    currentSignal_.phase = HookPoint::PostParse;
    currentSignal_.compileTimeSoFar = getElapsedTimeMs();
    if (ast) {
        currentSignal_.astNodeCount = countASTNodes(ast);
        // Count functions in AST
        currentSignal_.functionCount = 0;
        for (const auto& decl : ast->topLevelDecls) {
            // Count all declarations (simplified - just count them)
            currentSignal_.functionCount++;
        }
    }
    currentSignal_.currentPhase = "PostParse";
    currentSignal_.isOptimizing = isOptimizing_;
    return currentSignal_;
}

CompilationSignal SignalCollector::collectPostConstFoldSignal(int constantsFolded, int deadCodeRemoved) {
    currentSignal_.phase = HookPoint::PostConstFold;
    currentSignal_.compileTimeSoFar = getElapsedTimeMs();
    currentSignal_.constantsFolded = constantsFolded;
    currentSignal_.deadCodeRemoved = deadCodeRemoved;
    currentSignal_.currentPhase = "PostConstFold";
    currentSignal_.isOptimizing = isOptimizing_;
    return currentSignal_;
}

CompilationSignal SignalCollector::collectPostFuncAnalysisSignal(int functionsAnalyzed) {
    currentSignal_.phase = HookPoint::PostFuncAnalysis;
    currentSignal_.compileTimeSoFar = getElapsedTimeMs();
    currentSignal_.functionsAnalyzed = functionsAnalyzed;
    currentSignal_.currentPhase = "PostFuncAnalysis";
    currentSignal_.isOptimizing = isOptimizing_;
    return currentSignal_;
}

CompilationSignal SignalCollector::collectPreOptSelectSignal() {
    currentSignal_.phase = HookPoint::PreOptSelect;
    currentSignal_.compileTimeSoFar = getElapsedTimeMs();
    currentSignal_.currentPhase = "PreOptSelect";
    currentSignal_.isOptimizing = isOptimizing_;
    return currentSignal_;
}

CompilationSignal SignalCollector::collectPostInlineSelectSignal(int inlineCandidates) {
    currentSignal_.phase = HookPoint::PostInlineSelect;
    currentSignal_.compileTimeSoFar = getElapsedTimeMs();
    currentSignal_.inlineCandidates = inlineCandidates;
    currentSignal_.currentPhase = "PostInlineSelect";
    currentSignal_.isOptimizing = isOptimizing_;
    return currentSignal_;
}

CompilationSignal SignalCollector::collectPostCallGraphSignal(int callDensity) {
    currentSignal_.phase = HookPoint::PostCallGraph;
    currentSignal_.compileTimeSoFar = getElapsedTimeMs();
    currentSignal_.branchDensity = callDensity;
    currentSignal_.currentPhase = "PostCallGraph";
    currentSignal_.isOptimizing = isOptimizing_;
    return currentSignal_;
}

CompilationSignal SignalCollector::collectPostIRBuildSignal(const IRModule& irModule) {
    currentSignal_.phase = HookPoint::PostIRBuild;
    currentSignal_.compileTimeSoFar = getElapsedTimeMs();
    currentSignal_.irNodeCount = countIRNodes(irModule);
    currentSignal_.registerPressure = estimateRegisterPressure(irModule);
    currentSignal_.branchDensity = calculateBranchDensity(irModule);
    currentSignal_.currentPhase = "PostIRBuild";
    currentSignal_.isOptimizing = isOptimizing_;
    return currentSignal_;
}

CompilationSignal SignalCollector::collectPreIROptSignal() {
    currentSignal_.phase = HookPoint::PreIROpt;
    currentSignal_.compileTimeSoFar = getElapsedTimeMs();
    currentSignal_.currentPhase = "PreIROpt";
    currentSignal_.isOptimizing = isOptimizing_;
    return currentSignal_;
}

CompilationSignal SignalCollector::collectPostCodeGenSignal(int assemblySize) {
    currentSignal_.phase = HookPoint::PostCodeGen;
    currentSignal_.compileTimeSoFar = getElapsedTimeMs();
    currentSignal_.assemblySize = assemblySize;
    currentSignal_.currentPhase = "PostCodeGen";
    currentSignal_.isOptimizing = isOptimizing_;
    return currentSignal_;
}

CompilationSignal SignalCollector::collectPostAsmOptSignal() {
    currentSignal_.phase = HookPoint::PostAsmOpt;
    currentSignal_.compileTimeSoFar = getElapsedTimeMs();
    currentSignal_.currentPhase = "PostAsmOpt";
    currentSignal_.isOptimizing = isOptimizing_;
    return currentSignal_;
}

void SignalCollector::printSignalSummary(std::ostream& os) const {
    os << "=== Compilation Signal Summary ===" << std::endl;
    os << "Final Phase: " << hookPointToString(currentSignal_.phase) << std::endl;
    os << "Elapsed Time: " << currentSignal_.compileTimeSoFar << "ms" << std::endl;
    os << "Tokens: " << currentSignal_.tokenCount << std::endl;
    os << "AST Nodes: " << currentSignal_.astNodeCount << std::endl;
    os << "Functions: " << currentSignal_.functionCount << std::endl;
    os << "IR Nodes: " << currentSignal_.irNodeCount << std::endl;
    os << "Assembly Size: " << currentSignal_.assemblySize << " bytes" << std::endl;
}

int SignalCollector::countASTNodes(const std::shared_ptr<ASTNode>& node) {
    if (!node) return 0;
    int count = 1;
    // Recursively count child nodes
    // This is a simplified version - in practice would need a visitor pattern
    return count;
}

int SignalCollector::countIRNodes(const IRModule&) {
    // Placeholder: would count IR instructions in module
    return 0;
}

int SignalCollector::estimateRegisterPressure(const IRModule&) {
    // Placeholder: would analyze IR to estimate register usage
    return 4; // Conservative estimate: ~4 registers needed
}

int SignalCollector::calculateBranchDensity(const IRModule&) {
    // Placeholder: would calculate branches per 100 instructions
    return 15; // Conservative estimate: ~15%
}
