#include "DeadStoreElimination.hpp"
#include <algorithm>

DeadStoreElimination::DeadStoreElimination()
    : OptimizationPassBase(OptimizationType::DEAD_STORE_ELIMINATION,
                          "Dead Store Elimination") {
}

void DeadStoreElimination::apply(ir::Module& irModule) {
    // Phase C6.4: Dead store elimination at IR level
    // Walk all basic blocks, identifying and eliminating unused stores

    for (auto& func : irModule.functions) {
        for (auto& block : func.blocks) {
            // Find dead stores in this block
            auto deadStores = findDeadStores(block);

            // Remove dead stores (in reverse order to preserve indices)
            for (auto it = deadStores.rbegin(); it != deadStores.rend(); ++it) {
                size_t idx = *it;
                if (idx < block.insts.size()) {
                    block.insts.erase(block.insts.begin() + idx);
                    deadStoresEliminated_++;
                    metrics_.instructionsOptimized++;
                    metrics_.codeReductionBytes += 3;  // Typical STORE size
                }
            }
        }
    }

    // Report metrics
    if (deadStoresEliminated_ > 0) {
        metrics_.optimizationsApplied = "dead-store-elimination";
        metrics_.instructionsOptimized = deadStoresEliminated_;
    }
}

std::set<size_t> DeadStoreElimination::findDeadStores(ir::Block& block) {
    std::set<size_t> deadStores;
    BlockLiveness liveness;

    // Backward pass: compute liveness
    analyzeBlockLiveness(block, liveness);

    // Forward pass: identify dead stores
    for (size_t i = 0; i < block.insts.size(); ++i) {
        const auto& inst = block.insts[i];

        if (inst.op == ir::Op::STORE) {
            // Get the address being stored to
            std::string addr;
            if (inst.src2.kind == ir::OperandKind::VREG) {
                addr = "vreg_" + std::to_string(inst.src2.vregId);
            } else if (inst.src2.kind == ir::OperandKind::CONST) {
                addr = std::to_string(inst.src2.constValue);
            } else if (inst.src2.kind == ir::OperandKind::SYMBOL) {
                addr = inst.src2.symbolName;
            }

            // Check if this is a dead store
            if (!volatileAddresses_.count(addr) && !isLiveAfter(addr, i, liveness)) {
                // Check if there's a subsequent store to same address
                bool hasLaterStore = false;
                for (size_t j = i + 1; j < block.insts.size(); ++j) {
                    if (block.insts[j].op == ir::Op::STORE) {
                        std::string laterAddr;
                        const auto& laterInst = block.insts[j];
                        if (laterInst.src2.kind == ir::OperandKind::CONST) {
                            laterAddr = std::to_string(laterInst.src2.constValue);
                        } else if (laterInst.src2.kind == ir::OperandKind::SYMBOL) {
                            laterAddr = laterInst.src2.symbolName;
                        }
                        if (laterAddr == addr) {
                            hasLaterStore = true;
                            break;
                        }
                    }
                }

                // Dead store: either not live or overwritten without read
                deadStores.insert(i);
            }
        }
    }

    return deadStores;
}

void DeadStoreElimination::analyzeBlockLiveness(ir::Block& block,
                                               BlockLiveness& liveness) {
    // Simplified liveness analysis (backward pass)
    // Start from end of block and work backward

    std::set<std::string> liveSet;  // Variables live at current point

    for (int i = (int)block.insts.size() - 1; i >= 0; --i) {
        const auto& inst = block.insts[i];

        // Kill: if this instruction defines a variable
        if (inst.op == ir::Op::STORE) {
            std::string addr;
            if (inst.src2.kind == ir::OperandKind::SYMBOL) {
                addr = inst.src2.symbolName;
            }
            if (!addr.empty()) {
                liveSet.erase(addr);
            }
        }

        // Generate: if this instruction uses a variable
        if (inst.op == ir::Op::LOAD || inst.op == ir::Op::ADD ||
            inst.op == ir::Op::SUB || inst.op == ir::Op::MUL ||
            inst.op == ir::Op::DIV) {
            if (inst.src1.kind == ir::OperandKind::SYMBOL) {
                liveSet.insert(inst.src1.symbolName);
            }
            if (inst.src2.kind == ir::OperandKind::SYMBOL) {
                liveSet.insert(inst.src2.symbolName);
            }
        }
    }

    liveness.liveOut = liveSet;
}

bool DeadStoreElimination::isLiveAfter(const std::string& addr, size_t instIndex,
                                      const BlockLiveness& liveness) const {
    // Conservative: if address is in liveOut, assume it's used
    // More precise analysis would track individual variable liveness

    if (liveness.liveOut.count(addr) > 0) {
        return true;
    }

    // If address contains a vreg, assume it's live (conservative)
    if (addr.find("vreg_") != std::string::npos) {
        return true;
    }

    return false;
}

bool DeadStoreElimination::isDeadStore(const ir::Inst& inst,
                                      const BlockLiveness& liveness) const {
    if (inst.op != ir::Op::STORE) return false;

    // Cannot eliminate volatile stores
    if (inst.src2.kind == ir::OperandKind::SYMBOL) {
        if (volatileAddresses_.count(inst.src2.symbolName) > 0) {
            return false;
        }
    }

    // Dead if value not live after store
    std::string addr;
    if (inst.src2.kind == ir::OperandKind::SYMBOL) {
        addr = inst.src2.symbolName;
    }

    return !addr.empty() && liveness.liveOut.count(addr) == 0;
}

void DeadStoreElimination::markVolatile(const std::string& addr) {
    volatileAddresses_.insert(addr);
}
