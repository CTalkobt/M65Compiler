#pragma once
#include <string>
#include <vector>
#include <memory>

class ASTVisitor;
class FunctionDeclaration; // forward declaration for StructDefinition::methods

// Signature info for function pointer types
struct FuncPtrSignature {
    std::string returnType;
    int returnPointerLevel = 0;
    bool returnIsSigned = false;
    struct Param {
        std::string type;
        int pointerLevel = 0;
        bool isSigned = false;
    };
    std::vector<Param> params;
    bool isVariadic = false;   // (..., ...): called in the stack convention
    bool prototyped = true;    // false for (): any arguments (C17 6.7.6.3p14)
};

class ASTNode {
public:
    int line = 0;
    int column = 0;
    std::string sourceFile;  // Source file from #line directive
    virtual ~ASTNode() = default;
    virtual void accept(ASTVisitor& visitor) = 0;
};

class Expression : public ASTNode {
public:
    // __likely(x) / __unlikely(x) / __builtin_expect(x, c): +1 when the
    // expression is expected true, -1 when expected false (else 0)
    signed char branchHint = 0;
};
class Statement : public ASTNode {};

class IntegerLiteral : public Expression {
public:
    int64_t value;
    // Optional type from folded CastExpression (empty = default "int")
    std::string castType;
    int castPointerLevel = 0;
    bool castIsSigned = false;
    IntegerLiteral(int64_t v) : value(v) {}
    void accept(ASTVisitor& visitor) override;
};

class FloatLiteral : public Expression {
public:
    double value;
    FloatLiteral(double v) : value(v) {}
    void accept(ASTVisitor& visitor) override;
};

class StringLiteral : public Expression {
public:
    std::string value;
    bool isAscii = false;  // @"..." prefix — emit .ascii instead of .text
    std::string escMask;   // per byte: '.', 'n', 'r', 'x' (see Token::escMask)
    bool isWide = false;   // L"...": an array of 16-bit wchar_t
    // Encoding prefix: 0 p"..." (PETSCII), 2 s"..." (screen codes), -1 none
    // (isAscii, else #pragma encoding). Overrides the pragma.
    int prefixEncoding = -1;
    // #pragma cc45 noescape_strings (not __escape "..."): its address stays
    // in the object's code, so in an overlay it is kept in the overlay
    bool noescape = false;
    StringLiteral(const std::string& v, bool ascii = false) : value(v), isAscii(ascii) {}
    static std::unique_ptr<StringLiteral> copyOf(const StringLiteral& s) {
        auto c = std::make_unique<StringLiteral>(s.value, s.isAscii);
        c->escMask = s.escMask;
        c->isWide = s.isWide;
        c->prefixEncoding = s.prefixEncoding;
        c->noescape = s.noescape;
        return c;
    }
    void accept(ASTVisitor& visitor) override;
};

class VariableReference : public Expression {
public:
    std::string name;
    // Phase 91.3.4: Specialization tracking
    bool isConstantSpecialized = false;
    long specializedConstantValue = 0;
    VariableReference(const std::string& n) : name(n) {}
    void accept(ASTVisitor& visitor) override;
};

class FunctionCall : public Expression {
public:
    std::string name;
    std::vector<std::unique_ptr<Expression>> arguments;
    std::unique_ptr<Expression> callExpr; // non-null for indirect calls: (*fp)(args) or fp(args)
    FunctionCall(const std::string& n) : name(n) {}
    void accept(ASTVisitor& visitor) override;
};

class Assignment : public Expression {
public:
    std::string op;
    std::unique_ptr<Expression> target;
    std::unique_ptr<Expression> expression;
    Assignment(std::unique_ptr<Expression> t, std::unique_ptr<Expression> e, const std::string& o = "=") 
        : op(o), target(std::move(t)), expression(std::move(e)) {}
    void accept(ASTVisitor& visitor) override;
};

class BinaryOperation : public Expression {
public:
    std::string op;
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;
    BinaryOperation(const std::string& o, std::unique_ptr<Expression> l, std::unique_ptr<Expression> r) 
        : op(o), left(std::move(l)), right(std::move(r)) {}
    void accept(ASTVisitor& visitor) override;
};

