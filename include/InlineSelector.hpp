#pragma once

#include "AST.hpp"
#include "FunctionAnalyzer.hpp"
#include <set>
#include <string>

// Forward declarations for Phase 87
class CallGraphAnalyzer;
class CoOptimizationSelector;

// Profile-guided inline expansion selector
// Uses function characteristics to determine which functions should be inlined
// Phase 87: Extended with cross-function optimization recommendations
class InlineSelector {
public:
    struct InlineHints {
        bool shouldInline = false;
        int inlineThreshold = 0;  // Size threshold for this function
        std::string reason;       // Why this decision was made
        bool fromCrossFunc = false; // Phase 87: From cross-function optimization
        int benefitScore = 0;      // Phase 87: Estimated benefit of inlining
    };

    InlineSelector(int optimizationLevel = 2);

    // Analyze translation unit and mark functions for inlining (Phase 85)
    void selectInlineCandidates(TranslationUnit& unit,
                               FunctionAnalyzer& analyzer);

    // Phase 87: Apply cross-function optimization recommendations
    void applyRecommendations(const CallGraphAnalyzer* callGraph,
                             const CoOptimizationSelector* selector);

    // Phase 88: Apply recommendations to AST nodes for actual inlining
    void applyRecommendationsToAST(TranslationUnit& unit);

    // Get inlining decision for a specific function
    InlineHints getInlineHints(const std::string& funcName) const;

private:
    int optimizationLevel_;
    std::map<std::string, InlineHints> inlineHints_;

    InlineHints computeInlineDecision(
        const FunctionAnalyzer::FunctionCharacteristics* chars);
};
