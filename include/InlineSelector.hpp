#pragma once

#include "AST.hpp"
#include "FunctionAnalyzer.hpp"
#include <set>
#include <string>

// Profile-guided inline expansion selector
// Uses function characteristics to determine which functions should be inlined
class InlineSelector {
public:
    struct InlineHints {
        bool shouldInline = false;
        int inlineThreshold = 0;  // Size threshold for this function
        std::string reason;       // Why this decision was made
    };

    InlineSelector();

    // Analyze translation unit and mark functions for inlining
    void selectInlineCandidates(TranslationUnit& unit,
                               FunctionAnalyzer& analyzer);

    // Get inlining decision for a specific function
    InlineHints getInlineHints(const std::string& funcName) const;

private:
    std::map<std::string, InlineHints> inlineHints_;

    InlineHints computeInlineDecision(
        const FunctionAnalyzer::FunctionCharacteristics* chars);
};
