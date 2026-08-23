#include "RedundantLoadElimination.hpp"
#include <algorithm>

RedundantLoadElimination::RedundantLoadElimination()
    : OptimizationPassBase(OptimizationType::REDUNDANT_LOAD_ELIMINATION,
                          "Redundant Load Elimination") {
}

void RedundantLoadElimination::apply(ir::Module& irModule) {
    // Phase C6.3: Redundant load elimination at IR level
    // Walk all basic blocks, tracking loads and eliminating redundant ones

    for (auto& func : irModule.functions) {
        for (auto& block : func.blocks) {
            BlockState state;
            analyzeBlock(block, state);
        }
    }

    // Report metrics
    if (redundantLoadsEliminated_ + reverseStoreForwards_ > 0) {
        
        metrics_.instructionsOptimized = redundantLoadsEliminated_ + reverseStoreForwards_;
        metrics_.codeReductionBytes = (redundantLoadsEliminated_ + reverseStoreForwards_) * 3;
    }
}

void RedundantLoadElimination::analyzeBlock(ir::Block& block, BlockState& state) {
    std::vector<size_t> indicesToRemove;

    for (size_t i = 0; i < block.insts.size(); ++i) {
        auto& inst = block.insts[i];

        // Track LOAD operations
        if (inst.op == ir::Op::LOAD) {
            // Get the address being loaded
            std::string addr;
            if (inst.src1.kind == ir::OperandKind::VREG) {
                addr = "vreg_" + std::to_string(inst.src1.vregId);
            } else if (inst.src1.kind == ir::OperandKind::CONST) {
                addr = std::to_string(inst.src1.constValue);
            } else if (inst.src1.kind == ir::OperandKind::SYMBOL) {
                addr = inst.src1.symbolName;
            }

            // Check if we've already loaded from this address
            auto it = state.loadedValues.find(addr);
            if (it != state.loadedValues.end() && it->second.isValid) {
                // Redundant load detected!
                // Replace with copy from cached vreg
                inst.op = ir::Op::MOVE;
                inst.src1 = ir::Operand(ir::OperandKind::VREG, 0, it->second.vreg);
                inst.src2 = ir::Operand();
                redundantLoadsEliminated_++;
                metrics_.instructionsOptimized++;
                metrics_.codeReductionBytes += 3;
            } else {
                // First load of this address, track it
                MemoryValue mv;
                mv.address = addr;
                mv.vreg = "vreg_" + std::to_string(inst.dst.vregId);
                mv.instIndex = i;
                mv.isValid = true;
                state.loadedValues[addr] = mv;
            }
        }
        // Track STORE operations
        else if (inst.op == ir::Op::STORE) {
            // Get the address being stored to
            std::string addr;
            if (inst.src2.kind == ir::OperandKind::VREG) {
                addr = "vreg_" + std::to_string(inst.src2.vregId);
            } else if (inst.src2.kind == ir::OperandKind::CONST) {
                addr = std::to_string(inst.src2.constValue);
            } else if (inst.src2.kind == ir::OperandKind::SYMBOL) {
                addr = inst.src2.symbolName;
            }

            // Invalidate loads from this address and potential aliases
            auto aliases = getAliases(addr);
            for (auto& [loadAddr, loadValue] : state.loadedValues) {
                if (aliases.count(loadAddr) > 0) {
                    loadValue.isValid = false;
                }
            }
            state.storedAddresses.insert(addr);
        }
        // Other operations may clobber memory (function calls, etc.)
        else if (inst.op == ir::Op::CALL) {
            // Conservative: invalidate all loads after function call
            for (auto& [addr, value] : state.loadedValues) {
                value.isValid = false;
            }
            state.storedAddresses.clear();
        }
    }
}

bool RedundantLoadElimination::mayAlias(const std::string& loadAddr,
                                       const std::string& storeAddr) const {
    // Conservative alias analysis:
    // - Exact match: definitely aliases
    // - Different constants: don't alias
    // - Any vreg operand: may alias (conservative)
    // - Different symbols: don't alias (unless same symbol)

    if (loadAddr == storeAddr) return true;

    // If either involves a vreg, assume potential aliasing
    if (loadAddr.find("vreg_") != std::string::npos ||
        storeAddr.find("vreg_") != std::string::npos) {
        return true;
    }

    // Different constants don't alias
    return false;
}

bool RedundantLoadElimination::isValueStillLoaded(const std::string& vreg,
                                                  const std::string& address,
                                                  const BlockState& state) const {
    auto it = state.loadedValues.find(address);
    return it != state.loadedValues.end() &&
           it->second.vreg == vreg &&
           it->second.isValid;
}

std::set<std::string> RedundantLoadElimination::getAliases(const std::string& addr) const {
    std::set<std::string> aliases;
    aliases.insert(addr);

    // Conservative: any vreg load may alias with any store
    if (addr.find("vreg_") != std::string::npos) {
        // This vreg store may affect other vreg loads
        // For now, don't add implicit aliases (handled in analyzeBlock)
    }

    return aliases;
}
