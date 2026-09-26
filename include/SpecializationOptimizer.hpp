#pragma once

#include "IPOAnalyzer.hpp"
#include "IR.hpp"
#include <vector>
#include <string>
#include <memory>

// Phase 91.3.5: Specialization Optimizer
// Implements specialization through aggressive inlining + constant propagation
//
// Key insight: Specialization achieves code reduction by constant-folding
// inlined code. Rather than generating separate function variants, we:
// 1. Mark specialization candidates for aggressive inlining
// 2. The constant folding optimizer then optimizes the inlined code
// 3. Result: Equivalent code reduction through existing infrastructure

class SpecializationOptimizer {
public:
    SpecializationOptimizer();

    // Optimize IR module for specialization
    // Marks functions as inline-preferred and adds constant propagation hints
    void optimize(ir::Module& module,
                  const std::vector<SpecializationDecision>& specializations);

    // Apply specialization-driven inlining
    // Forces inlining of specialization candidates to trigger constant folding
    void applySpecializationInlining(ir::Module& module,
                                     const std::vector<SpecializationDecision>& specializations);

    // Mark function for aggressive inlining with specialization hints
    void markForSpecializationInlining(ir::Function& func,
                                       const std::vector<long>& constantArgs);

private:
    // Helper to find function by name
    ir::Function* findFunction(ir::Module& module, const std::string& name);

    // Helper to add constant propagation hints to function calls
    void addConstantPropagationHint(ir::Block& block,
                                   const std::string& callTarget,
                                   const std::vector<long>& constants);

    // Track which functions have specialization hints applied
    std::vector<std::string> specializedFunctions_;
};
