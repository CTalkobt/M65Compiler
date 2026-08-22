#pragma once
#include "OptimizationPassBase.hpp"
#include <map>
#include <string>

namespace ir { struct Module; }
class TranslationUnit;

// Bit Manipulation Strength Reduction Optimization
// Optimizes bit operations with strength reduction and special instruction hints
class BitManipulationReduction : public OptimizationPassBase {
public:
    BitManipulationReduction();
    ~BitManipulationReduction() override;

    void apply(TranslationUnit& ast) override;
    void apply(ir::Module& irModule) override;

private:
    struct BitPattern {
        std::string operation;          // AND, OR, XOR, shift, rotate
        unsigned int mask;              // Constant mask/shift amount
        bool isStrengthReducible;      // Can be replaced with cheaper op
        std::string replacementOp;      // Cheaper operation
        int bytesReduced;              // Estimated savings
    };

    int patternsOptimized_;
    int bytesReduced_;

    // Pattern detection
    void detectBitPatterns(TranslationUnit& ast);
    void detectBitPatternsIR(ir::Module& irModule);

    // Strength reduction rules
    bool matchesMaskPattern(unsigned int mask) const;
    std::string getOptimalInstruction(unsigned int mask, const std::string& op);

    // Code generation
    void applyStrengthReduction(TranslationUnit& ast);
    void applyStrengthReductionIR(ir::Module& irModule);
};
