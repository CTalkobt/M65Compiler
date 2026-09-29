#include "IPOAnalyzer.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>

IPOAnalysisResult IPOAnalyzer::analyze(const GlobalFunctionDatabase& db) {
    IPOAnalysisResult result;

    // Analyze each optimization type
    result.inlines = analyzeInlining(db);
    result.specializations = analyzeSpecialization(db);
    result.deadCode = analyzeDeadCode(db);

    // Calculate total estimated savings
    int totalSavings = 0;
    for (const auto& inl : result.inlines) {
        if (inl.shouldInline) {
            totalSavings += static_cast<int>(inl.savingsEstimate);
        }
    }
    for (const auto& spec : result.specializations) {
        if (spec.isDecided) {
            totalSavings += spec.estimatedSavings;
        }
    }

    result.estimatedTotalSavings = totalSavings;
    return result;
}

std::vector<InliningDecision> IPOAnalyzer::analyzeInlining(const GlobalFunctionDatabase& db) {
    std::vector<InliningDecision> decisions;

    // Get all external functions
    auto externalFuncs = const_cast<GlobalFunctionDatabase&>(db).getExternalFunctions();

    for (auto* profile : externalFuncs) {
        if (!profile) continue;

        InliningDecision decision;
        decision.functionName = profile->name;
        decision.codeSize = profile->codeSize;
        decision.callSites = profile->callSites.size();

        // Heuristic 1: Single-caller functions are good candidates
        if (decision.callSites == 1 && decision.codeSize < inlineThreshold_) {
            decision.shouldInline = true;
            decision.savingsEstimate = decision.codeSize - 3;  // Save func body, lose call overhead
            decision.reason = "Single caller, small function";
        }
        // Heuristic 2: Leaf functions in loops can benefit
        else if (profile->isLeaf && decision.codeSize < inlineThreshold_ / 2) {
            decision.shouldInline = true;
            decision.savingsEstimate = decision.codeSize * 0.5f;
            decision.reason = "Leaf function, significant savings";
        }
        // Heuristic 3: Very small functions (<10 bytes)
        else if (decision.codeSize < 10 && decision.callSites <= 3) {
            decision.shouldInline = true;
            decision.savingsEstimate = decision.codeSize - 2;
            decision.reason = "Tiny function, minimal overhead";
        }
        // Default: don't inline
        else {
            decision.shouldInline = false;
            decision.savingsEstimate = 0;
            decision.reason = "Code size or call frequency too high";
        }

        decisions.push_back(decision);
    }

    return decisions;
}

std::vector<SpecializationDecision> IPOAnalyzer::analyzeSpecialization(const GlobalFunctionDatabase& db) {
    std::vector<SpecializationDecision> decisions;

    auto candidates = db.getSpecializationCandidates();

    for (const auto& candidate : candidates) {
        SpecializationDecision decision;
        decision.baseFunctionName = candidate.baseFunctionName;
        decision.constantArgs = candidate.constantArgs;
        decision.estimatedSavings = candidate.estimatedSavings;

        // Apply ROI threshold
        if (candidate.roi >= roiThreshold_) {
            decision.isDecided = true;
            // Generate specialization name from constant arguments
            decision.specializationName = candidate.baseFunctionName + "_spec";
            for (const auto& arg : candidate.constantArgs) {
                decision.specializationName += "_" + std::to_string(arg.constantValue);
            }
        } else {
            decision.isDecided = false;
        }

        decisions.push_back(decision);
    }

    return decisions;
}

std::vector<DeadCodeDecision> IPOAnalyzer::analyzeDeadCode(const GlobalFunctionDatabase& db) {
    std::vector<DeadCodeDecision> decisions;

    auto deadFuncs = db.getDeadFunctions();

    for (const auto& funcName : deadFuncs) {
        auto* profile = const_cast<GlobalFunctionDatabase&>(db).getFunctionProfile(funcName);
        if (!profile) continue;

        DeadCodeDecision decision;
        decision.functionName = funcName;
        decision.isDeadCode = true;

        if (profile->isDeadCode) {
            decision.reason = "No external callers";
        } else if (!profile->hasExternalCaller) {
            decision.reason = "Not called from outside module";
        } else {
            decision.reason = "Unknown reason";
        }

        decisions.push_back(decision);
    }

    return decisions;
}

float IPOAnalyzer::calculateInliningBenefit(const GlobalFunctionProfile* profile) const {
    if (!profile) return 0.0f;

    // Benefit = (function size - call overhead) * number of inlinings
    // Call overhead is ~6 bytes (JSR + RTS)
    int functionSize = profile->codeSize;
    int callOverhead = 6;
    int callSites = profile->callSites.size();

    if (functionSize >= callOverhead) {
        return static_cast<float>((functionSize - callOverhead) * callSites);
    }
    return 0.0f;
}

int IPOAnalyzer::estimateCodeSizeImpact(const GlobalFunctionProfile* profile) const {
    if (!profile) return 0;

    // Conservative estimate: function size less call overhead savings
    int baseSize = profile->codeSize;

    // If inlined, we save the function definition but pay call overhead
    // Net savings = function size - (call overhead per call site)
    // Conservative: assume 6 bytes per call for JSR + RTS overhead
    return baseSize - std::min(6, baseSize / 2);
}

void IPOAnalyzer::printDecisions(const IPOAnalysisResult& result) const {
    std::cerr << "\n=== IPO Analysis Decisions ===\n\n";

    if (!result.specializations.empty()) {
        std::cerr << "Specialization Candidates: " << result.specializations.size() << "\n";
        for (const auto& spec : result.specializations) {
            if (spec.isDecided) {
                std::cerr << "  ✓ " << spec.specializationName << " (" << spec.estimatedSavings << " bytes)\n";
            } else {
                std::cerr << "  ✗ " << spec.baseFunctionName << " (ROI insufficient)\n";
            }
        }
        std::cerr << "\n";
    }

    if (!result.inlines.empty()) {
        std::cerr << "Inlining Decisions: " << result.inlines.size() << "\n";
        int inlineCount = 0;
        for (const auto& inl : result.inlines) {
            if (inl.shouldInline) {
                std::cerr << "  → " << inl.functionName << " (" << static_cast<int>(inl.savingsEstimate)
                         << " bytes, " << inl.reason << ")\n";
                inlineCount++;
            }
        }
        std::cerr << "Total functions to inline: " << inlineCount << "\n\n";
    }

    if (!result.deadCode.empty()) {
        std::cerr << "Dead Code Elimination: " << result.deadCode.size() << "\n";
        for (const auto& dead : result.deadCode) {
            std::cerr << "  ✗ " << dead.functionName << " (" << dead.reason << ")\n";
        }
        std::cerr << "\n";
    }

    std::cerr << "Estimated Total Savings: " << result.estimatedTotalSavings << " bytes\n";
    std::cerr << "\n";
}
