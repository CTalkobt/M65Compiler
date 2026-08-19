#pragma once

#include "AST.hpp"
#include "CallGraphAnalyzer.hpp"
#include <map>
#include <set>
#include <string>
#include <vector>

// Detects opportunities for virtual method devirtualization
// Identifies: single-implementation virtual methods, vtable analysis, direct call candidates

class DevirtualizationDetector : public ASTVisitor {
public:
    struct VirtualMethodInfo {
        std::string className;
        std::string methodName;
        std::vector<std::string> implementations;
        int callSiteCount = 0;
        bool canDevirtualize = false;
    };

    struct VtableInfo {
        std::string className;
        std::vector<std::string> methods;
        std::vector<int> methodCounts;  // How many implementations per method
    };

    void analyzeTranslationUnit(TranslationUnit& unit, const CallGraphAnalyzer& callGraph);

    // Get all detectable virtual methods
    const std::vector<VirtualMethodInfo>& getVirtualMethods() const { return virtualMethods_; }

    // Get vtable information
    const std::map<std::string, VtableInfo>& getVtables() const { return vtables_; }

    // Get devirtualizable methods (single implementation)
    std::vector<VirtualMethodInfo> getDevirtualizableMethods() const;

    // Check if a specific virtual call can be devirtualized
    bool canDevirtualize(const std::string& className, const std::string& methodName) const;

    // Standard visitor methods
    void visit(StructDefinition& node) override;
    void visit(FunctionDeclaration& node) override;
    void visit(FunctionCall& node) override;
    void visit(TranslationUnit& node) override;

    // Forward to default visitor for other node types
    void visit(ForStatement& node) override {}
    void visit(WhileStatement& node) override {}
    void visit(DoWhileStatement& node) override {}
    void visit(IfStatement& node) override {}
    void visit(CompoundStatement& node) override {}
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
    void visit(EnumDefinition& node) override {}

    void visit(IntegerLiteral&) override {}
    void visit(FloatLiteral&) override {}
    void visit(StringLiteral&) override {}
    void visit(VariableReference&) override {}
    void visit(ArrayAccess&) override {}
    void visit(MemberAccess&) override {}
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
    std::vector<VirtualMethodInfo> virtualMethods_;
    std::map<std::string, VtableInfo> vtables_;
    std::map<std::string, int> virtualCallCounts_;

    void analyzeStructForVirtualMethods(StructDefinition& structDef);
    void recordVirtualMethod(const std::string& className, const std::string& methodName,
                            const std::string& implementation);
};
