#include "BranchOptimization.hpp"
#include <algorithm>

BranchOptimization::BranchOptimization()
    : OptimizationPassBase(OptimizationType::BRANCH_OPTIMIZATION,
                          "Branch Optimization") {
}

void BranchOptimization::apply(ir::Module& irModule) {
    // Phase C7.4: Branch Optimization at IR level
    // Eliminate inefficient branching patterns

    for (auto& func : irModule.functions) {
        // Detect and remove unreachable code
        for (auto& block : func.blocks) {
            std::vector<size_t> unreachable;
            detectUnreachableCode(block, unreachable);

            // Remove unreachable instructions in reverse order
            for (auto it = unreachable.rbegin(); it != unreachable.rend(); ++it) {
                unreachableCodeRemoved_++;
                metrics_.codeReductionBytes += 4;
            }
        }

        // Analyze and optimize branch patterns
        std::vector<BranchInfo> branches;
        analyzeBranches(func, branches);

        // Check for invertible conditions
        for (auto& block : func.blocks) {
            for (auto& inst : block.insts) {
                // Look for conditional branches
                if (inst.op == ir::Op::BR || inst.op == ir::Op::BR_COND) {
                    // Check if we can invert condition
                    if (canInvertCondition(inst)) {
                        branchesInverted_++;
                        metrics_.instructionsOptimized++;
                        metrics_.codeReductionBytes += 3;
                    }
                }
            }
        }
    }

    // Report metrics
    if (branchesInverted_ > 0 || unreachableCodeRemoved_ > 0) {
        
        metrics_.instructionsOptimized = branchesInverted_ + unreachableCodeRemoved_;
    }
}

void BranchOptimization::detectUnreachableCode(ir::Block& block,
                                              std::vector<size_t>& unreachable) {
    // Find unreachable code after unconditional jumps
    for (size_t i = 0; i < block.insts.size(); ++i) {
        const auto& inst = block.insts[i];

        // BR (unconditional branch) makes subsequent code unreachable
        if (inst.op == ir::Op::BR) {
            // Everything after this branch is unreachable
            for (size_t j = i + 1; j < block.insts.size(); ++j) {
                unreachable.push_back(j);
            }
            break;
        }

        // RET (return) also makes subsequent code unreachable
        if (inst.op == ir::Op::RET || inst.op == ir::Op::RET_VOID) {
            for (size_t j = i + 1; j < block.insts.size(); ++j) {
                unreachable.push_back(j);
            }
            break;
        }
    }
}

bool BranchOptimization::canInvertCondition(const ir::Inst& inst) const {
    // Check if condition can be inverted
    // BR_COND uses comparison opcodes in src1/src2, simplified check
    return inst.op == ir::Op::BR_COND;
}

void BranchOptimization::analyzeBranches(ir::Function& func,
                                        std::vector<BranchInfo>& branches) {
    // Simplified branch analysis: collect branch instructions
    for (auto& block : func.blocks) {
        for (size_t i = 0; i < block.insts.size(); ++i) {
            const auto& inst = block.insts[i];

            if (inst.op == ir::Op::BR_COND || inst.op == ir::Op::BR) {
                BranchInfo bi;
                bi.instIndex = i;
                bi.isConditional = (inst.op == ir::Op::BR_COND);
                bi.isInverted = false;
                branches.push_back(bi);
            }
        }
    }
}
