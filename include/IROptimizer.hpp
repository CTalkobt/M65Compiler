#pragma once

#include "IR.hpp"
#include <map>
#include <set>
#include <vector>
#include <string>

namespace ir {

class IROptimizer {
public:
    explicit IROptimizer(int verboseLevel = 0) : verboseLevel_(verboseLevel) {}

    // Runs all optimization passes on the module
    void optimize(Module& mod);

private:
    int verboseLevel_ = 0;

    // Individual optimization passes on functions
    bool foldConstants(Function& fn);
    bool eliminateDeadCode(Function& fn);
    bool simplifyControlFlow(Function& fn);
    bool reduceStrength(Function& fn);
    bool eliminateUnreachableBlocks(Function& fn);
    bool propagateConstants(Function& fn);
    bool phiSimplification(Function& fn);
    bool globalValueNumber(Function& fn);
    bool hoistLoopInvariants(Function& fn);
    bool addressComputationFold(Function& fn);
    bool storeLoadForwarding(Function& fn);
    bool completeLoopHoisting(Function& fn);
    bool enhancedAliasAnalysis(Function& fn);
    bool inliningAwareOpt(Function& fn);
    bool crossFunctionOpt(Function& fn);
    bool speculativeHoisting(Function& fn);
    bool instructionPatternOpt(Function& fn);
    bool aggressiveDeadBlockRemoval(Function& fn);
    bool commonSubexprElim(Function& fn);
    bool propagateCopies(Function& fn);
    bool optimizeTailCalls(Function& fn);

    // Helper functions for constant folding
    bool foldInstruction(Inst& inst, const std::map<uint32_t, Operand>& constMap);
    Operand evaluateBinaryOp(Op op, Type type, int64_t v1, int64_t v2);
    Operand evaluateUnaryOp(Op op, Type type, int64_t val);

    // Helper functions for loop hoisting (Phase 1 - simple hoisting)
    bool isLoopInvariant(const Inst& inst, const std::set<uint32_t>& loopModifiedVregs,
                         const std::set<uint32_t>& phiDefVregs);
    bool isPureComputation(Op op);
    bool canHoistToPreheader(const Inst& inst, const std::set<uint32_t>& loopModifiedVregs,
                             const std::set<uint32_t>& phiDefVregs);

    // Helper functions for loop hoisting (Phase 2 - PHI duplication)
    void findDuplicatablePHIsAndCreate(Block& headerBlock, Block& preheaderBlock, Function& fn,
                                       std::map<uint32_t, uint32_t>& vregMapping);
    std::string findBlockPredecessor(const Block& block, const Function& fn);
    Operand remapOperand(const Operand& op, const std::map<uint32_t, uint32_t>& vregMapping);
    void remapInstructionOperands(Inst& inst, const std::map<uint32_t, uint32_t>& vregMapping);
    bool isLoopInvariantWithPHIMapping(const Inst& inst, const std::set<uint32_t>& loopModifiedVregs,
                                       const std::map<uint32_t, uint32_t>& vregMapping);
    bool canHoistToPreheaderWithPHI(const Inst& inst, const std::set<uint32_t>& loopModifiedVregs,
                                    const std::map<uint32_t, uint32_t>& vregMapping);
};

} // namespace ir
