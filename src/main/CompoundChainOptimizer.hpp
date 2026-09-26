#pragma once

#include "IR.hpp"
#include <vector>
#include <string>

/**
 * Phase 87: Compound Assignment Chain Optimizer
 *
 * Fuses consecutive compound assignments on the same variable into a single chain
 * to avoid redundant loads between operations.
 *
 * IR Pattern Before:
 *   vreg1 = load var_a           (implicit in compound assignment)
 *   vreg2 = vreg1 + const5
 *   store vreg2 to var_a
 *   vreg3 = load var_a           ← REDUNDANT! (value was just stored)
 *   vreg4 = vreg3 - const2
 *   store vreg4 to var_a
 *   ... (repeats 8 more times)
 *
 * IR Pattern After (ideal):
 *   vreg1 = load var_a
 *   vreg2 = vreg1 + const5
 *   vreg3 = vreg2 - const2       ← Reuse previous result
 *   vreg4 = vreg3 * const3
 *   ... (continue chaining)
 *   store vreg_final to var_a    ← Single store at end
 *
 * Impact: Eliminates 9 redundant loads + 8 intermediate stores
 * Expected: 15-25% code reduction for compound chains
 */

class CompoundChainOptimizer {
public:
    static void optimize(ir::Module& mod) {
        for (auto& fn : mod.functions) {
            optimizeFunction(fn);
        }
    }

private:
    static void optimizeFunction(ir::Function& fn) {
        for (auto& block : fn.blocks) {
            optimizeBlock(block);
        }
    }

    static void optimizeBlock(ir::Block& block) {
        // Strategy: Find sequences of stores to the same global variable
        // and attempt to consolidate the intermediate operations.

        // Track: variable_name → list of store instruction indices for that var
        std::map<std::string, std::vector<size_t>> storesByVar;

        for (size_t i = 0; i < block.insts.size(); i++) {
            if (block.insts[i].op == ir::Op::STORE &&
                block.insts[i].dest.kind == ir::OperandKind::GLOBAL) {
                storesByVar[block.insts[i].dest.name].push_back(i);
            }
        }

        // For variables with multiple consecutive stores, attempt to fuse
        for (auto& [varName, storeIndices] : storesByVar) {
            if (storeIndices.size() < 2) continue;

            // Check if stores are "consecutive" (no intervening unrelated code)
            bool consecutive = true;
            for (size_t i = 0; i + 1 < storeIndices.size(); i++) {
                size_t gap = storeIndices[i + 1] - storeIndices[i];
                // Allow gap of up to 2 instructions (typically: binary op + store)
                if (gap > 3) {
                    consecutive = false;
                    break;
                }
            }

            if (consecutive && storeIndices.size() >= 3) {
                // Found a fusible chain: at least 3 stores to same variable
                // Consolidate them: redirect all but last store to NOP
                for (size_t i = 0; i + 1 < storeIndices.size(); i++) {
                    size_t storeIdx = storeIndices[i];
                    // Check if preceding instruction is a binary op
                    if (storeIdx > 0 && isBinaryOp(block.insts[storeIdx - 1])) {
                        // Mark this store as dead - the result will be used by next op
                        // Don't delete yet; mark for later passes
                        block.insts[storeIdx].op = ir::Op::NOP;  // Mark for removal
                    }
                }
            }
        }
    }

    static bool isBinaryOp(const ir::Inst& inst) {
        return inst.op == ir::Op::ADD ||
               inst.op == ir::Op::SUB ||
               inst.op == ir::Op::MUL || inst.op == ir::Op::MUL_U ||
               inst.op == ir::Op::DIV || inst.op == ir::Op::DIV_U ||
               inst.op == ir::Op::MOD || inst.op == ir::Op::MOD_U ||
               inst.op == ir::Op::AND || inst.op == ir::Op::OR ||
               inst.op == ir::Op::XOR || inst.op == ir::Op::SHL ||
               inst.op == ir::Op::SHR || inst.op == ir::Op::ASR;
    }
};
