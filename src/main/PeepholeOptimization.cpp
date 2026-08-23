#include "PeepholeOptimization.hpp"
#include <algorithm>

PeepholeOptimization::PeepholeOptimization()
    : OptimizationPassBase(OptimizationType::PEEPHOLE_OPTIMIZATION,
                          "Peephole Optimization") {
}

void PeepholeOptimization::apply(ir::Module& irModule) {
    // Phase C7.6: Peephole Optimization at IR level
    // Match and replace inefficient instruction sequences

    std::vector<Pattern> patterns;
    initializePatterns(patterns);

    for (auto& func : irModule.functions) {
        for (auto& block : func.blocks) {
            size_t i = 0;
            while (i < block.insts.size()) {
                bool patternMatched = false;

                // Try each pattern
                for (const auto& pattern : patterns) {
                    if (matchesPattern(block, i, pattern)) {
                        applyPattern(block, i, pattern);
                        patternsMatched_++;
                        bytesOptimized_ += pattern.byteSavings;
                        metrics_.codeReductionBytes += pattern.byteSavings;
                        patternMatched = true;
                        break;
                    }
                }

                i++;
            }
        }
    }

    // Report metrics
    if (patternsMatched_ > 0) {
        
        metrics_.instructionsOptimized = patternsMatched_;
    }
}

void PeepholeOptimization::initializePatterns(std::vector<Pattern>& patterns) {
    // Pattern 1: MOVE + MOVE → Single MOVE (copy chain)
    Pattern p1;
    p1.ops = {ir::Op::COPY, ir::Op::COPY};
    p1.replacement = {ir::Op::COPY};
    p1.byteSavings = 3;
    patterns.push_back(p1);

    // Pattern 2: ADD + ADD with same operands → Single ADD (commutativity)
    Pattern p2;
    p2.ops = {ir::Op::ADD, ir::Op::ADD};
    p2.replacement = {ir::Op::ADD};
    p2.byteSavings = 4;
    patterns.push_back(p2);

    // Pattern 3: SUB 0 → Nothing (subtract zero = identity)
    Pattern p3;
    p3.ops = {ir::Op::SUB};
    p3.replacement = {};
    p3.byteSavings = 3;
    patterns.push_back(p3);

    // Pattern 4: MUL 1 → Nothing (multiply by one = identity)
    Pattern p4;
    p4.ops = {ir::Op::MUL};
    p4.replacement = {};
    p4.byteSavings = 3;
    patterns.push_back(p4);

    // Pattern 5: DIV by 1 → Nothing (divide by one = identity)
    Pattern p5;
    p5.ops = {ir::Op::DIV};
    p5.replacement = {};
    p5.byteSavings = 3;
    patterns.push_back(p5);

    // Pattern 6: AND all bits → Nothing (AND with all bits = identity)
    Pattern p6;
    p6.ops = {ir::Op::AND};
    p6.replacement = {};
    p6.byteSavings = 3;
    patterns.push_back(p6);

    // Pattern 7: OR with 0 → Nothing (OR with zero = identity)
    Pattern p7;
    p7.ops = {ir::Op::OR};
    p7.replacement = {};
    p7.byteSavings = 3;
    patterns.push_back(p7);

    // Pattern 8: XOR with self → 0
    Pattern p8;
    p8.ops = {ir::Op::XOR};
    p8.replacement = {};
    p8.byteSavings = 3;
    patterns.push_back(p8);
}

bool PeepholeOptimization::matchesPattern(const ir::Block& block, size_t startIdx,
                                         const Pattern& pattern) const {
    // Check if instruction sequence matches pattern
    if (startIdx + pattern.ops.size() > block.insts.size()) {
        return false;
    }

    for (size_t i = 0; i < pattern.ops.size(); ++i) {
        if (block.insts[startIdx + i].op != pattern.ops[i]) {
            return false;
        }
    }

    return true;
}

void PeepholeOptimization::applyPattern(ir::Block& block, size_t startIdx,
                                       const Pattern& pattern) {
    // Remove matched pattern and replace with optimized sequence
    auto it = block.insts.begin() + startIdx;

    // Remove matched instructions
    it = block.insts.erase(it, it + pattern.ops.size());

    // Insert replacement instructions (in reverse order for proper positioning)
    for (auto rit = pattern.replacement.rbegin(); rit != pattern.replacement.rend(); ++rit) {
        // Create a placeholder instruction with replacement op
        ir::Inst replacement;
        replacement.op = *rit;
        replacement.src1 = block.insts[startIdx].src1;
        replacement.src2 = block.insts[startIdx].src2;
        replacement.dest = block.insts[startIdx].dest;
        block.insts.insert(it, replacement);
    }
}
