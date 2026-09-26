#pragma once

#include "IR.hpp"
#include "IPOAnalyzer.hpp"
#include <vector>
#include <string>
#include <memory>
#include <map>

// Phase 91.3.6: IR-Level Specialization Code Generation
//
// Generates specialized function variants at the IR level by:
// 1. Cloning IR functions for each specialization candidate
// 2. Replacing parameter references with constant values
// 3. Allowing constant folding to optimize the specialized code
//
// This approach is pragmatic and production-ready:
// - Works with existing IR infrastructure
// - No AST manipulation required
// - Leverages proven constant folding pipeline
// - Results in equivalent code reduction

class IRSpecializationGenerator {
public:
    IRSpecializationGenerator();

    // Generate specialized IR function variants
    void generateSpecializations(ir::Module& module,
                                const std::vector<SpecializationDecision>& decisions);

    // Clone an IR function with parameter specialization
    ir::Function cloneAndSpecializeFunction(
        const ir::Function& originalFunc,
        const std::string& newName,
        const std::vector<long>& constantArgs);

private:
    // Helper to deep-copy an IR block
    ir::Block cloneBlock(const ir::Block& originalBlock,
                         ir::Function& targetFunc,
                         const std::map<uint32_t, ir::Operand>& paramSubstitutions);

    // Helper to deep-copy an IR instruction
    ir::Inst cloneInstruction(const ir::Inst& originalInst,
                              const std::map<uint32_t, ir::Operand>& paramSubstitutions,
                              std::map<uint32_t, uint32_t>& vregMapping);

    // Find function by name in module
    ir::Function* findFunction(ir::Module& module, const std::string& name);

    // Track specialized functions generated
    std::vector<std::string> generatedSpecializations_;

    // Map original vregs to new vregs when cloning
    std::map<uint32_t, uint32_t> vregMap_;
};
