#include "CrossModuleOptimizer.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>

CrossModuleOptimizer::CrossModuleOptimizer() {}

void CrossModuleOptimizer::analyzeSpecializations(const O45Linker& linker) {
    // Phase 91.4: Analyze cross-module specialization opportunities
    //
    // This phase leverages the linker's existing specialization analysis
    // infrastructure to identify opportunities across module boundaries.

    std::cerr << "[Phase 91.4] Analyzing cross-module specialization opportunities...\n";

    // Get linker's analysis results
    const auto& funcAttrs = linker.getFuncAttrs();
    const auto& callGraph = linker.getCallGraph();
    const auto& specializedParams = linker.getSpecializedParams();

    std::cerr << "[Phase 91.4] Linker analysis: "
              << funcAttrs.size() << " functions, "
              << callGraph.size() << " call graph edges\n";

    // Analyze parameter specialization patterns
    analyzeParameterPatterns(linker);

    // Identify cross-module opportunities
    identifyOpportunities(linker);

    int totalCandidates = candidates_.size();
    std::cerr << "[Phase 91.4] Found " << totalCandidates
              << " potential specialization candidate(s)\n";

    // Update metrics
    metrics_.totalFunctions = funcAttrs.size();
    metrics_.specializationCandidates = totalCandidates;
    metrics_.crossModuleOpportunities = crossModuleCallCounts_.size();
}

void CrossModuleOptimizer::generateCallRouting(const O45Linker& linker) {
    // Phase 91.4: Generate call routing decisions for cross-module calls
    //
    // This determines which calls should be routed to specialized versions
    // and how the routing should be performed (direct call vs dispatcher).

    std::cerr << "[Phase 91.4] Generating cross-module call routing decisions...\n";

    generateRoutingDecisions(linker);

    std::cerr << "[Phase 91.4] Generated " << routingDecisions_.size()
              << " call routing decision(s)\n";
}

void CrossModuleOptimizer::createDispatchers(O45Linker& linker) {
    // Phase 91.4: Create dispatcher stubs for multi-specialization cases
    //
    // For functions with multiple specialization patterns, generate dispatcher
    // stubs that route to the appropriate specialized variant based on arguments.

    std::cerr << "[Phase 91.4] Creating dispatcher stubs for multi-specialization...\n";

    // Count functions with multiple specialization patterns
    std::map<std::string, int> specializationCounts;
    for (const auto& candidate : candidates_) {
        specializationCounts[candidate.functionName]++;
    }

    int dispatchersNeeded = 0;
    for (const auto& [func, count] : specializationCounts) {
        if (count > 1) {
            dispatchersNeeded++;
        }
    }

    if (dispatchersNeeded > 0) {
        std::cerr << "[Phase 91.4] Dispatcher stubs needed for " << dispatchersNeeded
                  << " function(s) with multiple specialization patterns\n";

        // Note: Full dispatcher generation delegated to linker phase 56
        // We just coordinate the decision here
        metrics_.dispatchersGenerated = dispatchersNeeded;
    }
}

std::string CrossModuleOptimizer::getSpecializationReport() const {
    std::ostringstream oss;
    oss << "Cross-Module Specialization Analysis Report\n"
        << "===========================================\n\n"
        << "Specialization Candidates: " << candidates_.size() << "\n";

    for (const auto& candidate : candidates_) {
        oss << "  " << candidate.functionName << ":\n"
            << "    Pattern: [";
        for (size_t i = 0; i < candidate.constantPattern.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << candidate.constantPattern[i];
        }
        oss << "]\n"
            << "    Call sites: " << candidate.callSites << "\n"
            << "    Est. savings: " << candidate.estimatedSavings << " bytes\n";
    }

    return oss.str();
}

std::string CrossModuleOptimizer::getCallRoutingReport() const {
    std::ostringstream oss;
    oss << "Cross-Module Call Routing Report\n"
        << "================================\n\n"
        << "Routing Decisions: " << routingDecisions_.size() << "\n";

    for (const auto& decision : routingDecisions_) {
        oss << "  Call site: " << decision.callSite << "\n"
            << "    Target: " << decision.targetFunction << "\n"
            << "    Routed to: " << decision.routingTarget << "\n";
    }

    return oss.str();
}

void CrossModuleOptimizer::analyzeParameterPatterns(const O45Linker& linker) {
    // Analyze constant parameter patterns across all call sites
    const auto& specializedParams = linker.getSpecializedParams();

    for (const auto& [funcName, paramMap] : specializedParams) {
        std::vector<long> pattern;
        for (const auto& [paramIdx, param] : paramMap) {
            if (param.isConstant) {
                pattern.push_back(param.value);
            }
        }

        if (!pattern.empty()) {
            parameterPatterns_[funcName] = pattern;
        }
    }

    std::cerr << "[Phase 91.4] Analyzed " << parameterPatterns_.size()
              << " functions with constant parameters\n";
}

void CrossModuleOptimizer::identifyOpportunities(const O45Linker& linker) {
    // Identify cross-module specialization opportunities
    // A function is a candidate if it has constant parameters in multiple call sites

    const auto& callGraph = linker.getCallGraph();

    for (const auto& [caller, callees] : callGraph) {
        for (const auto& callee : callees) {
            // Check if callee has constant parameters
            auto it = parameterPatterns_.find(callee);
            if (it != parameterPatterns_.end()) {
                // Found a cross-module specialization opportunity
                SpecializationCandidate candidate;
                candidate.functionName = callee;
                candidate.constantPattern = it->second;
                candidate.callSites++;
                candidate.estimatedSavings = 10 * candidate.constantPattern.size();

                candidates_.push_back(candidate);
                crossModuleCallCounts_[callee]++;
            }
        }
    }
}

void CrossModuleOptimizer::generateRoutingDecisions(const O45Linker& linker) {
    // Generate routing decisions based on specialization candidates
    const auto& callGraph = linker.getCallGraph();

    for (const auto& candidate : candidates_) {
        // For each specialization candidate, determine if calls should be routed
        auto it = callGraph.find(candidate.functionName);
        if (it != callGraph.end()) {
            // Generate routing decision for this function
            CallRoutingDecision decision;
            decision.targetFunction = candidate.functionName;
            decision.argumentPattern = candidate.constantPattern;

            // Routing target would be the specialized variant name
            // (e.g., "_compute" → "_compute_spec_2_3" for pattern [2,3])
            std::string routingTarget = candidate.functionName + "_spec";
            for (const auto& val : candidate.constantPattern) {
                routingTarget += "_" + std::to_string(val);
            }
            decision.routingTarget = routingTarget;

            routingDecisions_.push_back(decision);
        }
    }
}
