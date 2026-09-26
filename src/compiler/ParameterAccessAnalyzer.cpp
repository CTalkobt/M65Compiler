#include "ParameterAccessAnalyzer.h"
#include "../include/IR.hpp"
#include <algorithm>
#include <iostream>

namespace compiler {

std::map<int, ParameterAccessInfo> ParameterAccessAnalyzer::analyze(const ir::Function& fn) {
    std::map<int, ParameterAccessInfo> accessMap;

    // Initialize entries for each parameter
    for (size_t i = 0; i < fn.paramNames.size(); i++) {
        ParameterAccessInfo info;
        info.paramID = (int)i;
        accessMap[(int)i] = info;
    }

    // If no parameters, nothing to analyze
    if (fn.paramNames.empty()) {
        return accessMap;
    }

    // Traverse IR to count parameter accesses
    uint32_t instructionOffset = 0;
    for (const auto& block : fn.blocks) {
        analyzeBlock(*block, accessMap, instructionOffset, fn.paramNames);
    }

    return accessMap;
}

void ParameterAccessAnalyzer::analyzeBlock(
    const ir::Block& block,
    std::map<int, ParameterAccessInfo>& accessMap,
    uint32_t& instructionOffset,
    const std::vector<std::string>& paramNames) {

    for (const auto& instr : block.instructions) {
        // Check if this instruction accesses a parameter
        int paramID = extractParameterAccess(*instr, paramNames);

        if (paramID >= 0 && paramID < (int)paramNames.size()) {
            auto& info = accessMap[paramID];
            info.accessCount++;
            info.instructionOffsets.push_back(instructionOffset);

            // Record access size (estimate based on parameter type)
            // This will be refined when we have actual codegen information
            uint8_t size = 1;  // Default to 1 byte, will be updated in codegen
            info.accessSizes.push_back(size);
        }

        // Increment offset (placeholder - actual offset computed during codegen)
        // Each instruction is roughly 2-3 bytes, we'll refine this later
        instructionOffset += 3;  // Conservative estimate
    }
}

int ParameterAccessAnalyzer::extractParameterAccess(
    const ir::Instruction& instr,
    const std::vector<std::string>& paramNames) {

    // Look for patterns that indicate parameter access
    // In the IR, parameters appear in various forms:
    // 1. VariableLoad from parameter variable
    // 2. ParameterRef nodes
    // 3. Loads from parameter offsets

    // Check all operands for parameter references
    for (size_t i = 0; i < paramNames.size(); i++) {
        const auto& paramName = paramNames[i];

        // Check instruction description for parameter name
        // This is a simplified heuristic - real implementation would need
        // deeper IR inspection
        std::string instrDesc;  // Would be set from actual instruction

        // For now, return -1 (not found)
        // This will be expanded in actual implementation with proper IR visiting
    }

    return -1;  // No parameter access found
}

}  // namespace compiler
