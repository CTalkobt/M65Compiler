#pragma once

#include "OptimizationPassBase.hpp"
#include "IR.hpp"
#include <string>
#include <vector>

// Phase C6.2: Algebraic Simplification at IR level
// Eliminates identity and annihilator patterns:
// - a * 1 = a, a / 1 = a (identity)
// - a + 0 = a, a - 0 = a (identity)
// - a | 0 = a, a & ~0 = a (identity)
// - a * 0 = 0, a & 0 = 0 (annihilator)
// - Boolean: a || 1 = 1, a && 0 = 0 (short-circuit)
// - a << 0 = a, a >> 0 = a (identity shift)

class AlgebraicSimplification : public OptimizationPassBase {
public:
    AlgebraicSimplification();
    ~AlgebraicSimplification() override = default;

    // AST-level pass (deferred to IR)
    void apply(TranslationUnit& ast) override {}

    // IR-level optimization
    void apply(ir::Module& irModule) override;

private:
    struct SimplificationOpportunity {
        std::string type;  // "identity", "annihilator", "boolean", "shift"
        ir::Inst* instruction;
        int operandIdx;  // Which operand is the identity/annihilator
        ir::Operand replacement;  // What to replace with
    };

    // Check if value is arithmetic identity (1 for *, /, 0 for +, -)
    bool isArithmeticIdentity(ir::Op op, unsigned val) const;

    // Check if value is arithmetic annihilator (0 for *, &)
    bool isArithmeticAnnihilator(ir::Op op, unsigned val) const;

    // Check if value is bitwise identity (0 for |, ~0 for &)
    bool isBitwiseIdentity(ir::Op op, unsigned val) const;

    // Check if value is bitwise annihilator (0 for &)
    bool isBitwiseAnnihilator(ir::Op op, unsigned val) const;

    // Check if shift amount is zero (identity shift)
    bool isIdentityShift(ir::Op op, unsigned shiftAmount) const;

    // Check for boolean short-circuit (a || 1 = 1, a && 0 = 0)
    bool isBooleanShortCircuit(ir::Op op, unsigned val, ir::Operand& result) const;

    // Simplify instruction in place, return true if changed
    bool simplifyInstruction(ir::Inst& inst);

    // Metrics
    int identitiesEliminated_ = 0;
    int annihilatorsEliminated_ = 0;
    int shortCircuitsApplied_ = 0;
    int shiftsSimplified_ = 0;
};
