#pragma once

#include "OptimizationPassBase.hpp"
#include "IR.hpp"
#include <string>
#include <vector>
#include <map>
#include <set>

// Phase C7.2: Common Subexpression Elimination at IR level
// Detects identical expressions and reuses computed results
// - Track expression values (op, src1, src2)
// - Reuse vreg holding identical expression
// - Handle aliasing via memory barriers

class CommonSubexpressionElimination : public OptimizationPassBase {
public:
    CommonSubexpressionElimination();
    ~CommonSubexpressionElimination() override = default;

    void apply(TranslationUnit& ast) override {}
    void apply(ir::Module& irModule) override;

private:
    struct ExpressionValue {
        ir::Op op;
        std::string src1;  // Normalized operand
        std::string src2;
        std::string vreg;  // Vreg holding result
        size_t instIndex;  // Instruction index
        bool isValid;      // Still valid (no clobbering)
    };

    struct BlockCSEState {
        std::map<std::string, ExpressionValue> expressions;  // expr_hash → value
        std::set<std::string> clobberedVregs;
    };

    // Normalize operand to string key
    std::string normalizeOperand(const ir::Operand& op) const;

    // Create expression hash from op + operands
    std::string hashExpression(ir::Op op, const std::string& src1,
                              const std::string& src2) const;

    // Check if expression is CSE-eligible
    bool isCSEEligible(ir::Op op) const;

    // Check if instruction clobbers expression result
    bool clobbersExpression(const ir::Inst& inst, const ExpressionValue& expr) const;

    // Metrics
    int redundantExpressions_ = 0;
    int cseEliminationsBlocked_ = 0;
};
