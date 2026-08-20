#include "AddressTemplateDetector.hpp"
#include <algorithm>
#include <cmath>

AddressTemplateDetector::MatchedPattern
AddressTemplateDetector::detectPattern(const BinaryOp& expr) const {
    // Try patterns in order of specificity

    // Try cumulative stride (most specific)
    auto pattern = detectCumulativeStride(expr);
    if (pattern.canOptimize) return pattern;

    // Try hardware patterns
    pattern = detectHardwarePattern(expr);
    if (pattern.canOptimize) return pattern;

    // Try linear row-major (handles both direct and optimized IR forms)
    pattern = detectLinearRowMajor(expr);
    if (pattern.canOptimize) return pattern;

    // Try sprite offset
    pattern = detectSpriteOffset(expr);
    if (pattern.canOptimize) return pattern;

    // Try single multiplication (e.g., row * 40 without the +col part)
    pattern = detectSimpleMultiplication(expr);
    if (pattern.canOptimize) return pattern;

    // No pattern matched
    return MatchedPattern();
}

AddressTemplateDetector::MatchedPattern
AddressTemplateDetector::detectLinearRowMajor(const BinaryOp& expr) const {
    MatchedPattern result;

    // Pattern: (a * WIDTH) + b  OR  (b * WIDTH) + a
    // WHERE WIDTH ∈ {40, 80, 160, 320, 640}

    if (expr.op != BinaryOp::Op::ADD) {
        return result;
    }

    // Check left side for multiply pattern
    const auto* leftMul = asBinaryOp(*expr.left);
    if (leftMul && leftMul->op == BinaryOp::Op::MUL) {
        int width_value = 0;
        std::string row_name, col_name;

        // Pattern: (row * WIDTH) + col
        if (isVariable(*leftMul->left, row_name) &&
            isConstant(*leftMul->right, width_value) &&
            isKnownWidth(width_value) &&
            isVariable(*expr.right, col_name)) {

            result.type = PatternType::LINEAR_ROW_MAJOR;
            result.description = "Linear row-major (row * " + std::to_string(width_value) + " + col)";
            result.operands = {row_name, col_name};
            result.constants = {width_value};
            result.canOptimize = true;
            result.estimatedCycleSavings = 6;
            result.estimatedByteSavings = 8;
            return result;
        }

        // Pattern: (col * WIDTH) + row (reversed operands)
        if (isVariable(*leftMul->left, col_name) &&
            isConstant(*leftMul->right, width_value) &&
            isKnownWidth(width_value) &&
            isVariable(*expr.right, row_name)) {

            result.type = PatternType::LINEAR_ROW_MAJOR;
            result.description = "Linear row-major (col * " + std::to_string(width_value) + " + row)";
            result.operands = {col_name, row_name};
            result.constants = {width_value};
            result.canOptimize = true;
            result.estimatedCycleSavings = 6;
            result.estimatedByteSavings = 8;
            return result;
        }
    }

    // Check right side for multiply pattern (shouldn't occur but handle anyway)
    const auto* rightMul = asBinaryOp(*expr.right);
    if (rightMul && rightMul->op == BinaryOp::Op::MUL) {
        int width_value = 0;
        std::string row_name, col_name;

        // Pattern: col + (row * WIDTH)
        if (isVariable(*expr.left, col_name) &&
            isVariable(*rightMul->left, row_name) &&
            isConstant(*rightMul->right, width_value) &&
            isKnownWidth(width_value)) {

            result.type = PatternType::LINEAR_ROW_MAJOR;
            result.description = "Linear row-major (col + row * " + std::to_string(width_value) + ")";
            result.operands = {row_name, col_name};
            result.constants = {width_value};
            result.canOptimize = true;
            result.estimatedCycleSavings = 6;
            result.estimatedByteSavings = 8;
            return result;
        }
    }

    return result;
}

AddressTemplateDetector::MatchedPattern
AddressTemplateDetector::detectSpriteOffset(const BinaryOp& expr) const {
    MatchedPattern result;

    // Pattern: a + (b * SIZE)  OR  (b * SIZE) + a
    // WHERE SIZE ∈ {3, 8, 16, 32, 256}

    if (expr.op != BinaryOp::Op::ADD) {
        return result;
    }

    // Check left side for multiply
    const auto* leftMul = asBinaryOp(*expr.left);
    if (leftMul && leftMul->op == BinaryOp::Op::MUL) {
        int size_value = 0;
        std::string base_name, index_name;

        if (isVariable(*expr.left, base_name) &&
            isVariable(*leftMul->left, index_name) &&
            isConstant(*leftMul->right, size_value) &&
            isKnownSize(size_value)) {

            // Pattern: base + (index * SIZE)
            result.type = PatternType::SPRITE_OFFSET;
            result.description = "Sprite offset (base + index * " + std::to_string(size_value) + ")";
            result.operands = {base_name, index_name};
            result.constants = {size_value};
            result.canOptimize = true;
            result.estimatedCycleSavings = 4;
            result.estimatedByteSavings = 6;
            return result;
        }
    }

    // Check right side for multiply
    const auto* rightMul = asBinaryOp(*expr.right);
    if (rightMul && rightMul->op == BinaryOp::Op::MUL) {
        int size_value = 0;
        std::string base_name, index_name;

        if (isVariable(*expr.left, base_name) &&
            isVariable(*rightMul->left, index_name) &&
            isConstant(*rightMul->right, size_value) &&
            isKnownSize(size_value)) {

            // Pattern: base + (index * SIZE)
            result.type = PatternType::SPRITE_OFFSET;
            result.description = "Sprite offset (base + index * " + std::to_string(size_value) + ")";
            result.operands = {base_name, index_name};
            result.constants = {size_value};
            result.canOptimize = true;
            result.estimatedCycleSavings = 4;
            result.estimatedByteSavings = 6;
            return result;
        }
    }

    return result;
}

