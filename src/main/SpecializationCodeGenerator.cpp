#include "SpecializationCodeGenerator.hpp"
#include <algorithm>
#include <iostream>

SpecializationCodeGenerator::SpecializationCodeGenerator() {}

std::vector<SpecializedFunctionPlan> SpecializationCodeGenerator::analyzeSpecializations(
    const std::vector<SpecializationDecision>& decisions) {

    std::vector<SpecializedFunctionPlan> plans;

    for (const auto& decision : decisions) {
        if (!decision.isDecided) {
            continue;  // Skip specializations that didn't pass ROI threshold
        }

        // Extract constant values from decision
        std::vector<long> constantValues;
        for (const auto& arg : decision.constantArgs) {
            constantValues.push_back(arg.constantValue);
        }

        SpecializedFunctionPlan plan;
        plan.baseFunctionName = decision.baseFunctionName;
        plan.specializationName = decision.specializationName;
        plan.constantArgs = constantValues;
        plan.estimatedSavings = decision.estimatedSavings;

        plans.push_back(plan);

        std::cerr << "[Phase 91.3.4] Specialization plan: "
                  << decision.specializationName << " from " << decision.baseFunctionName
                  << " (est. savings: " << decision.estimatedSavings << "B)\n";
    }

    return plans;
}

SpecializationCodeGenerator::SpecializationAnalysis SpecializationCodeGenerator::analyze(
    const std::vector<SpecializationDecision>& decisions) {

    SpecializationAnalysis result;
    result.totalCandidates = decisions.size();

    // Analyze each decision
    for (const auto& decision : decisions) {
        // Extract constant values from the decision
        std::vector<long> constantValues;
        for (const auto& arg : decision.constantArgs) {
            constantValues.push_back(arg.constantValue);
        }

        result.plans.push_back({
            decision.baseFunctionName,
            decision.specializationName,
            constantValues,
            decision.estimatedSavings
        });

        // Count viable candidates (those that pass ROI threshold)
        if (decision.isDecided) {
            result.viableCandidates++;
            result.estimatedTotalSavings += decision.estimatedSavings;
        }
    }

    return result;
}

int SpecializationCodeGenerator::estimateImplementationEffort(
    const SpecializationDecision& decision) {

    // Estimate effort based on number of constant arguments and code size
    // Base effort: 20 bytes (function header/prologue)
    // Per-constant: 5 bytes (parameter elimination)
    int effort = 20 + (decision.constantArgs.size() * 5);

    return effort;
}

float SpecializationCodeGenerator::calculateROI(
    int estimatedSavings,
    int implementationEffort) {

    if (implementationEffort <= 0) return 0.0f;
    return static_cast<float>(estimatedSavings) / static_cast<float>(implementationEffort);
}
