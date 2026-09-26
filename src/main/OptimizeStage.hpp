#pragma once
#include "Stage.hpp"
#include <memory>
#include <string>

class TranslationUnit;
class FunctionAnalyzer;

// AST-level optimization stage
// Performs constant folding, function analysis, inline selection, and cross-function optimizations
class OptimizeStage : public Stage {
public:
    OptimizeStage(std::shared_ptr<TranslationUnit> ast,
                 int optimizationLevel = 2,
                 int verboseLevel = 0,
                 bool inlineFunctions = false)
        : ast_(ast),
          optimizationLevel_(optimizationLevel),
          verboseLevel_(verboseLevel),
          inlineFunctions_(inlineFunctions) {}

    Result execute() override;
    std::string getName() const override { return "Optimize (AST)"; }

    // Get optimized AST
    std::shared_ptr<TranslationUnit> getAST() const { return ast_; }

    // Get function analyzer (needed for downstream codegen stage)
    std::shared_ptr<FunctionAnalyzer> getFunctionAnalyzer() const { return analyzer_; }

private:
    std::shared_ptr<TranslationUnit> ast_;
    int optimizationLevel_;
    int verboseLevel_;
    bool inlineFunctions_;
    std::shared_ptr<FunctionAnalyzer> analyzer_;

    // Helper methods for optimization phases
    void performConstantFolding();
    void performFunctionAnalysis();
    void performInlineSelection();
    void performCrossModuleOptimization();
    void performLoopOptimizations();
};
