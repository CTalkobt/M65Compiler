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
    bool aggressiveDeadBlockRemoval(Function& fn);
    bool commonSubexprElim(Function& fn);
    bool propagateCopies(Function& fn);

    // Helper functions for constant folding
    bool foldInstruction(Inst& inst, const std::map<uint32_t, Operand>& constMap);
    Operand evaluateBinaryOp(Op op, Type type, int64_t v1, int64_t v2);
    Operand evaluateUnaryOp(Op op, Type type, int64_t val);
};

} // namespace ir
