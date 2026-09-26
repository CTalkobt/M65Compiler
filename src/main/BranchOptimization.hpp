#pragma once

#include "OptimizationPassBase.hpp"
#include "IR.hpp"
#include <string>
#include <vector>
#include <set>

// Phase C7.4: Branch Optimization at IR level
// Eliminates inefficient branching patterns
// - Branch folding (unreachable code after unconditional jumps)
// - Branch inversion (flip condition to avoid jump)
// - Redundant branch elimination
// - Merge redundant branch targets

class BranchOptimization : public OptimizationPassBase {
public:
    BranchOptimization();
    ~BranchOptimization() override = default;

    void apply(TranslationUnit& ast) override {}
    void apply(ir::Module& irModule) override;

private:
    struct BranchInfo {
        size_t instIndex;
        bool isConditional;
        std::string target;
        std::string fallthrough;
        bool isInverted;
    };

    // Detect unreachable code after unconditional branches
    void detectUnreachableCode(ir::Block& block, std::vector<size_t>& unreachable);

    // Check if condition can be inverted to avoid branch
    bool canInvertCondition(const ir::Inst& inst) const;

    // Analyze branch patterns
    void analyzeBranches(ir::Function& func, std::vector<BranchInfo>& branches);

    // Metrics
    int branchesInverted_ = 0;
    int unreachableCodeRemoved_ = 0;
    int redundantBranchesEliminated_ = 0;
};
