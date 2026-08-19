#pragma once

#include "AST.hpp"
#include "FunctionAnalyzer.hpp"

// Maps function characteristics to per-function optimization flags
class OptimizationSelector {
public:
    struct OptimizationFlags {
        bool enableLoopUnrolling = true;
        bool enableLoopInterchange = true;
        bool enable2PassFolding = false;
        bool enableSAC = true;
        int unrollFactor = 4;  // Default: 4
    };

    OptimizationSelector(int globalUnrollFactor = 4);

    // Analyze translation unit and apply per-function flags
    void selectOptimizations(TranslationUnit& unit, FunctionAnalyzer& analyzer);

private:
    int globalUnrollFactor_;

    OptimizationFlags selectForFunction(
        const FunctionAnalyzer::FunctionCharacteristics* chars);

    int computeUnrollFactor(const FunctionAnalyzer::FunctionCharacteristics* chars);
};
