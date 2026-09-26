#pragma once

#include "GlobalFunctionDatabase.hpp"
#include <vector>
#include <unordered_map>
#include <string>

// Decision for function specialization
struct SpecializationDecision {
    std::string baseFunctionName;
    std::string specializationName;      // e.g., "compute_spec_42_10"
    std::vector<ArgumentPattern> constantArgs;
    int estimatedSavings;
    bool isDecided;  // true = implement this specialization
};

// Decision for function inlining
struct InliningDecision {
    std::string functionName;
    bool shouldInline;
    int callSites;
    int codeSize;
    float savingsEstimate;
    std::string reason;  // Why inline or not
};

// Decision for dead code elimination
struct DeadCodeDecision {
    std::string functionName;
    bool isDeadCode;
    std::string reason;
};

// Complete IPO analysis result
struct IPOAnalysisResult {
    std::vector<SpecializationDecision> specializations;
    std::vector<InliningDecision> inlines;
    std::vector<DeadCodeDecision> deadCode;
    int estimatedTotalSavings;
};

class IPOAnalyzer {
public:
    IPOAnalyzer() = default;

    // Analyze global database and make IPO decisions
    IPOAnalysisResult analyze(const GlobalFunctionDatabase& db);

    // Decision thresholds (configurable)
    void setInlineThreshold(int bytes) { inlineThreshold_ = bytes; }
    void setSpecializationROIThreshold(float roi) { roiThreshold_ = roi; }
    void setDeadCodeThreshold(int unused) { deadCodeThreshold_ = unused; }

    // Print analysis decisions
    void printDecisions(const IPOAnalysisResult& result) const;

private:
    int inlineThreshold_ = 20;      // Functions < 20 bytes good candidates for inlining
    float roiThreshold_ = 1.5f;     // Require ROI > 1.5 for specialization
    int deadCodeThreshold_ = 0;     // Any unused function is dead code

    // Helper methods
    std::vector<InliningDecision> analyzeInlining(const GlobalFunctionDatabase& db);
    std::vector<SpecializationDecision> analyzeSpecialization(const GlobalFunctionDatabase& db);
    std::vector<DeadCodeDecision> analyzeDeadCode(const GlobalFunctionDatabase& db);

    // Calculate inlining benefit
    float calculateInliningBenefit(const GlobalFunctionProfile* profile) const;

    // Estimate code size impact
    int estimateCodeSizeImpact(const GlobalFunctionProfile* profile) const;
};
