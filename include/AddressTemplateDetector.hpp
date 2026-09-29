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

class AddressTemplateDetector : public ASTVisitor {
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

    // Visitor methods
    void visit(BinaryOperation& node) override {}
    void visit(Assignment& node) override {}
    void visit(ArrayAccess& node) override {}
    void visit(TranslationUnit& node) override {}
    void visit(FunctionDeclaration& node) override {}
    void visit(CompoundStatement& node) override {}

    // Forward to default visitor for other node types
    void visit(ForStatement& node) override {}
    void visit(WhileStatement& node) override {}
    void visit(DoWhileStatement& node) override {}
    void visit(IfStatement& node) override {}
    void visit(ExpressionStatement& node) override {}
    void visit(ReturnStatement& node) override {}
    void visit(BreakStatement& node) override {}
    void visit(ContinueStatement& node) override {}
    void visit(SwitchStatement& node) override {}
    void visit(CaseStatement& node) override {}
    void visit(DefaultStatement& node) override {}
    void visit(LabelledStatement& node) override {}
    void visit(GotoStatement& node) override {}
    void visit(SwitchContinueStatement& node) override {}
    void visit(RepeatStatement& node) override {}
    void visit(VariableDeclaration& node) override {}
    void visit(AsmStatement& node) override {}
    void visit(StaticAssert& node) override {}
    void visit(StructDefinition& node) override {}
    void visit(EnumDefinition& node) override {}
    void visit(IntegerLiteral& node) override {}
    void visit(FloatLiteral& node) override {}
    void visit(StringLiteral& node) override {}
    void visit(VariableReference& node) override {}
    void visit(MemberAccess& node) override {}
    void visit(FunctionCall& node) override {}
    void visit(UnaryOperation& node) override {}
    void visit(CastExpression& node) override {}
    void visit(SizeofExpression& node) override {}
    void visit(AlignofExpression& node) override {}
    void visit(ConditionalExpression& node) override {}
    void visit(InitializerList& node) override {}
    void visit(CompoundLiteral& node) override {}
    void visit(GenericSelection& node) override {}
    void visit(BuiltinVaStart& node) override {}
    void visit(BuiltinVaArg& node) override {}
    void visit(CpuRegisterAccess& node) override {}
    void visit(CpuFlagAccess& node) override {}
    void visit(LabelAddressExpression& node) override {}

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
