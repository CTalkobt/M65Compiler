#pragma once
#include <vector>
#include <memory>
#include <map>
#include <optional>
#include <set>
#include "Token.hpp"
#include "AST.hpp"
#include "ConstEval.hpp"

// (the context of ConstEval in constant expressions: enum constants, sizes)
class Parser : public ConstEvalContext {
    // Phase 102: Define TypeAlias before public methods that use it
    // This struct must be defined early so public methods can reference it
public:
    struct TypeAlias {
        std::string baseType;
        int pointerLevel;
        bool isSigned;
        bool isFunctionPointer = false;
        std::shared_ptr<FuncPtrSignature> funcPtrSig;
        std::vector<int> arrayDims;
        // typedef int T[n] in a function: the hidden variable holding each
        // run-time dimension ("" for a constant one)
        std::vector<std::string> vlaDimVars;
    };

    Parser(const std::vector<Token>& tokens);
    std::unique_ptr<TranslationUnit> parse();

    // Plain char/short/int/long are signed (standard C) instead of cc45's
    // unsigned default. Set by -fsigned-default; toggled by #pragma cc45
    // signed_default / unsigned_default.
    bool signedDefault = false;
    bool unsignedChar = false;  // -funsigned-char: plain char unsigned regardless of signedDefault

    // Phase 102: Get typedef information for IR generation
    const std::map<std::string, TypeAlias>& getTypedefs() const { return typedefs; }
    const std::map<std::string, StructDefinition*>& getStructDefinitions() const { return structs; }

private:
    std::vector<Token> tokens;
    // Insert tokens at 'at' (a declaration split in two: int f(void), g(void);).
    // tokens has spare capacity reserved by the constructor, so references to
    // tokens held by callers stay valid; beyond it this is an error.
    void insertTokens(size_t at, const std::vector<Token>& v);
    size_t pos;

    const Token& peek() const;
    const Token& advance();
    bool match(TokenType type);
    const Token& expect(TokenType type, const std::string& message);

    template<typename T>
    std::unique_ptr<T> setPos(std::unique_ptr<T> node, const Token& token) {
        node->line = token.line;
        node->column = token.column;
        node->sourceFile = token.sourceFile;
        return node;
    }

    int inStructMethod_ = 0;  // >0 while parsing a non-static struct method body
    std::unique_ptr<FunctionDeclaration> parseFunctionDeclaration();
    std::unique_ptr<CompoundStatement> parseCompoundStatement();
    std::unique_ptr<Statement> parseStatement();
    std::unique_ptr<Statement> parseVariableDeclaration(bool isVolatile, bool isConst = false, bool isStatic = false, bool isRegister = false, bool isStriped = false);
    std::unique_ptr<StaticAssert> parseStaticAssert();
    std::unique_ptr<EnumDefinition> parseEnumDefinition();
    std::unique_ptr<StructDefinition> parseStructDefinition(bool isUnion = false);
    std::unique_ptr<Expression> parseExpression();
    std::unique_ptr<Expression> parseConditional();
    std::unique_ptr<Expression> parseLogicalOr();
    std::unique_ptr<Expression> parseLogicalAnd();
    std::unique_ptr<Expression> parseBitwiseOr();
    std::unique_ptr<Expression> parseBitwiseXor();
    std::unique_ptr<Expression> parseBitwiseAnd();
    std::unique_ptr<Expression> parseEquality();
    std::unique_ptr<Expression> parseRelational();
    std::unique_ptr<Expression> parseShift();
    std::unique_ptr<Expression> parseAdditive();
    std::unique_ptr<Expression> parseMultiplicative();
    std::unique_ptr<Expression> parseUnary();
    std::unique_ptr<Expression> parsePrimary();
    std::unique_ptr<Expression> parseGenericSelection();
    std::unique_ptr<Expression> parseInitializerList();
    void parseTypedef();
    // Declarations of the hidden variables of a typedef's run-time dimensions
    std::vector<std::unique_ptr<Statement>> typedefVlaDecls_;
    int typedefVlaCounter_ = 0;
    bool tryParseAttribute();  // parse __attribute__((...)), returns true if found
    // the width of a bitfield of type `type` (0 only unnamed); an error if out of range
    int parseBitfieldWidth(const std::string& type, const std::string& name, bool unnamed);
    bool pendingAlwaysInline_ = false;  // set by always_inline, consumed by next function declaration
    // __attribute__((aligned(N))): consumed by the declaration it belongs to
    // (variable, struct member or struct type) as if _Alignas(N) were written
    int pendingAlign_ = 0;
    int takePendingAlign() { int a = pendingAlign_; pendingAlign_ = 0; return a; }
    // __noescape (__attribute__((noescape))) seen in the declaration
    bool pendingNoescape_ = false;
    // __pure / pure (1), const (2) seen on a function declaration
    int pendingPurity_ = 0;
    bool takePendingNoescape() { bool n = pendingNoescape_; pendingNoescape_ = false; return n; }
    // #pragma cc45 noescape_strings (until escape_strings): string literals
    // in functions stay with their code (Issue #301)
    bool noescapeStrings_ = false;
    // __jsr / __jsr_raw / __jmp: once a unit uses one, the register bytes
    // (lib/stdlib_asm/cbm_args.s45) are declared at its start; a target
    // named by a function is checked at the end (declared, not defined here)
    bool usesJsr_ = false;
    std::vector<std::pair<std::string, Token>> jsrTargets_;
    std::unique_ptr<Expression> parseJsr();
    // __attribute__((alias("target"))): the declared name is another name
    // for target; later uses of it are read as target
    // Declared variables' types, for typeof(name)
    struct DeclType { std::string type; int ptrLevel = 0; bool isSigned = false; };
    std::map<std::string, DeclType> declTypes_;
    // typeof(expression): the declared type of a variable name, else int
    DeclType typeofExpr(Expression* e);
    std::string pendingAlias_;
    // __attribute__((vector_size(N))) / ((mode(M))): applied by
    // applyVectorMode to the declared type
    int pendingVectorSize_ = 0;
    std::string pendingMode_;
    void applyVectorMode(std::string& type, int ptrLevel, bool& isSigned);
    std::set<std::string> vectorTypes_;
    std::map<std::string, std::string> aliasNames_;
    void applyAlias(std::string& declName) {
        if (pendingAlias_.empty()) return;
        aliasNames_[declName] = pendingAlias_;
        declName = pendingAlias_;
        pendingAlias_.clear();
    }
    bool pendingNoInline_ = false;
    std::string resolveComplexType(); // consume type after COMPLEX token, return struct name
    std::string resolveIntNType(bool isSigned); // consume (N) after __int/__uint, return type name
    std::shared_ptr<FuncPtrSignature> parseFuncPtrParams(const std::string& returnType, int returnPtrLevel, bool returnIsSigned);