class UnaryOperation : public Expression {
public:
    std::string op;
    std::unique_ptr<Expression> operand;
    UnaryOperation(const std::string& o, std::unique_ptr<Expression> opnd)
        : op(o), operand(std::move(opnd)) {}
    void accept(ASTVisitor& visitor) override;
};

class MemberAccess : public Expression {
public:
    std::unique_ptr<Expression> structExpr;
    std::string memberName;
    bool isArrow;
    MemberAccess(std::unique_ptr<Expression> s, const std::string& m, bool arrow) 
        : structExpr(std::move(s)), memberName(m), isArrow(arrow) {}
    void accept(ASTVisitor& visitor) override;
};

struct GenericAssociation {
    std::string typeName;
    int pointerLevel = 0;
    std::unique_ptr<Expression> result;
    bool isDefault = false;
};

class GenericSelection : public Expression {
public:
    std::unique_ptr<Expression> control;
    std::vector<GenericAssociation> associations;
    GenericSelection(std::unique_ptr<Expression> c) : control(std::move(c)) {}
    void accept(ASTVisitor& visitor) override;
};

class InitializerList : public Expression {
public:
    struct Designator {
        std::string memberName;  // non-empty for .member designator
        int arrayIndex = -1;     // >= 0 for [index] designator
        bool isDesignated() const { return !memberName.empty() || arrayIndex >= 0; }
    };
    std::vector<std::unique_ptr<Expression>> elements;
    std::vector<Designator> designators; // parallel to elements; may be empty or shorter
    InitializerList() {}
    void accept(ASTVisitor& visitor) override;
    Designator getDesignator(size_t i) const {
        return (i < designators.size()) ? designators[i] : Designator{};
    }
};

class ArrayAccess : public Expression {
public:
    std::unique_ptr<Expression> arrayExpr;
    std::unique_ptr<Expression> indexExpr;
    ArrayAccess(std::unique_ptr<Expression> a, std::unique_ptr<Expression> i)
        : arrayExpr(std::move(a)), indexExpr(std::move(i)) {}
    void accept(ASTVisitor& visitor) override;
};

class ConditionalExpression : public Expression {
public:
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Expression> thenExpr;
    std::unique_ptr<Expression> elseExpr;
    ConditionalExpression(std::unique_ptr<Expression> c, std::unique_ptr<Expression> t, std::unique_ptr<Expression> e)
        : condition(std::move(c)), thenExpr(std::move(t)), elseExpr(std::move(e)) {}
    void accept(ASTVisitor& visitor) override;
};

class CastExpression : public Expression {
public:
    std::string targetType;
    int pointerLevel;
    bool isSigned = false;
    std::unique_ptr<Expression> expression;
    CastExpression(const std::string& t, int p, bool s, std::unique_ptr<Expression> e)
        : targetType(t), pointerLevel(p), isSigned(s), expression(std::move(e)) {}
    void accept(ASTVisitor& visitor) override;
};

class CompoundLiteral : public Expression {
public:
    std::string targetType;
    int pointerLevel = 0;
    bool isSigned = false;
    std::vector<int> arrayDims; // empty = scalar/struct; e.g. {3} for (int[]){1,2,3}
    std::unique_ptr<InitializerList> initializer;
    int tempId = 0; // unique ID for frame slot allocation
    CompoundLiteral(const std::string& t, int p, bool s, std::unique_ptr<InitializerList> init)
        : targetType(t), pointerLevel(p), isSigned(s), initializer(std::move(init)) {}
    void accept(ASTVisitor& visitor) override;
};

class AlignofExpression : public Expression {
public:
    std::string typeName;
    int pointerLevel;
    bool isSigned = false;
    std::unique_ptr<Expression> expression;  // __alignof__(expr) (typeName empty)
    AlignofExpression(const std::string& t, int p = 0) : typeName(t), pointerLevel(p) {}
    void accept(ASTVisitor& visitor) override;
};

