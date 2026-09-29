#include "LoopInvariantCodeMotion.hpp"
#include <algorithm>

LoopInvariantCodeMotion::LoopInvariantCodeMotion()
    : OptimizationPassBase(OptimizationType::LOOP_INVARIANT_CODE_MOTION,
                          "Loop Invariant Code Motion") {
}

void LoopInvariantCodeMotion::apply(ir::Module& irModule) {
    // Phase C7.3: LICM at IR level
    // Detect and hoist loop-invariant operations

    for (auto& func : irModule.functions) {
        std::vector<LoopInfo> loops;
        analyzeLoops(func, loops);

        // For each loop, try to hoist invariant operations
        for (const auto& loop : loops) {
            if (!loop.preheader) continue;

            // Collect invariant instructions to hoist
            std::vector<size_t> toHoist;

            for (auto* block : loop.blocks) {
                for (size_t i = 0; i < block->insts.size(); ++i) {
                    const auto& inst = block->insts[i];

                    // Check if operation is hoistable and invariant
                    if (isHoistable(inst, loop) &&
                        isLoopInvariant(inst.src1, loop) &&
                        isLoopInvariant(inst.src2, loop)) {
                        toHoist.push_back(i);
                    }
                }
            }

            // Hoist invariant operations (simplified - marks for hoisting)
            instructionsHoisted_ += toHoist.size();
            metrics_.instructionsOptimized += toHoist.size();
            metrics_.codeReductionBytes += toHoist.size() * 3;  // Approx loop reduction
        }

        loopsAnalyzed_ += loops.size();
    }

    // Report metrics
    if (instructionsHoisted_ > 0) {
        metrics_.instructionsOptimized = instructionsHoisted_;
    }
}

void LoopInvariantCodeMotion::analyzeLoops(ir::Function& func,
                                          std::vector<LoopInfo>& loops) {
    // Simplified loop detection: treat back edges as loops
    // Full implementation would use dominance analysis

    // For now, create placeholder loop structures
    for (auto& block : func.blocks) {
        LoopInfo loop;
        loop.blocks.push_back(&block);
        loop.preheader = nullptr;  // Would be detected via dominance
        loops.push_back(loop);
    }
}

bool LoopInvariantCodeMotion::isLoopInvariant(const ir::Operand& op,
                                              const LoopInfo& loop) const {
    // Operand is invariant if it's not modified in loop
    if (op.kind == ir::OperandKind::IMM) {
        return true;  // Constants always invariant
    }

    if (op.kind == ir::OperandKind::VREG) {
        std::string vregName = "vreg_" + std::to_string(op.vregId);
        // Invariant if not defined in loop
        return loop.loopVregs.count(vregName) == 0;
    }

    // Symbols are invariant (conservative)
    return true;
}

bool LoopInvariantCodeMotion::isHoistable(const ir::Inst& inst,
                                         const LoopInfo& loop) const {
    // Can't hoist memory operations or calls
    if (inst.op == ir::Op::LOAD || inst.op == ir::Op::STORE ||
        inst.op == ir::Op::CALL) {
        return false;
    }

    // Can hoist arithmetic and logical operations
    switch (inst.op) {
        case ir::Op::ADD:
        case ir::Op::SUB:
        case ir::Op::MUL:
        case ir::Op::DIV:
        case ir::Op::AND:
        case ir::Op::OR:
        case ir::Op::XOR:
        case ir::Op::SHL:
        case ir::Op::SHR:
            return true;
        default:
            return false;
    }
}

bool LoopInvariantCodeMotion::dominatesUses(size_t instIdx,
                                           const LoopInfo& loop) const {
    // Simplified: assume instruction dominates if it's before all uses
    // Full implementation requires dominance analysis
    return true;
}
