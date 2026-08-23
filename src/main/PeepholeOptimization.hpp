#pragma once

#include "OptimizationPassBase.hpp"
#include "IR.hpp"
#include <string>
#include <vector>

// Phase C7.6: Peephole Optimization at IR level
// IR-level instruction sequence pattern matching
// - Replace inefficient patterns with optimized sequences
// - Handle register allocation artifacts
// - Combine multiple operations into single efficient instruction

class PeepholeOptimization : public OptimizationPassBase {
public:
    PeepholeOptimization();
    ~PeepholeOptimization() override = default;

    void apply(TranslationUnit& ast) override {}
    void apply(ir::Module& irModule) override;

private:
    struct Pattern {
        std::vector<ir::Op> ops;      // Sequence of operations to match
        std::vector<ir::Op> replacement; // Replacement sequence
        int byteSavings;
    };

    // Define peephole patterns
    void initializePatterns(std::vector<Pattern>& patterns);

    // Check if instruction sequence matches pattern
    bool matchesPattern(const ir::Block& block, size_t startIdx,
                       const Pattern& pattern) const;

    // Apply pattern replacement
    void applyPattern(ir::Block& block, size_t startIdx,
                      const Pattern& pattern);

    // Metrics
    int patternsMatched_ = 0;
    int bytesOptimized_ = 0;
};
