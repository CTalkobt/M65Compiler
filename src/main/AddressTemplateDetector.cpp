#include "AddressTemplateDetector.hpp"

AddressTemplateDetector::MatchedPattern
AddressTemplateDetector::detectPattern(const BinaryOperation& binOp) {
    // Look for patterns:
    // 1. (row * WIDTH) + col  →  LINEAR_ROW_MAJOR
    // 2. (base + idx * SIZE)  →  SPRITE_OFFSET
    // 3. base + (row*X) + (col*Y)  →  CUMULATIVE_STRIDE

    MatchedPattern result;

    // Pattern 1: (row * WIDTH) + col
    if (binOp.op == "+") {
        // Check if left is multiplication and right is variable
        if (auto multOp = dynamic_cast<BinaryOperation*>(binOp.left.get())) {
            if (multOp->op == "*") {
                int width;
                if (extractConstantInt(multOp->right.get(), width) && isRowMajorWidth(width)) {
                    result = tryLinearRowMajor(binOp.left.get(), binOp.right.get());
                    if (result.canOptimize) return result;
                }
            }
        }
        // Also check right side for multiplication
        if (auto multOp = dynamic_cast<BinaryOperation*>(binOp.right.get())) {
            if (multOp->op == "*") {
                int width;
                if (extractConstantInt(multOp->right.get(), width) && isRowMajorWidth(width)) {
                    result = tryLinearRowMajor(binOp.right.get(), binOp.left.get());
                    if (result.canOptimize) return result;
                }
            }
        }
    }

    // Pattern 2: base + (index * SIZE) for sprite offset
    if (binOp.op == "+") {
        if (auto multOp = dynamic_cast<BinaryOperation*>(binOp.right.get())) {
            if (multOp->op == "*") {
                int size;
                if (extractConstantInt(multOp->right.get(), size) && isSpriteOffsetSize(size)) {
                    result = trySpriteOffset(binOp.left.get(), binOp.right.get());
                    if (result.canOptimize) return result;
                }
            }
        }
    }

    return result;
}

AddressTemplateDetector::MatchedPattern
AddressTemplateDetector::tryLinearRowMajor(Expression* multiply, Expression* add) {
    MatchedPattern result;

    auto multOp = dynamic_cast<BinaryOperation*>(multiply);
    if (!multOp || multOp->op != "*") return result;

    int width;
    if (!extractConstantInt(multOp->right.get(), width)) return result;
    if (!isRowMajorWidth(width)) return result;

    std::string rowVar = extractVarName(multOp->left.get());
    std::string colVar = extractVarName(add);

    if (rowVar.empty() || colVar.empty()) return result;

    result.type = PatternType::LINEAR_ROW_MAJOR;
    result.width = width;
    result.operands = {rowVar, colVar};
    result.canOptimize = true;
    result.estimatedBytesSaved = (width == 40) ? 11 : 10;  // Depends on width

    return result;
}

AddressTemplateDetector::MatchedPattern
AddressTemplateDetector::trySpriteOffset(Expression* base, Expression* multiply) {
    MatchedPattern result;

    auto multOp = dynamic_cast<BinaryOperation*>(multiply);
    if (!multOp || multOp->op != "*") return result;

    int size;
    if (!extractConstantInt(multOp->right.get(), size)) return result;
    if (!isSpriteOffsetSize(size)) return result;

    std::string baseVar = extractVarName(base);
    std::string indexVar = extractVarName(multOp->left.get());

    if (baseVar.empty() || indexVar.empty()) return result;

    result.type = PatternType::SPRITE_OFFSET;
    result.multiplier = size;
    result.operands = {baseVar, indexVar};
    result.canOptimize = true;
    result.estimatedBytesSaved = 8;  // Typical saving

    return result;
}

AddressTemplateDetector::MatchedPattern
AddressTemplateDetector::tryCumulativeStride(Expression* expr) {
    MatchedPattern result;
    // TODO: Implement cumulative stride detection
    return result;
}

std::string AddressTemplateDetector::extractVarName(Expression* expr) const {
    if (auto varRef = dynamic_cast<VariableReference*>(expr)) {
        return varRef->name;
    }
    return "";
}

bool AddressTemplateDetector::extractConstantInt(Expression* expr, int& value) const {
    if (auto intLit = dynamic_cast<IntegerLiteral*>(expr)) {
        value = static_cast<int>(intLit->value);
        return true;
    }
    return false;
}

bool AddressTemplateDetector::isRowMajorWidth(int width) const {
    return width == 40 || width == 80 || width == 160 || width == 320 || width == 640;
}

bool AddressTemplateDetector::isSpriteOffsetSize(int size) const {
    return size == 3 || size == 8 || size == 16 || size == 32 || size == 256;
}

int AddressTemplateDetector::estimateSavings(const MatchedPattern& pattern) const {
    return pattern.estimatedBytesSaved;
}

void AddressTemplateDetector::analyzeTranslationUnit(TranslationUnit& unit) {}

AddressTemplateDetector::MatchedPattern
AddressTemplateDetector::detectPatternInExpr(Expression* expr) {
    return MatchedPattern();
}
