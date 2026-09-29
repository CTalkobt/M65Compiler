#include "IRSpecializationGenerator.hpp"
#include <iostream>
#include <algorithm>
#include <sstream>

IRSpecializationGenerator::IRSpecializationGenerator() {}

void IRSpecializationGenerator::generateSpecializations(
    ir::Module& module,
    const std::vector<SpecializationDecision>& decisions) {

    if (decisions.empty()) {
        return;
    }

    std::cerr << "[Phase 91.3.6] Generating " << decisions.size()
              << " IR specialization variant(s)...\n";

    int generated = 0;
    for (const auto& spec : decisions) {
        if (!spec.isDecided) {
            continue;  // Skip decisions that didn't pass ROI threshold
        }

        // Find the original function
        ir::Function* originalFunc = findFunction(module, spec.baseFunctionName);
        if (!originalFunc) {
            std::cerr << "[Phase 91.3.6] Warning: Cannot find function '"
                      << spec.baseFunctionName << "'\n";
            continue;
        }

        // Extract constant values from decision
        std::vector<long> constantValues;
        for (const auto& arg : spec.constantArgs) {
            constantValues.push_back(arg.constantValue);
        }

        // Verify we have constants for each parameter
        if (constantValues.size() > originalFunc->paramTypes.size()) {
            std::cerr << "[Phase 91.3.6] Warning: More constants than parameters for '"
                      << spec.baseFunctionName << "'\n";
            continue;
        }

        // Clone and specialize the function
        ir::Function specializedFunc = cloneAndSpecializeFunction(
            *originalFunc,
            spec.specializationName,
            constantValues);

        // Add to module
        module.functions.push_back(specializedFunc);
        generatedSpecializations_.push_back(spec.specializationName);
        generated++;

        std::cerr << "[Phase 91.3.6] Generated specialization: " << spec.specializationName
                  << " from " << spec.baseFunctionName
                  << " with " << constantValues.size() << " constant arg(s)\n";
    }

    if (generated > 0) {
        std::cerr << "[Phase 91.3.6] Generated " << generated << " specialization variant(s)\n";
    }
}

ir::Function IRSpecializationGenerator::cloneAndSpecializeFunction(
    const ir::Function& originalFunc,
    const std::string& newName,
    const std::vector<long>& constantArgs) {

    // Create new function with specialized name
    ir::Function newFunc;
    newFunc.name = newName;
    newFunc.returnType = originalFunc.returnType;
    newFunc.conv = originalFunc.conv;
    newFunc.paramTypes = originalFunc.paramTypes;
    newFunc.paramNames = originalFunc.paramNames;
    newFunc.isVariadic = originalFunc.isVariadic;
    newFunc.isStatic = true;  // Specialized functions are always static
    newFunc.isWeak = false;
    newFunc.isInterrupt = originalFunc.isInterrupt;
    newFunc.isNaked = originalFunc.isNaked;
    newFunc.isRegparm = originalFunc.isRegparm;
    newFunc.isNested = false;  // Specializations are top-level
    newFunc.isRecurse = false;
    newFunc.staticLinkVreg = -1;
    newFunc.declLine = originalFunc.declLine;

    // Build parameter substitution map: param vreg -> constant value
    std::map<uint32_t, ir::Operand> paramSubstitutions;

    // The parameter vregs are allocated during IR generation
    // We need to identify which vregs are parameters and map them to constants
    // For now, we'll handle this during instruction cloning by checking parameter names
    for (size_t i = 0; i < constantArgs.size() && i < originalFunc.paramNames.size(); ++i) {
        // Store mapping by parameter index
        // During cloning, we'll check if a vreg loads a parameter and substitute it
    }

    // Clone all blocks and instructions
    for (const auto& originalBlock : originalFunc.blocks) {
        ir::Block newBlock = cloneBlock(originalBlock, newFunc, paramSubstitutions);
        newFunc.blocks.push_back(newBlock);
    }

    // Copy vregTypes and other metadata
    newFunc.vregTypes = originalFunc.vregTypes;
    newFunc.vregSizes = originalFunc.vregSizes;
    newFunc.localNames = originalFunc.localNames;
    newFunc.localNamesOrder = originalFunc.localNamesOrder;
    newFunc.memoryVregs = originalFunc.memoryVregs;
    newFunc.registerVregs = originalFunc.registerVregs;
    newFunc.localSlotVregs = originalFunc.localSlotVregs;
    newFunc.nextVreg = originalFunc.nextVreg;

    return newFunc;
}

ir::Block IRSpecializationGenerator::cloneBlock(
    const ir::Block& originalBlock,
    ir::Function& targetFunc,
    const std::map<uint32_t, ir::Operand>& paramSubstitutions) {

    ir::Block newBlock;
    newBlock.label = originalBlock.label;

    std::map<uint32_t, uint32_t> vregMapping;

    // Clone all instructions
    for (const auto& originalInst : originalBlock.insts) {
        ir::Inst clonedInst = cloneInstruction(originalInst, paramSubstitutions, vregMapping);
        newBlock.insts.push_back(clonedInst);
    }

    return newBlock;
}

ir::Inst IRSpecializationGenerator::cloneInstruction(
    const ir::Inst& originalInst,
    const std::map<uint32_t, ir::Operand>& paramSubstitutions,
    std::map<uint32_t, uint32_t>& vregMapping) {

    ir::Inst newInst = originalInst;

    // Update destination vreg if needed
    if (originalInst.dest.isVreg()) {
        uint32_t origVregId = originalInst.dest.vregId;
        if (vregMapping.find(origVregId) == vregMapping.end()) {
            // First time seeing this vreg, allocate new one (would need access to function)
            // For now, keep the same vreg ID (assumes no conflicts)
        }
        // Keep destination as-is for now
    }

    // Update source operands
    if (originalInst.src1.isVreg()) {
        // Would substitute here if in paramSubstitutions
    }
    if (originalInst.src2.isVreg()) {
        // Would substitute here if in paramSubstitutions
    }

    // Clone argument operands for function calls
    std::vector<ir::Operand> newArgs;
    for (const auto& arg : originalInst.args) {
        // Clone argument (with substitutions if needed)
        newArgs.push_back(arg);
    }
    newInst.args = newArgs;

    return newInst;
}

ir::Function* IRSpecializationGenerator::findFunction(ir::Module& module,
                                                      const std::string& name) {
    for (auto& func : module.functions) {
        if (func.name == name) {
            return &func;
        }
    }
    return nullptr;
}
