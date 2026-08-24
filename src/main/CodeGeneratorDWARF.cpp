#include "CodeGenerator.hpp"
#include "AST.hpp"

// Phase 113: DWARF emission helpers for CodeGenerator

void CodeGenerator::emitFunctionDIE(FunctionDeclaration& node, uint64_t startAddr) {
    // Get reference to IRBuilder to access DebugInfoBuilder
    // (In practice, this would be passed from main compilation pipeline)

    // For now, we'll create the DIE structure but won't serialize it
    // until O45Writer integration in Phase 113.5 Part 3

    currentFunctionStartAddress_ = startAddr;

    // Note: Full DIE emission requires access to DebugInfoBuilder from IRBuilder
    // This will be wired through the compilation pipeline in Part 3

    // Emit line entry for function definition
    lineNumberBuilder_.emitLineEntry(startAddr, node.line, node.column);
}

void CodeGenerator::emitLocalVariableDIE(const std::string& varName, uint64_t address,
                                         const VarInfo& varInfo, uint32_t frameOffset) {
    // Emit line entry for variable at its declaration
    // The actual DIE creation happens during IRBuilder phase
    // This tracks the address for line number mapping
}
