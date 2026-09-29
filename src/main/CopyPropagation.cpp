#include "CopyPropagation.hpp"
#include <algorithm>

CopyPropagation::CopyPropagation()
    : OptimizationPassBase(OptimizationType::COPY_PROPAGATION,
                          "Copy Propagation") {
}

void CopyPropagation::apply(ir::Module& irModule) {
    // Phase C7.1: Copy Propagation at IR level
    // Eliminate redundant copy operations by tracking copy chains

    for (auto& func : irModule.functions) {
        for (auto& block : func.blocks) {
            copyMap_.clear();
            std::vector<size_t> indicesToRemove;

            // First pass: record all copy operations
            for (size_t i = 0; i < block.insts.size(); ++i) {
                const auto& inst = block.insts[i];

                // MOVE operations are copies
                if (inst.op == ir::Op::COPY && inst.src1.kind == ir::OperandKind::VREG) {
                    std::string dst = "vreg_" + std::to_string(inst.dest.vregId);
                    std::string src = "vreg_" + std::to_string(inst.src1.vregId);

                    CopyInfo ci;
                    ci.source = getUltimateSource(src);
                    ci.instIndex = i;
                    ci.isValid = true;
                    copyMap_[dst] = ci;
                }
                // Memory operations invalidate copies
                else if (inst.op == ir::Op::STORE || inst.op == ir::Op::CALL) {
                    copyMap_.clear();
                }
            }

            // Second pass: replace copy uses with ultimate sources
            for (size_t i = 0; i < block.insts.size(); ++i) {
                auto& inst = block.insts[i];

                // Check if src1 is a copy target
                if (inst.src1.kind == ir::OperandKind::VREG) {
                    std::string src1 = "vreg_" + std::to_string(inst.src1.vregId);
                    std::string ultimate = getUltimateSource(src1);

                    if (ultimate != src1) {
                        // Replace with ultimate source
                        chainsFollowed_++;
                        // Parse "vreg_N" back to vregId
                        size_t vregId = std::stoul(ultimate.substr(6));
                        inst.src1 = ir::Operand::vreg(vregId, ir::Type::I32);
                        metrics_.instructionsOptimized++;
                        metrics_.codeReductionBytes += 2;
                    }
                }

                // Check if src2 is a copy target
                if (inst.src2.kind == ir::OperandKind::VREG) {
                    std::string src2 = "vreg_" + std::to_string(inst.src2.vregId);
                    std::string ultimate = getUltimateSource(src2);

                    if (ultimate != src2) {
                        chainsFollowed_++;
                        size_t vregId = std::stoul(ultimate.substr(6));
                        inst.src2 = ir::Operand::vreg(vregId, ir::Type::I32);
                        metrics_.instructionsOptimized++;
                        metrics_.codeReductionBytes += 2;
                    }
                }
            }

            // Third pass: remove dead copies (copies whose results are never used)
            for (auto it = block.insts.rbegin(); it != block.insts.rend(); ++it) {
                if (it->op == ir::Op::COPY && it->src1.kind == ir::OperandKind::VREG) {
                    std::string dst = "vreg_" + std::to_string(it->dest.vregId);

                    // Check if this vreg is used in any subsequent instruction
                    bool isUsed = false;
                    for (auto it2 = std::next(it); it2 != block.insts.rend(); ++it2) {
                        if ((it2->src1.kind == ir::OperandKind::VREG &&
                             it2->src1.vregId == it->dest.vregId) ||
                            (it2->src2.kind == ir::OperandKind::VREG &&
                             it2->src2.vregId == it->dest.vregId)) {
                            isUsed = true;
                            break;
                        }
                    }

                    if (!isUsed) {
                        copiesEliminated_++;
                        metrics_.codeReductionBytes += 3;
                    }
                }
            }
        }
    }

    // Report metrics
    if (chainsFollowed_ > 0 || copiesEliminated_ > 0) {
        
        metrics_.instructionsOptimized = chainsFollowed_ + copiesEliminated_;
    }
}

std::string CopyPropagation::getUltimateSource(const std::string& vreg) const {
    // Follow copy chains to get the ultimate source
    std::string current = vreg;
    std::set<std::string> visited;  // Prevent infinite loops

    while (visited.count(current) == 0) {
        visited.insert(current);
        auto it = copyMap_.find(current);
        if (it != copyMap_.end() && it->second.isValid) {
            current = it->second.source;
        } else {
            break;
        }
    }

    return current;
}

bool CopyPropagation::isCopyTarget(const ir::Operand& op) const {
    if (op.kind != ir::OperandKind::VREG) {
        return false;
    }
    std::string vregName = "vreg_" + std::to_string(op.vregId);
    return copyMap_.count(vregName) > 0;
}