class VariableDeclaration : public Statement {
public:
    std::string type;
    int pointerLevel;
    bool isSigned = false;
    // a plain char/short/int/long (no signed/unsigned, not a typedef) that is
    // unsigned only by cc45's default: storing a negative constant warns
    bool isPlainUnsigned = false;
    std::string name;
    bool isVolatile = false;
    bool isConst = false;        // const on the base type (pointed-to data is const)
    bool isPointerConst = false; // const on the pointer itself (pointer variable is const)
    // const on the pointers it points to: bit k for the one k dereferences
    // away (T *const *p: bit 1, *p is const; #334)
    unsigned innerConstMask = 0;
    bool isGlobal = false;
    bool isExtern = false;
    bool isStatic = false;
    bool isRegister = false;
    bool isStriped = false;         // Phase 92: Striped array optimization
    int alignment = 0;
    std::unique_ptr<Expression> alignmentExpr;
    std::unique_ptr<Expression> initializer;
    std::vector<int> arrayDims; // empty means not an array; e.g. {3,4} for int a[3][4]
    // A variable-length array (int a[n][4]): the expression of each run-time
    // dimension (its arrayDims entry is 0); empty otherwise
    std::vector<std::shared_ptr<Expression>> vlaDimExprs;
    std::vector<int> ptrArrayDims;   // T (*p)[N][M]: the pointed-to array's dims
    int arraySize() const { if (arrayDims.empty()) return -1; int s=1; for (int d:arrayDims) s*=d; return s; }
    bool isFunctionPointer = false;
    std::shared_ptr<FuncPtrSignature> funcPtrSig; // non-null when isFunctionPointer
    int addressSpace = 0;  // Phase 97: Address space (0=DEFAULT, 1=ZP, 2=ABS, 3=FAR)
    // __noescape (__attribute__((noescape))): a constant whose address
    // is used only by its object's code: kept with that code (an overlay's)
    bool isNoescape = false;
    // __section("NAME") (Issue #70): placed in the section NAME (ln45 gives
    // its address; #pragma section declares it)
    std::string section;
    VariableDeclaration(const std::string& t, const std::string& n, int p = 0) : type(t), pointerLevel(p), name(n) {}
    void accept(ASTVisitor& visitor) override;
};

class ReturnStatement : public Statement {
public:
    std::unique_ptr<Expression> expression;
    ReturnStatement(std::unique_ptr<Expression> e) : expression(std::move(e)) {}
    void accept(ASTVisitor& visitor) override;
};

class IfStatement : public Statement {
public:
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> thenBranch;
    std::unique_ptr<Statement> elseBranch;
    int hint = 0;       // +1: the condition is expected true, -1: false (__likely/__unlikely)
    IfStatement(std::unique_ptr<Expression> c, std::unique_ptr<Statement> t, std::unique_ptr<Statement> e = nullptr)
        : condition(std::move(c)), thenBranch(std::move(t)), elseBranch(std::move(e)) {}
    void accept(ASTVisitor& visitor) override;
};

class BreakStatement : public Statement {
public:
    int levels = 1;     // break N: leave N enclosing loops/switches
    BreakStatement(int n = 1) : levels(n) {}
    void accept(ASTVisitor& visitor) override;
};

class ContinueStatement : public Statement {
public:
    void accept(ASTVisitor& visitor) override;
};

class SwitchContinueStatement : public Statement {
public:
    std::unique_ptr<Expression> target; // nullptr means 'default'
    SwitchContinueStatement(std::unique_ptr<Expression> t) : target(std::move(t)) {}
    void accept(ASTVisitor& visitor) override;
};

class GotoStatement : public Statement {
public:
    std::string label;
    std::unique_ptr<Expression> target; // GCC extension: goto *expr
    GotoStatement(const std::string& l, std::unique_ptr<Expression> t = nullptr) : label(l), target(std::move(t)) {}
    void accept(ASTVisitor& visitor) override;
};

class LabelAddressExpression : public Expression {
public:
    std::string label;
    LabelAddressExpression(const std::string& l) : label(l) {}
    void accept(ASTVisitor& visitor) override;
};

class LabelledStatement : public Statement {
public:
    std::string label;
    std::unique_ptr<Statement> statement;
    LabelledStatement(const std::string& l, std::unique_ptr<Statement> s) : label(l), statement(std::move(s)) {}
    void accept(ASTVisitor& visitor) override;
};


