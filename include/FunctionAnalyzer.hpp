#pragma once

#include "AST.hpp"
#include <map>
#include <set>
#include <string>

// Simple function characteristic analyzer (no recursion detection)
// Detects: loop count, branch count, code metrics
// Computes: optimization suitability flags for unrolling, interchange, constant folding, SAC

class FunctionAnalyzer : public ASTVisitor {
public:
    struct FunctionCharacteristics {
        std::string name;
        int parameterCount = 0;
        int loopCount = 0;
        int branchCount = 0;
        int maxLoopNestingDepth = 0;
        int estimatedCodeSize = 0;
        bool isLeaf = true;  // Updated if any function calls found

        bool shouldUnrollLoops = false;
        bool shouldInterchangeLoops = false;
        bool shouldFoldConstants2x = false;
        bool shouldUseSAC = false;
    };

    void analyzeTranslationUnit(TranslationUnit& unit);
    const FunctionCharacteristics* getCharacteristics(const std::string& funcName) const;

    // Standard visitor methods
    void visit(ForStatement& node) override;
    void visit(WhileStatement& node) override;
    void visit(DoWhileStatement& node) override;
    void visit(IfStatement& node) override;
    void visit(CompoundStatement& node) override;
    void visit(ExpressionStatement& node) override;
    void visit(ReturnStatement& node) override;
    void visit(BreakStatement& node) override;
    void visit(ContinueStatement& node) override;
    void visit(SwitchStatement& node) override;
    void visit(CaseStatement& node) override;
    void visit(DefaultStatement& node) override;
    void visit(LabelledStatement& node) override;
    void visit(GotoStatement& node) override;
    void visit(SwitchContinueStatement& node) override;
    void visit(RepeatStatement& node) override;
    void visit(VariableDeclaration& node) override;
    void visit(FunctionDeclaration& node) override;
    void visit(AsmStatement& node) override;
    void visit(StaticAssert& node) override;
    void visit(StructDefinition& node) override;
    void visit(EnumDefinition& node) override;
    void visit(TranslationUnit& node) override;

    // Expression visitors
    void visit(IntegerLiteral&) override {}
    void visit(FloatLiteral&) override {}
    void visit(StringLiteral&) override {}
    void visit(VariableReference&) override {}
    void visit(ArrayAccess&) override {}
    void visit(MemberAccess&) override {}
    void visit(FunctionCall& node) override;
    void visit(BinaryOperation&) override {}
    void visit(UnaryOperation&) override {}
    void visit(CastExpression&) override {}
    void visit(SizeofExpression&) override {}
    void visit(AlignofExpression&) override {}
    void visit(ConditionalExpression&) override {}
    void visit(Assignment&) override {}
    void visit(InitializerList&) override {}
    void visit(CompoundLiteral&) override {}
    void visit(GenericSelection&) override {}
    void visit(BuiltinVaStart&) override {}
    void visit(BuiltinVaArg&) override {}
    void visit(CpuRegisterAccess&) override {}
    void visit(CpuFlagAccess&) override {}
    void visit(LabelAddressExpression&) override {}

private:
    struct AnalysisState {
        FunctionCharacteristics* current = nullptr;
        int currentLoopDepth = 0;
    } state_;

    std::map<std::string, FunctionCharacteristics> characteristics_;

    void analyzeFunction(FunctionDeclaration* func);
    void computeOptimizationFlags();
};
