#pragma once

#include "O45Linker.hpp"
#include "IPOAnalyzer.hpp"
#include <vector>
#include <string>
#include <map>
#include <memory>

// Phase 91.4: Cross-Module Optimization Coordinator
//
// Coordinates linker-level cross-module optimization by:
// 1. Collecting specialization opportunities across modules
// 2. Generating call routing decisions
// 3. Creating dispatcher stubs for multi-specialization
// 4. Integrating specialized variants into final binary
//
// This phase bridges the compiler's Phase 91.3.6 (IR specialization)
// and the linker's cross-module analysis infrastructure.

class CrossModuleOptimizer {
public:
    CrossModuleOptimizer();

    // Analyze cross-module specialization opportunities
    // Called after linking with all objects
    void analyzeSpecializations(const O45Linker& linker);

    // Generate call routing decisions for cross-module calls
    void generateCallRouting(const O45Linker& linker);

    // Create dispatcher stubs for multi-specialization cases
    void createDispatchers(O45Linker& linker);

    // Get cross-module specialization report
    std::string getSpecializationReport() const;

    // Get call routing report
    std::string getCallRoutingReport() const;

    // Get optimization metrics
    struct OptimizationMetrics {
        int totalFunctions = 0;
        int specializationCandidates = 0;
        int crossModuleOpportunities = 0;
        int dispatchersGenerated = 0;
        int estimatedCodeSavings = 0;
        float compressionRatio = 0.0f;
    };

    OptimizationMetrics getMetrics() const { return metrics_; }

private:
    // Analyze parameter patterns across all call sites
    void analyzeParameterPatterns(const O45Linker& linker);

    // Identify cross-module specialization opportunities
    void identifyOpportunities(const O45Linker& linker);

    // Generate routing decisions for constant-matching calls
    void generateRoutingDecisions(const O45Linker& linker);

    // Track specialization candidates
    struct SpecializationCandidate {
        std::string functionName;
        std::vector<long> constantPattern;
        int callSites = 0;
        int estimatedSavings = 0;
    };

    // Track call routing decisions
    struct CallRoutingDecision {
        std::string callSite;
        std::string targetFunction;
        std::vector<long> argumentPattern;
        std::string routingTarget;  // Original or specialized variant
    };

    // State tracking
    std::vector<SpecializationCandidate> candidates_;
    std::vector<CallRoutingDecision> routingDecisions_;
    OptimizationMetrics metrics_;

    // Cross-module analysis results
    std::map<std::string, std::vector<long>> parameterPatterns_;
    std::map<std::string, int> crossModuleCallCounts_;
};