class SizeofExpression : public Expression {
public:
    std::unique_ptr<Expression> expression; // For sizeof expr
    std::string typeName;                   // For sizeof(type)
    int pointerLevel = 0;
    bool isType = false;
    std::vector<int> arrayDims;             // of a typedef'd array type (pointerLevel 0)
    std::vector<int> declDims;              // sizeof(T *[3][4]): the type is an array of them
    SizeofExpression(std::unique_ptr<Expression> e) : expression(std::move(e)), isType(false) {}
    SizeofExpression(const std::string& t, int p = 0) : typeName(t), pointerLevel(p), isType(true) {}
    void accept(ASTVisitor& visitor) override;
};

class ExpressionStatement : public Statement {
public:
    std::unique_ptr<Expression> expression;
    ExpressionStatement(std::unique_ptr<Expression> e) : expression(std::move(e)) {}
    void accept(ASTVisitor& visitor) override;
};

class WhileStatement : public Statement {
public:
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> body;
    WhileStatement(std::unique_ptr<Expression> c, std::unique_ptr<Statement> b)
        : condition(std::move(c)), body(std::move(b)) {}
    void accept(ASTVisitor& visitor) override;
};

class DoWhileStatement : public Statement {
public:
    std::unique_ptr<Statement> body;
    std::unique_ptr<Expression> condition;
    DoWhileStatement(std::unique_ptr<Statement> b, std::unique_ptr<Expression> c)
        : body(std::move(b)), condition(std::move(c)) {}
    void accept(ASTVisitor& visitor) override;
};

class ForStatement : public Statement {
public:
    std::unique_ptr<Statement> initializer;
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Expression> increment;
    std::unique_ptr<Statement> body;
    ForStatement(std::unique_ptr<Statement> init, std::unique_ptr<Expression> cond, std::unique_ptr<Expression> inc, std::unique_ptr<Statement> body)
        : initializer(std::move(init)), condition(std::move(cond)), increment(std::move(inc)), body(std::move(body)) {}
    void accept(ASTVisitor& visitor) override;
};

class RepeatStatement : public Statement {
public:
    int count;                              // compile-time repeat count
    std::string varName;                    // optional loop variable name (empty if none)
    std::string varType;                    // optional loop variable type (e.g., "char")
    bool varSigned = false;
    std::unique_ptr<Statement> body;
    RepeatStatement(int n, std::unique_ptr<Statement> b)
        : count(n), body(std::move(b)) {}
    void accept(ASTVisitor& visitor) override;
};

class SwitchStatement : public Statement {
public:
    std::unique_ptr<Expression> expression;
    std::unique_ptr<Statement> body;
    SwitchStatement(std::unique_ptr<Expression> expr, std::unique_ptr<Statement> body)
        : expression(std::move(expr)), body(std::move(body)) {}
    void accept(ASTVisitor& visitor) override;
};

class CaseStatement : public Statement {
public:
    std::unique_ptr<Expression> value;
    std::unique_ptr<Expression> rangeEnd;  // non-null for case ranges (e.g., case 'A' ... 'Z':)
    std::string label;
    CaseStatement(std::unique_ptr<Expression> val, std::unique_ptr<Expression> end = nullptr)
        : value(std::move(val)), rangeEnd(std::move(end)) {}
    void accept(ASTVisitor& visitor) override;
};

class DefaultStatement : public Statement {
public:
    std::string label;
    void accept(ASTVisitor& visitor) override;
};

class AsmStatement : public Statement {

public:
    std::string code;
    AsmStatement(const std::string& c) : code(c) {}
    void accept(ASTVisitor& visitor) override;
};

class StaticAssert : public Statement {
public:
    std::unique_ptr<Expression> condition;
    std::string message;
    StaticAssert(std::unique_ptr<Expression> c, const std::string& m)
        : condition(std::move(c)), message(m) {}
    void accept(ASTVisitor& visitor) override;
};

class CompoundStatement : public Statement {
public:
    std::vector<std::unique_ptr<Statement>> statements;
    // The declarators of one declaration (int a, b[n];), not a block
    bool isDeclGroup = false;
    void accept(ASTVisitor& visitor) override;
};

