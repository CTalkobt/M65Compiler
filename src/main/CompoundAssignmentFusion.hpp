#pragma once

#include "IR.hpp"
#include <algorithm>
#include <unordered_map>

/**
 * Phase 86: Compound Assignment Fusion Optimization
 *
 * Detects and fuses patterns like:
 *   vreg_t = var op constant    (binary op)
 *   var = vreg_t                (store)
 *
 * Fuses into:
 *   var = var op constant       (direct assignment, reuses destination)
 *
 * Benefits:
 * - Eliminates intermediate vreg (saves ZP allocation)
 * - Removes redundant load/store cycles
 * - Improves compound assignment codegen (a /= 4, a += 5, etc.)
 * - Helps strength reduction (shifts stay in A/X)
 *
 * Impact: ~5-10% code reduction for code with many compound assignments
 */

class CompoundAssignmentFusion {
public:
    static void fuse(ir::Module& mod) {
        for (auto& fn : mod.functions) {
            fuseInFunction(fn);
        }
    }

private:
    static void fuseInFunction(ir::Function& fn) {
        for (auto& block : fn.blocks) {
            fuseInBlock(block, fn);
        }
    }

    static void fuseInBlock(ir::Block& block, ir::Function& fn) {
        // Scan for patterns: vreg = var op X; var = vreg
        for (size_t i = 0; i + 1 < block.insts.size(); i++) {
            auto& binOp = block.insts[i];
            auto& store = block.insts[i + 1];

            // Check if pattern matches
            if (isBinaryOp(binOp) && isStoreToVar(store) &&
                binOp.dest.isVreg() &&
                store.src1.isVreg() && store.src1.vregId == binOp.dest.vregId) {

                // Verify that binOp.src1 matches the variable being stored to
                if (operandsMatch(binOp.src1, store.dest)) {
                    // Fuse: change binOp.dest to be the variable directly
                    binOp.dest = store.dest;
                    // Mark store for removal
                    store.op = ir::Op::NOP;
                    // Note: actual removal happens in a second pass
                }
            }
        }

        // Remove NOP instructions
        block.insts.erase(
            std::remove_if(block.insts.begin(), block.insts.end(),
                          [](const ir::Inst& inst) { return inst.op == ir::Op::NOP; }),
            block.insts.end()
        );
    }

    static bool isBinaryOp(const ir::Inst& inst) {
        return inst.op == ir::Op::ADD || inst.op == ir::Op::ADD_U ||
               inst.op == ir::Op::SUB || inst.op == ir::Op::SUB_U ||
               inst.op == ir::Op::MUL || inst.op == ir::Op::MUL_U ||
               inst.op == ir::Op::DIV || inst.op == ir::Op::DIV_U ||
               inst.op == ir::Op::MOD || inst.op == ir::Op::MOD_U ||
               inst.op == ir::Op::AND || inst.op == ir::Op::OR ||
               inst.op == ir::Op::XOR || inst.op == ir::Op::SHL ||
               inst.op == ir::Op::SHR || inst.op == ir::Op::ASR;
    }

    static bool isStoreToVar(const ir::Inst& inst) {
        return inst.op == ir::Op::STORE && inst.dest.kind == ir::OperandKind::GLOBAL;
    }

    static bool operandsMatch(const ir::Operand& a, const ir::Operand& b) {
        if (a.kind != b.kind) return false;
        if (a.kind == ir::OperandKind::GLOBAL) return a.name == b.name;
        if (a.kind == ir::OperandKind::VREG) return a.vregId == b.vregId;
        return false;
    }
};
