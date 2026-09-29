#pragma once

#include "OptimizationPassBase.hpp"
#include "IR.hpp"
#include <string>
#include <vector>
#include <set>
#include <map>

// Phase C7.3: Loop Invariant Code Motion at IR level
// Moves computations outside of loops when operands don't change
// - Detect loop-invariant operations (operands unchanged in loop)
// - Hoist to pre-loop block
// - Reuse hoisted result in loop

class LoopInvariantCodeMotion : public OptimizationPassBase {
public:
    LoopInvariantCodeMotion();
    ~LoopInvariantCodeMotion() override = default;

    void apply(TranslationUnit& ast) override {}
    void apply(ir::Module& irModule) override;

private:
    struct LoopInfo {
        std::vector<ir::Block*> blocks;     // Blocks in loop
        ir::Block* preheader;               // Pre-loop block
        std::set<std::string> loopVregs;    // Vregs defined in loop
        std::set<std::string> loopVarsRead; // Variables read in loop
    };

    // Detect loops and analyze invariant operands
    void analyzeLoops(ir::Function& func, std::vector<LoopInfo>& loops);

    // Check if operand is loop-invariant
    bool isLoopInvariant(const ir::Operand& op, const LoopInfo& loop) const;

    // Check if instruction is hoistable (no side effects, invariant operands)
    bool isHoistable(const ir::Inst& inst, const LoopInfo& loop) const;

    // Detect if instruction dominates all uses in loop
    bool dominatesUses(size_t instIdx, const LoopInfo& loop) const;

    // Metrics
    int instructionsHoisted_ = 0;
    int loopsAnalyzed_ = 0;
};