// GCC statement expression ({ stmt; ...; expr; }): the statements run in
// order, then `result` (null when the block doesn't end in an expression
// statement) is the value
class StatementExpression : public Expression {
public:
    std::unique_ptr<CompoundStatement> body;
    std::unique_ptr<Expression> result;
    void accept(ASTVisitor& visitor) override;
};

struct StructMember {
    std::string type;
    int pointerLevel;
    bool isSigned = false;
    std::string name;
    bool isConst = false;
    int alignment = 0;
    std::unique_ptr<Expression> alignmentExpr;
    bool isAnonymous = false;
    std::vector<int> arrayDims; // empty means not an array
    bool isFlexible = false;    // name[] (a zero-length name[0] is a GNU extension)
    // Run-time array dimensions (char x[n] in a struct defined in a function):
    // parallel to arrayDims (whose entry is then 1), null for a constant one
    std::vector<std::unique_ptr<Expression>> vlaDimExprs;
    int arraySize() const { if (arrayDims.empty()) return -1; int s=1; for (int d:arrayDims) s*=d; return s; }
    int bitWidth = 0; // 0 = not a bitfield; >0 = bitfield width in bits
    std::shared_ptr<FuncPtrSignature> funcPtrSig;   // a function pointer member's signature
};

class EnumDefinition : public Statement {
public:
    std::string name;
    std::vector<std::pair<std::string, int>> enumerators;
    EnumDefinition(const std::string& n) : name(n) {}
    void accept(ASTVisitor& visitor) override;
};

class StructDefinition : public Statement {
public:
    std::string name;
    bool isUnion = false;
    bool isUnpacked = false;  // __unpacked — opt-in to alignment padding
    std::vector<StructMember> members;
    std::vector<std::unique_ptr<FunctionDeclaration>> methods; // struct methods (Phase 2 OOP)
    std::string parentStruct;  // Phase 3: single inheritance (empty = no parent)
    bool hasVirtual = false;   // Phase 3: true if any method is virtual
    bool isFinal = false;      // Phase 4: struct cannot be inherited from
    int alignment = 0;         // __attribute__((aligned(N))) on the struct type
    StructDefinition(const std::string& n, bool isUnion = false) : name(n), isUnion(isUnion) {}
    void accept(ASTVisitor& visitor) override;
};

struct Parameter {
    std::string type;
    int pointerLevel;
    bool isSigned = false;
    std::string name;
    bool isVolatile = false;
    bool isConst = false;
    bool isPointerConst = false;
    bool isFunctionPointer = false;
    std::shared_ptr<FuncPtrSignature> funcPtrSig;
    // int a[n++]: a non-constant array size, evaluated on entry for its side
    // effects (C11 6.9.1p10); the parameter itself is a pointer
    std::shared_ptr<Expression> vlaSizeExpr;
    // int a[][M][N]: the dimensions after the first ({M, N}); a run-time
    // one (char a[2][n]) is 0 here with its expression in innerDimExprs
    std::vector<int> innerArrayDims;
    std::vector<std::shared_ptr<Expression>> innerDimExprs;
    std::string sourceFile;  // position of the parameter name (0 line = unknown)
    int line = 0;
    int column = 0;
};

class FunctionDeclaration : public Statement {
public:
    std::string name;
    std::string returnType;
    int returnPointerLevel = 0;
    bool isSigned = false;
    std::vector<Parameter> parameters;
    std::unique_ptr<CompoundStatement> body;
    bool isNoreturn = false;
    bool isPrototype = false;
    bool isStatic = false;
    bool isVariadic = false;
    bool isFastcall = false;
    bool isInterrupt = false;
    bool isNaked = false;
    bool isRegparm = false;
    bool isFar = false;           // __far: kept in far memory, run through the mapped window (ln45)
    bool isInline = false;
    bool isAlwaysInline = false;  // __attribute__((always_inline)) / __forceinline
    bool isNoInline = false;      // __attribute__((noinline))
    int alignment = 0;            // __attribute__((aligned(N))): code address alignment
    // __pure / __attribute__((pure)): 1, no side effects (may read memory);
    // __attribute__((const)): 2, the result depends only on the arguments
    int purity = 0;

