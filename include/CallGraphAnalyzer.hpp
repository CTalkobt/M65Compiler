#pragma once

#include "AST.hpp"
#include "FunctionAnalyzer.hpp"
#include <map>
#include <set>
#include <string>
#include <vector>

// Call graph analysis for cross-function optimization
// Tracks: function call relationships, call frequencies, optimization opportunities

class CallGraphAnalyzer : public ASTVisitor {
public:
    struct CallInfo {
        std::string caller;
        std::string callee;
        int callCount = 1;
        bool isVirtual = false;
        bool isRecursive = false;
    };

    struct FunctionNode {
        std::string name;
        std::vector<std::string> callees;      // Functions this function calls
        std::vector<std::string> callers;      // Functions that call this function
        int incomingCallCount = 0;             // Total number of incoming calls
        int outgoingCallCount = 0;             // Total number of outgoing calls
        bool isLeaf = true;                    // True if function makes no calls
        bool isRoot = false;                   // True if function is called from nowhere
        const FunctionAnalyzer::FunctionCharacteristics* characteristics = nullptr;
    };

    void analyzeTranslationUnit(TranslationUnit& unit, const FunctionAnalyzer& analyzer);

    // Query call graph
    const FunctionNode* getFunctionNode(const std::string& funcName) const;
    const std::map<std::string, FunctionNode>& getCallGraph() const { return callGraph_; }

    // Get all functions that can be reached from a given function
    std::set<std::string> getReachableFunctions(const std::string& funcName) const;

    // Get functions that call a given function
    std::set<std::string> getCallers(const std::string& funcName) const;

    // Find leaf functions (good candidates for inlining)
    std::vector<std::string> getLeafFunctions() const;

    // Find functions with single caller (good candidates for inlining)
    std::vector<std::string> getSimpleCalleeFunctions() const;

    // Detect virtual method calls and return potential implementations
    struct VirtualMethodInfo {
        std::string methodName;
        std::vector<std::string> implementations;
        bool hasSingleImplementation() const { return implementations.size() == 1; }
    };
    std::vector<VirtualMethodInfo> getVirtualMethods() const;

    // Check if two functions can be co-optimized (both safe to inline in each other's caller)
    bool canCoOptimize(const std::string& func1, const std::string& func2) const;

    // Standard visitor methods
    void visit(FunctionCall& node) override;
    void visit(FunctionDeclaration& node) override;
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
    void visit(StructDefinition& node) override {}
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
    std::map<std::string, FunctionNode> callGraph_;
    std::string currentFunction_;
    std::vector<VirtualMethodInfo> virtualMethods_;  // Collected virtual method info

    void buildCallGraph();
    void analyzeVirtualMethods();
    std::set<std::string> reachableFunctionsDFS(const std::string& funcName,
                                               std::set<std::string>& visited) const;
};
