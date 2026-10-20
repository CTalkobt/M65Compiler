#pragma once

#include "AST.hpp"
#include <map>
#include <set>
#include <string>
#include <vector>
#include <memory>

// Detects address calculation patterns for template optimization
// Identifies: row-major addressing, sprite offset tables, cumulative strides, hardware patterns
// Enables 30-50% code reduction for common patterns like (row * 40 + col)

class AddressTemplateDetector {
public:
    enum class PatternType {
        NONE = 0,
        LINEAR_ROW_MAJOR,      // (row * WIDTH) + col where WIDTH is 40, 80, 160, 320, 640
        SPRITE_OFFSET,         // base + (index * SIZE) where SIZE is 3, 8, 16, 32, 256
        CUMULATIVE_STRIDE,     // base + (x * X_STRIDE) + (y * Y_STRIDE)
        HARDWARE_PATTERN       // VIC-IV, SID, DMA patterns
    };

    struct MatchedPattern {
        PatternType type = PatternType::NONE;
        std::string name;                           // Template name (e.g., "text_screen_40")
        std::vector<std::string> operands;          // Variable names involved
        int multiplier = 0;                         // For SPRITE_OFFSET: the SIZE constant
        int width = 0;                              // For LINEAR_ROW_MAJOR: the WIDTH constant
        bool canOptimize = false;
        std::string templateName;                   // Hardcoded template identifier
        int estimatedBytesSaved = 0;                // Estimated code size reduction
    };

    struct PatternMatch {
        Expression* expr = nullptr;
        MatchedPattern pattern;
    };

    AddressTemplateDetector() = default;

    // Analyze a translation unit for address calculation patterns
    void analyzeTranslationUnit(TranslationUnit& unit);

    // Detect pattern in a binary operation (primary detection point)
    MatchedPattern detectPattern(const BinaryOperation& expr);

    // Detect pattern in any expression (recursive helper)
    MatchedPattern detectPatternInExpr(Expression* expr);

    // Get all detected patterns
    const std::vector<PatternMatch>& getDetectedPatterns() const { return detectedPatterns_; }

    // Get statistics
    struct Statistics {
        int totalPatterns = 0;
        int linearRowMajor = 0;
        int spriteOffset = 0;
        int cumulativeStride = 0;
        int hardwarePatterns = 0;
        int estimatedTotalBytesSaved = 0;
    };

    const Statistics& getStatistics() const { return stats_; }

private:
    // Pattern detection helpers
    MatchedPattern tryLinearRowMajor(Expression* multiply, Expression* add);
    MatchedPattern trySpriteOffset(Expression* base, Expression* multiply);
    MatchedPattern tryCumulativeStride(Expression* expr);

    // Helper: extract variable reference name
    std::string extractVarName(Expression* expr) const;

    // Helper: extract constant integer value
    bool extractConstantInt(Expression* expr, int& value) const;

    // Helper: check if width is a recognized row-major width (40, 80, 160, 320, 640)
    bool isRowMajorWidth(int width) const;

    // Helper: check if size is a recognized sprite offset size (3, 8, 16, 32, 256)
    bool isSpriteOffsetSize(int size) const;

    // Helper: estimate code size savings
    int estimateSavings(const MatchedPattern& pattern) const;

    std::vector<PatternMatch> detectedPatterns_;
    Statistics stats_;
};