    // Nested function support
    bool isNested = false;
    FunctionDeclaration* parentFunc = nullptr;

    // Struct method support (Phase 2/3 OOP)
    bool isMethod = false;
    std::string methodStructName; // "Point" for struct Point methods
    bool isVirtual = false;       // Phase 3: virtual method (dispatched via vtable)
    bool isFinal = false;         // Phase 4: method cannot be overridden → direct call
    bool isOperator = false;      // Operator overload method
    int vtableSlot = -1;          // Phase 3: slot index in vtable (-1 = not virtual)

    // Phase 82: Per-function optimization selection
    bool optimizeLoopUnroll = true;
    bool optimizeLoopInterchange = true;
    bool optimize2PassFolding = false;
    bool optimizeSAC = true;
    int unrollFactor = 4;

    FunctionDeclaration(const std::string& n, const std::string& rt) : name(n), returnType(rt) {}
    void accept(ASTVisitor& visitor) override;
};

// __builtin_va_start(ap, last_named_param)
class BuiltinVaStart : public Expression {
public:
    std::unique_ptr<Expression> ap;        // the va_list variable (lvalue)
    std::string lastParamName;             // name of the last named parameter
    BuiltinVaStart(std::unique_ptr<Expression> a, const std::string& lp)
        : ap(std::move(a)), lastParamName(lp) {}
    void accept(ASTVisitor& visitor) override;
};

// __builtin_va_arg(ap, type) — always returns int-width (2 bytes) per default promotions
class BuiltinVaArg : public Expression {
public:
    std::unique_ptr<Expression> ap;        // the va_list variable (lvalue)
    std::string typeName;                  // the type requested
    int pointerLevel = 0;
    bool isSigned = false;
    BuiltinVaArg(std::unique_ptr<Expression> a, const std::string& t, int p = 0, bool s = false)
        : ap(std::move(a)), typeName(t), pointerLevel(p), isSigned(s) {}
    void accept(ASTVisitor& visitor) override;
};


// __cpu.A, __cpu.X, etc.
class CpuRegisterAccess : public Expression {
public:
    std::string regName;
    CpuRegisterAccess(const std::string& r) : regName(r) {}
    void accept(ASTVisitor& visitor) override;
};

// __flags.Carry, __flags.Zero, etc.
class CpuFlagAccess : public Expression {
public:
    std::string flagName;
    CpuFlagAccess(const std::string& f) : flagName(f) {}
    void accept(ASTVisitor& visitor) override;
};

// __bit(v, n) / __bits(v, lo, hi) (#204): bits lo..hi of the integer lvalue
// v, an unsigned value (and an lvalue: v is read once and written once)
class BitSelect : public Expression {
public:
    std::unique_ptr<Expression> operand;
    int lo, hi;
    BitSelect(std::unique_ptr<Expression> v, int l, int h) : operand(std::move(v)), lo(l), hi(h) {}
    void accept(ASTVisitor& visitor) override;
};


class TranslationUnit : public ASTNode {
public:
    std::vector<std::unique_ptr<Statement>> topLevelDecls;
    void accept(ASTVisitor& visitor) override;
};


