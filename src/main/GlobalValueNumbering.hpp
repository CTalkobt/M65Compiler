#pragma once

#include "OptimizationPassBase.hpp"
#include "IR.hpp"
#include <string>
#include <map>
#include <set>
#include <unordered_map>

// Phase C7.5: Global Value Numbering at IR level
// Tracks value identities across basic blocks
// - Reuse computed values globally (not just within blocks)
// - Build equivalence classes for values
// - More powerful than CSE (works across block boundaries)

class GlobalValueNumbering : public OptimizationPassBase {
public:
    GlobalValueNumbering();
    ~GlobalValueNumbering() override = default;

    void apply(TranslationUnit& ast) override {}
    void apply(ir::Module& irModule) override;

private:
    struct ValueNumber {
        size_t number;       // Unique value number
        std::string expr;    // Expression representation
        size_t firstInstIdx; // First instruction producing this value
        std::string resultVreg; // Vreg holding result
    };

    struct BlockGVNState {
        std::map<std::string, size_t> exprToValue;     // Expression hash → value number
        std::map<size_t, ValueNumber> valueNumbers;    // Value number → info
        size_t nextValueNumber = 1;
    };

    // Compute value number for expression
    size_t getValueNumber(const ir::Inst& inst, BlockGVNState& state);

    // Build expression hash key
    std::string buildExpressionKey(ir::Op op, const ir::Operand& src1,
                                   const ir::Operand& src2) const;

    // Check if value number exists
    bool hasValueNumber(const std::string& key, const BlockGVNState& state) const;

    // Metrics
    int valuesReused_ = 0;
    int equivalencesFound_ = 0;
};