    // Unified type specifier parsing — replaces 15+ inline type-matching blocks
    struct TypeSpec {
        std::string name;       // "int", "long", "float", "struct Foo", "struct _Complex_float"
        bool isSigned = false;
        bool isUnsigned = false;
        bool isConst = false;
        bool isVolatile = false;
        int pointerLevel = 0;
        std::vector<int> arrayDims;  // from typedef
        std::vector<std::string> vlaDimVars;  // run-time typedef dimensions (see TypeAlias)
        bool isFunctionPointer = false;
        std::shared_ptr<FuncPtrSignature> funcPtrSig;
        bool valid = false;     // true if a type was successfully parsed
        bool fromTypedef = false; // signedness came from a typedef
        int addressSpace = 0;   // Phase 97: AddressSpace (0=DEFAULT, 1=ZP, 2=ABS, 3=FAR)
        int starsParsed = 0;    // '*'s after the type consumed here
        bool lastStarConst = false; // a const after the last of them (T *const)
    };
    std::unique_ptr<Expression> parseCommaExpression();   // expression , expression ...
    TypeSpec parseTypeSpecifier();    // consume type keywords + qualifiers, return TypeSpec
    TypeSpec parseTypeSpecifierRaw(); // parseTypeSpecifier without the signedness default
    bool isTypeStartToken() const;   // peek: does current token start a type?
    bool isTypeStartAt(size_t look) const; // lookahead version

    std::map<std::string, TypeAlias> typedefs;
    static constexpr const char* BIT_BUILTIN = "__bit(";   // the builtin __bit(v, n)'s token (not the type)
    std::map<std::string, int> enumConstants;
    // Block scopes inside functions: the enum constants a declaration hid or
    // an enumerator replaced there, with their previous values (none: the
    // name was no enum constant), restored when the scope ends
    std::vector<std::vector<std::pair<std::string, std::optional<int>>>> enumScopes_;
    void hideEnumConstant(const std::string& name);     // a local declaration's name
    void setEnumConstant(const std::string& name, int value);
    struct EnumScope {
        Parser& p;
        explicit EnumScope(Parser& parser) : p(parser) { p.enumScopes_.emplace_back(); }
        ~EnumScope();
    };
    // Byte sizes of declared variables with known scalar/array types (sizeof var
    // in constant expressions such as array dimensions)
    std::map<std::string, long> varByteSizes;
    // a variable's shape for sizeof x[i] / sizeof *p in constant expressions
    struct VarShape { std::string type; int ptrLevel = 0; std::vector<int> dims; };
    std::map<std::string, VarShape> varShapes_;
    void recordVarSize(const std::string& name, const std::string& type, int ptrLevel, const std::vector<int>& dims);
    // Evaluate an integer constant expression (ConstEval); false if it isn't one
    bool evalConstInt(Expression* e, long& out);
    // ConstEvalContext: enum constants, sizeof / _Alignof as far as the
    // declarations so far say
    bool identifier(VariableReference& node, ConstValue& out) override;
    bool sizeOf(SizeofExpression& node, int64_t& out) override;
    bool alignOf(AlignofExpression& node, int64_t& out) override;
    int initListLength(const InitializerList& il);
    // Size of a scalar/pointer type in bytes; -1 for aggregates and unknowns
    static int scalarTypeSize(const std::string& type, int ptrLevel);
    // Size of any type known so far (scalars, enums, typedefs, structs and
    // unions laid out as IRBuilder does); -1 when it is not known. natAlign /
    // declAlign: its alignment as an __unpacked struct's member / declared one
    int typeByteSize(const std::string& type, int ptrLevel, int* natAlign = nullptr,
                     int* declAlign = nullptr, int depth = 0);
    std::set<std::string> enumNames;
    bool isTypedef(const std::string& name) const;

    int anonymousAggregateCount = 0;
    std::string currentFunctionName;
    FunctionDeclaration* currentFunction = nullptr;
    std::map<std::string, std::string> nestedFunctionMap;
    std::vector<std::unique_ptr<StructDefinition>> pendingDefinitions;
    std::map<std::string, StructDefinition*> structs; // Phase 3: track parsed struct defs for inheritance
    void flushPending(TranslationUnit& unit);
    bool isFunctionDeclaration();
};