class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;
    virtual void visit(IntegerLiteral& node) = 0;
    virtual void visit(FloatLiteral& node) = 0;
    virtual void visit(StringLiteral& node) = 0;
    virtual void visit(VariableReference& node) = 0;
    virtual void visit(Assignment& node) = 0;
    virtual void visit(BinaryOperation& node) = 0;
    virtual void visit(UnaryOperation& node) = 0;
    virtual void visit(ConditionalExpression& node) = 0;
    virtual void visit(GenericSelection& node) = 0;
    virtual void visit(InitializerList& node) = 0;
    virtual void visit(ArrayAccess& node) = 0;
    virtual void visit(FunctionCall& node) = 0;
    virtual void visit(MemberAccess& node) = 0;
    virtual void visit(CastExpression& node) = 0;
    virtual void visit(CompoundLiteral& node) = 0;
    virtual void visit(AlignofExpression& node) = 0;
    virtual void visit(SizeofExpression& node) = 0;
    virtual void visit(VariableDeclaration& node) = 0;
    virtual void visit(ReturnStatement& node) = 0;
    virtual void visit(BreakStatement& node) = 0;
    virtual void visit(ContinueStatement& node) = 0;
    virtual void visit(SwitchContinueStatement& node) = 0;
    virtual void visit(GotoStatement& node) = 0;
    virtual void visit(LabelledStatement& node) = 0;
    virtual void visit(ExpressionStatement& node) = 0;
    virtual void visit(IfStatement& node) = 0;
    virtual void visit(WhileStatement& node) = 0;
    virtual void visit(DoWhileStatement& node) = 0;
    virtual void visit(ForStatement& node) = 0;
    virtual void visit(RepeatStatement& node) = 0;
    virtual void visit(SwitchStatement& node) = 0;
    virtual void visit(CaseStatement& node) = 0;
    virtual void visit(DefaultStatement& node) = 0;
    virtual void visit(AsmStatement& node) = 0;
    virtual void visit(StaticAssert& node) = 0;
    virtual void visit(EnumDefinition& node) = 0;
    virtual void visit(StructDefinition& node) = 0;
    virtual void visit(CompoundStatement& node) = 0;
    virtual void visit(FunctionDeclaration& node) = 0;
    virtual void visit(BuiltinVaStart& node) = 0;
    virtual void visit(BuiltinVaArg& node) = 0;
    virtual void visit(CpuRegisterAccess& node) = 0;
    virtual void visit(CpuFlagAccess& node) = 0;
    virtual void visit(BitSelect& node) = 0;
    virtual void visit(LabelAddressExpression& node) = 0;
    virtual void visit(TranslationUnit& node) = 0;
    // Default: visit the statements, then the result
    virtual void visit(StatementExpression& node);
};

// A visitor that walks into every child node (expressions and statements,
// in source order) and does nothing else: a pass derives from it and
// overrides only the nodes it acts on, calling ASTWalker::visit(node) to
// keep walking below them (or not, to stop there)
class ASTWalker : public ASTVisitor {
public:
    using ASTVisitor::visit;
    void visit(IntegerLiteral& node) override;
    void visit(FloatLiteral& node) override;
    void visit(StringLiteral& node) override;
    void visit(VariableReference& node) override;
    void visit(Assignment& node) override;
    void visit(BinaryOperation& node) override;
    void visit(UnaryOperation& node) override;
    void visit(ConditionalExpression& node) override;
    void visit(GenericSelection& node) override;
    void visit(InitializerList& node) override;
    void visit(ArrayAccess& node) override;
    void visit(FunctionCall& node) override;
    void visit(MemberAccess& node) override;
    void visit(CastExpression& node) override;
    void visit(CompoundLiteral& node) override;
    void visit(AlignofExpression& node) override;
    void visit(SizeofExpression& node) override;
    void visit(VariableDeclaration& node) override;
    void visit(ReturnStatement& node) override;
    void visit(BreakStatement& node) override;
    void visit(ContinueStatement& node) override;
    void visit(SwitchContinueStatement& node) override;
    void visit(GotoStatement& node) override;
    void visit(LabelledStatement& node) override;
    void visit(ExpressionStatement& node) override;
    void visit(IfStatement& node) override;
    void visit(WhileStatement& node) override;
    void visit(DoWhileStatement& node) override;
    void visit(ForStatement& node) override;
    void visit(RepeatStatement& node) override;
    void visit(SwitchStatement& node) override;
    void visit(CaseStatement& node) override;
    void visit(DefaultStatement& node) override;
    void visit(AsmStatement& node) override;
    void visit(StaticAssert& node) override;
    void visit(EnumDefinition& node) override;
    void visit(StructDefinition& node) override;
    void visit(CompoundStatement& node) override;
    void visit(FunctionDeclaration& node) override;
    void visit(BuiltinVaStart& node) override;
    void visit(BuiltinVaArg& node) override;
    void visit(CpuRegisterAccess& node) override;
    void visit(CpuFlagAccess& node) override;
    void visit(BitSelect& node) override;
    void visit(LabelAddressExpression& node) override;
    void visit(TranslationUnit& node) override;
};
