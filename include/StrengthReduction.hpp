#pragma once

#include "OptimizationPassBase.hpp"
#include "IR.hpp"
#include <string>
#include <vector>
#include <map>

// Phase C6.1: Strength Reduction at IR level
// Optimizes expensive operations to cheaper alternatives:
// - Multiply by 2^N → shift left by N
// - Divide by 2^N → shift right by N (unsigned)
// - i++ → ++i (avoid temporary for post-increment)
// - Loop-invariant strength reduction (move multiplies outside loops)

class StrengthReduction : public OptimizationPassBase {
public:
    StrengthReduction();
    ~StrengthReduction() override = default;

    // AST-level pass (deferred to IR)
    void apply(TranslationUnit& ast) override {}

    // IR-level optimization
    void apply(ir::Module& irModule) override;

private:
    struct ReductionOpportunity {
        std::string type;  // "mul_shift", "div_shift", "postinc_preinc"
        ir::Inst* instruction;
        int shiftAmount;  // For multiply/divide by power of 2
        bool beneficial;  // Worth optimizing
    };

    // Detect multiply by power of 2
    bool isMultiplyByPowerOf2(const ir::Inst& inst, int& shiftAmount) const;

    // Detect divide by power of 2
    bool isDivideByPowerOf2(const ir::Inst& inst, int& shiftAmount) const;

    // Check if divisor is unsigned divide
    bool isUnsignedDivide(const ir::Inst& inst) const;

    // Detect post-increment that could be pre-increment
    bool isOptimizablePostIncrement(const ir::Inst& inst) const;

    // Loop-invariant strength reduction: detect multiplies that can be hoisted
    bool isLoopInvariantMultiply(const ir::Inst& inst, ir::Block* loopBody) const;

    // Count set bits (for power-of-2 detection: should be exactly 1)
    static int countSetBits(unsigned val);

    // Metrics
    int multiplyReductions_ = 0;
    int divideReductions_ = 0;
    int incrementOptimizations_ = 0;
    int loopHoistingOpportunities_ = 0;
};
