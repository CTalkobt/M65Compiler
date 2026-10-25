#include "AST.hpp"

void IntegerLiteral::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void StatementExpression::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ASTVisitor::visit(StatementExpression& node) {
    if (node.body) node.body->accept(*this);
    if (node.result) node.result->accept(*this);
}
void FloatLiteral::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void StringLiteral::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void VariableReference::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void Assignment::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void BinaryOperation::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void UnaryOperation::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void MemberAccess::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void GenericSelection::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void InitializerList::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ArrayAccess::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ConditionalExpression::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void CastExpression::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void CompoundLiteral::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void AlignofExpression::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void SizeofExpression::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void FunctionCall::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void VariableDeclaration::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ReturnStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void BreakStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ContinueStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void SwitchContinueStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void GotoStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void LabelledStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ExpressionStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void IfStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void WhileStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void DoWhileStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ForStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void RepeatStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void SwitchStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void CaseStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void DefaultStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void AsmStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void StaticAssert::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void EnumDefinition::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void StructDefinition::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void CompoundStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void FunctionDeclaration::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void BuiltinVaStart::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void BuiltinVaArg::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void CpuRegisterAccess::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void CpuFlagAccess::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void BitSelect::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void TranslationUnit::accept(ASTVisitor& visitor) { visitor.visit(*this); }

void LabelAddressExpression::accept(ASTVisitor& visitor) { visitor.visit(*this); }

// ASTWalker: every child, in source order

void ASTWalker::visit(IntegerLiteral&) {}
void ASTWalker::visit(FloatLiteral&) {}
void ASTWalker::visit(StringLiteral&) {}
void ASTWalker::visit(VariableReference&) {}
void ASTWalker::visit(Assignment& node) {
    if (node.target) node.target->accept(*this);
    if (node.expression) node.expression->accept(*this);
}
void ASTWalker::visit(BinaryOperation& node) {
    if (node.left) node.left->accept(*this);
    if (node.right) node.right->accept(*this);
}
void ASTWalker::visit(UnaryOperation& node) {
    if (node.operand) node.operand->accept(*this);
}
void ASTWalker::visit(ConditionalExpression& node) {
    if (node.condition) node.condition->accept(*this);
    if (node.thenExpr) node.thenExpr->accept(*this);
    if (node.elseExpr) node.elseExpr->accept(*this);
}
void ASTWalker::visit(GenericSelection& node) {
    if (node.control) node.control->accept(*this);
    for (auto& a : node.associations) if (a.result) a.result->accept(*this);
}
void ASTWalker::visit(InitializerList& node) {
    for (auto& e : node.elements) if (e) e->accept(*this);
}
void ASTWalker::visit(ArrayAccess& node) {
    if (node.arrayExpr) node.arrayExpr->accept(*this);
    if (node.indexExpr) node.indexExpr->accept(*this);
}
void ASTWalker::visit(FunctionCall& node) {
    if (node.callExpr) node.callExpr->accept(*this);
    for (auto& a : node.arguments) if (a) a->accept(*this);
}
void ASTWalker::visit(MemberAccess& node) {
    if (node.structExpr) node.structExpr->accept(*this);
}
void ASTWalker::visit(CastExpression& node) {
    if (node.expression) node.expression->accept(*this);
}
void ASTWalker::visit(CompoundLiteral& node) {
    if (node.initializer) node.initializer->accept(*this);
}
void ASTWalker::visit(AlignofExpression& node) {
    if (node.expression) node.expression->accept(*this);
}
void ASTWalker::visit(SizeofExpression& node) {
    if (!node.isType) if (node.expression) node.expression->accept(*this);
}
void ASTWalker::visit(VariableDeclaration& node) {
    if (node.alignmentExpr) node.alignmentExpr->accept(*this);
    for (auto& d : node.vlaDimExprs) if (d) d->accept(*this);
    if (node.initializer) node.initializer->accept(*this);
}
void ASTWalker::visit(ReturnStatement& node) {
    if (node.expression) node.expression->accept(*this);
}
void ASTWalker::visit(BreakStatement&) {}
void ASTWalker::visit(ContinueStatement&) {}
void ASTWalker::visit(SwitchContinueStatement& node) {
    if (node.target) node.target->accept(*this);
}
void ASTWalker::visit(GotoStatement& node) {
    if (node.target) node.target->accept(*this);
}
void ASTWalker::visit(LabelledStatement& node) {
    if (node.statement) node.statement->accept(*this);
}
void ASTWalker::visit(ExpressionStatement& node) {
    if (node.expression) node.expression->accept(*this);
}
void ASTWalker::visit(IfStatement& node) {
    if (node.condition) node.condition->accept(*this);
    if (node.thenBranch) node.thenBranch->accept(*this);
    if (node.elseBranch) node.elseBranch->accept(*this);
}
void ASTWalker::visit(WhileStatement& node) {
    if (node.condition) node.condition->accept(*this);
    if (node.body) node.body->accept(*this);
}
void ASTWalker::visit(DoWhileStatement& node) {
    if (node.body) node.body->accept(*this);
    if (node.condition) node.condition->accept(*this);
}
void ASTWalker::visit(ForStatement& node) {
    if (node.initializer) node.initializer->accept(*this);
    if (node.condition) node.condition->accept(*this);
    if (node.increment) node.increment->accept(*this);
    if (node.body) node.body->accept(*this);
}
void ASTWalker::visit(RepeatStatement& node) {
    if (node.body) node.body->accept(*this);
}
void ASTWalker::visit(SwitchStatement& node) {
    if (node.expression) node.expression->accept(*this);
    if (node.body) node.body->accept(*this);
}
void ASTWalker::visit(CaseStatement& node) {
    if (node.value) node.value->accept(*this);
    if (node.rangeEnd) node.rangeEnd->accept(*this);
}
void ASTWalker::visit(DefaultStatement&) {}
void ASTWalker::visit(AsmStatement&) {}
void ASTWalker::visit(StaticAssert& node) {
    if (node.condition) node.condition->accept(*this);
}
void ASTWalker::visit(EnumDefinition&) {}
void ASTWalker::visit(StructDefinition& node) {
    for (auto& m : node.methods) if (m) m->accept(*this);
}
void ASTWalker::visit(CompoundStatement& node) {
    for (auto& st : node.statements) if (st) st->accept(*this);
}
void ASTWalker::visit(FunctionDeclaration& node) {
    for (auto& p : node.parameters) {
        if (p.vlaSizeExpr) p.vlaSizeExpr->accept(*this);
        for (auto& d : p.innerDimExprs) if (d) d->accept(*this);
    }
    if (node.body) node.body->accept(*this);
}
void ASTWalker::visit(BuiltinVaStart& node) {
    if (node.ap) node.ap->accept(*this);
}
void ASTWalker::visit(BuiltinVaArg& node) {
    if (node.ap) node.ap->accept(*this);
}
void ASTWalker::visit(CpuRegisterAccess&) {}
void ASTWalker::visit(CpuFlagAccess&) {}
void ASTWalker::visit(BitSelect& node) {
    if (node.operand) node.operand->accept(*this);
}
void ASTWalker::visit(LabelAddressExpression&) {}
void ASTWalker::visit(TranslationUnit& node) {
    for (auto& d : node.topLevelDecls) if (d) d->accept(*this);
}
