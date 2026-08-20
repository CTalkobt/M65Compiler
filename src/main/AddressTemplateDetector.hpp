#pragma once

#include "ASTNode.h"
#include <string>
#include <vector>
#include <memory>

/**
 * Phase 89: Address Template Detection
 *
 * Detects common address calculation patterns and classifies them
 * for optimized code generation using hardcoded 6502 templates.
 *
 * Supported patterns:
 * 1. Linear row-major: (row * WIDTH) + col (common in graphics)
 * 2. Sprite offset: base + (index * SIZE)
 * 3. Cumulative stride: base + (x * SX) + (y * SY)
 * 4. Hardware patterns: VIC-IV, SID, DMA addressing
 *
 * Expected code reduction: 30-50% on address calculation overhead
 */

class AddressTemplateDetector {
public:
    enum class PatternType {
        NONE,
        LINEAR_ROW_MAJOR,      // (row * width) + col or (col * height) + row
        SPRITE_OFFSET,         // base + (index * size)
        CUMULATIVE_STRIDE,     // base + (x * sx) + (y * sy)
        HARDWARE_PATTERN       // VIC, SID, DMA, etc.
    };

    struct MatchedPattern {
        PatternType type;
        std::string description;
        std::vector<std::string> operands;  // Variables involved
        std::vector<int> constants;          // Constants (widths, sizes, strides)
        bool canOptimize;
        int estimatedCycleSavings;           // Expected cycles saved vs naive
        int estimatedByteSavings;            // Expected bytes saved vs naive

        MatchedPattern()
            : type(PatternType::NONE), canOptimize(false),
              estimatedCycleSavings(0), estimatedByteSavings(0) {}
    };

    // Main detection method
    MatchedPattern detectPattern(const BinaryOp& expr) const;

    // Helper: detect linear row-major pattern (row * width + col)
    MatchedPattern detectLinearRowMajor(const BinaryOp& expr) const;

    // Helper: detect sprite offset pattern (base + index * size)
    MatchedPattern detectSpriteOffset(const BinaryOp& expr) const;

    // Helper: detect simple multiplication pattern (e.g., row * 40)
    MatchedPattern detectSimpleMultiplication(const BinaryOp& expr) const;

    // Helper: detect cumulative stride pattern
    MatchedPattern detectCumulativeStride(const BinaryOp& expr) const;

    // Helper: detect hardware-specific patterns
    MatchedPattern detectHardwarePattern(const BinaryOp& expr) const;

private:
    // Utility: check if an expression is a constant
    bool isConstant(const Expression& expr, int& out_value) const;

    // Utility: check if an expression is a variable reference
    bool isVariable(const Expression& expr, std::string& out_name) const;

    // Utility: check if an expression is a binary op
    const BinaryOp* asBinaryOp(const Expression& expr) const;

    // Utility: validate operand constraints
    bool validateOperands(const std::vector<std::string>& operands) const;

    // Check if constant is a known width (40, 80, 160, 320, 640)
    bool isKnownWidth(int width) const;

    // Check if constant is a known size for sprite/offset patterns
    bool isKnownSize(int size) const;

    // Estimate cycle savings for pattern type
    int estimateCycles(PatternType type, const std::vector<int>& constants) const;

    // Estimate byte savings for pattern type
    int estimateBytes(PatternType type, const std::vector<int>& constants) const;
};

/**
 * Implementation Notes:
 *
 * LINEAR_ROW_MAJOR Detection:
 * Pattern: (a * C) + b where C ∈ {40, 80, 160, 320, 640}
 * Or: (a * C) where C is known width (simple multiplication)
 *
 * Example IR for screen[row * 40 + col]:
 *   BinaryOp: BINARY_ADD
 *     left: BinaryOp: BINARY_MUL
 *       left: VariableRef: row
 *       right: IntegerLiteral: 40
 *     right: VariableRef: col
 *
 * SPRITE_OFFSET Detection:
 * Pattern: a + (b * C) where C ∈ {3, 8, 16, 32, 256}
 * Or: (b * C) + a
 *
 * CUMULATIVE_STRIDE Detection:
 * Pattern: a + (b * C1) + (c * C2)
 * Matches complex memory layouts with multiple dimensions
 *
 * HARDWARE_PATTERN Detection:
 * Hardcoded patterns for known MEGA65 hardware:
 * - VIC-IV register access
 * - SID addressing (4 × SID offset)
 * - DMA descriptor patterns
 *
 * Optimization Potential:
 * - Naive (row * 40 + col): 15+ instructions
 * - Template-based: 7-9 instructions
 * - Savings: 40-50% for single operation
 * - Aggregate savings in graphics code: 10-30%
 */