AddressTemplateDetector::MatchedPattern
AddressTemplateDetector::detectSimpleMultiplication(const BinaryOp& expr) const {
    MatchedPattern result;

    // Detect simple multiplication patterns that might be part of address calc
    // Pattern: a * WIDTH where WIDTH is a known width constant
    // Example: row * 40 (for text screen row addressing)

    if (expr.op != BinaryOp::Op::MUL) {
        return result;
    }

    int width_value = 0;
    std::string var_name;

    // Pattern: var * WIDTH
    if (isVariable(*expr.left, var_name) &&
        isConstant(*expr.right, width_value) &&
        isKnownWidth(width_value)) {

        result.type = PatternType::LINEAR_ROW_MAJOR;
        result.description = "Simple row multiply (var * " + std::to_string(width_value) + ")";
        result.operands = {var_name};
        result.constants = {width_value};
        result.canOptimize = true;
        result.estimatedCycleSavings = 8;
        result.estimatedByteSavings = 5;
        return result;
    }

    // Pattern: WIDTH * var (reversed)
    if (isConstant(*expr.left, width_value) &&
        isKnownWidth(width_value) &&
        isVariable(*expr.right, var_name)) {

        result.type = PatternType::LINEAR_ROW_MAJOR;
        result.description = "Simple row multiply (" + std::to_string(width_value) + " * var)";
        result.operands = {var_name};
        result.constants = {width_value};
        result.canOptimize = true;
        result.estimatedCycleSavings = 8;
        result.estimatedByteSavings = 5;
        return result;
    }

    return result;
}

AddressTemplateDetector::MatchedPattern
AddressTemplateDetector::detectCumulativeStride(const BinaryOp& expr) const {
    MatchedPattern result;

    // Pattern: a + (b * C1) + (c * C2)
    // Most complex; requires nested structure

    // For now, this is a placeholder for future implementation
    // Cumulative stride patterns are less common than row-major
    return result;
}

AddressTemplateDetector::MatchedPattern
AddressTemplateDetector::detectHardwarePattern(const BinaryOp& expr) const {
    MatchedPattern result;

    // Hardware-specific patterns for MEGA65
    // These are often used in system code and game engines

    // TODO: Implement detection for:
    // - VIC-IV register addressing
    // - SID addressing (multiply by 32 for SID offset)
    // - DMA descriptor patterns

    return result;
}

bool AddressTemplateDetector::isConstant(const Expression& expr, int& out_value) const {
    if (auto* intLit = dynamic_cast<const IntegerLiteral*>(&expr)) {
        out_value = intLit->value;
        return true;
    }
    return false;
}

bool AddressTemplateDetector::isVariable(const Expression& expr, std::string& out_name) const {
    if (auto* varRef = dynamic_cast<const VariableReference*>(&expr)) {
        out_name = varRef->name;
        return true;
    }
    return false;
}

const BinaryOp* AddressTemplateDetector::asBinaryOp(const Expression& expr) const {
    return dynamic_cast<const BinaryOp*>(&expr);
}

bool AddressTemplateDetector::validateOperands(const std::vector<std::string>& operands) const {
    // Verify operands are valid (non-empty names, no duplicates)
    if (operands.empty()) return false;

    for (const auto& op : operands) {
        if (op.empty()) return false;
    }

    // Check for duplicates
    std::vector<std::string> sorted_ops = operands;
    std::sort(sorted_ops.begin(), sorted_ops.end());
    for (size_t i = 1; i < sorted_ops.size(); i++) {
        if (sorted_ops[i] == sorted_ops[i-1]) return false;
    }

    return true;
}

bool AddressTemplateDetector::isKnownWidth(int width) const {
    // Common screen/bitmap widths
    static const int KNOWN_WIDTHS[] = {40, 80, 160, 320, 640};
    for (int w : KNOWN_WIDTHS) {
        if (width == w) return true;
    }
    return false;
}

bool AddressTemplateDetector::isKnownSize(int size) const {
    // Common sprite/offset sizes
    static const int KNOWN_SIZES[] = {3, 8, 16, 32, 64, 128, 256};
    for (int s : KNOWN_SIZES) {
        if (size == s) return true;
    }
    return false;
}

// Note: cycle/byte estimation is now in AddressTemplates.cpp within template metadata
