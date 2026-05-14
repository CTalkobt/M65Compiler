#pragma once
// IR.hpp — Intermediate Representation for cc45
//
// Linear three-address code with basic blocks and virtual registers.
// This is the data structure definition only; no compiler integration yet.
// See doc/ir.md for the human-readable text format specification.

#include <cstdint>
#include <set>
#include <string>
#include <vector>
#include <memory>
#include <ostream>

namespace ir {

// ============================================================================
// Types
// ============================================================================

enum class Type : uint8_t {
    VOID = 0,
    I8   = 1,   // char (1 byte)
    I16  = 2,   // int, pointer (2 bytes)
    I32  = 4,   // long (4 bytes)
    PTR  = 2,   // alias for I16 on 45GS02 (16-bit address space)
};

inline int typeSize(Type t) {
    switch (t) {
        case Type::VOID: return 0;
        case Type::I8:   return 1;
        case Type::I16:  return 2;
        case Type::I32:  return 4;
        default:         return 2;
    }
}

inline const char* typeName(Type t) {
    switch (t) {
        case Type::VOID: return "void";
        case Type::I8:   return "i8";
        case Type::I16:  return "i16";
        case Type::I32:  return "i32";
        default:         return "i16";
    }
}

// ============================================================================
// Calling convention
// ============================================================================

enum class CallConv : uint8_t {
    STACK = 0,   // Parameters on hardware stack (default)
    ZP    = 1,   // Parameters in zero-page block ($03+)
};

// ============================================================================
// Opcodes
// ============================================================================

enum class Op : uint8_t {
    // Constants
    CONST,          // %d = const <type> <imm>

    // Arithmetic (binary: dest = src1 op src2)
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    NEG,            // unary: dest = -src1

    // Bitwise
    AND,
    OR,
    XOR,
    NOT,            // unary: dest = ~src1
    SHL,
    SHR,            // logical shift right
    ASR,            // arithmetic shift right

    // Comparison (dest = src1 cmp src2, result is I8 boolean)
    CMP_EQ,
    CMP_NE,
    CMP_LT,         // signed <
    CMP_LE,         // signed <=
    CMP_GT,         // signed >
    CMP_GE,         // signed >=
    CMP_LTU,        // unsigned <
    CMP_LEU,        // unsigned <=
    CMP_GTU,        // unsigned >
    CMP_GEU,        // unsigned >=

    // Memory
    LOAD,           // %d = load <type> <addr>
    STORE,          // store <type> <val>, <addr>
    LOAD_ZP,        // %d = load_zp <type> <zpAddr>
    STORE_ZP,       // store_zp <type> <val>, <zpAddr>

    // Address computation
    ADDR_GLOBAL,    // %d = addr_global @name        (address of global/static)
    ADDR_LOCAL,     // %d = addr_local <frameOffset>  (address of stack local)
    ADDR_ELEM,      // %d = addr_elem <base>, <index>, <elemSize>

    // Type conversion
    SEXT,           // %d = sext <srcType> <src> to <destType>  (sign-extend)
    ZEXT,           // %d = zext <srcType> <src> to <destType>  (zero-extend)
    TRUNC,          // %d = trunc <srcType> <src> to <destType> (truncate)

    // Bitfield operations
    BFEXT,          // %d = bfext <type> <src>, <bitOffset>, <bitWidth>  (extract)
    BFINS,          // bfins <type> <val>, <addr>, <bitOffset>, <bitWidth> (insert RMW)

    // Control flow
    BR,             // br <label>
    BR_COND,        // br_cond <cond>, <trueLabel>, <falseLabel>
    SWITCH,         // switch <val>, <defaultLabel>, [case1: label1, ...]
    RET,            // ret <val>  (or ret void)
    RET_VOID,       // ret void

    // Calls
    CALL,           // %d = call @name(<args...>) -> <type>
    CALL_INDIRECT,  // %d = call_indirect <funcPtr>(<args...>) -> <type>
    CALL_VOID,      // call_void @name(<args...>)

    // Special
    ASM_INLINE,     // asm("...")
    PHI,            // %d = phi [<val1>, <label1>], [<val2>, <label2>], ...
    NOP,
};

// ============================================================================
// Operand
// ============================================================================

enum class OperandKind : uint8_t {
    NONE,
    VREG,       // virtual register: %N
    IMM,        // immediate constant
    GLOBAL,     // global symbol: @name
    LABEL,      // block label: .label
};

struct Operand {
    OperandKind kind = OperandKind::NONE;
    Type type = Type::VOID;

    uint32_t vregId = 0;        // for VREG
    int64_t immVal = 0;         // for IMM
    std::string name;           // for GLOBAL or LABEL

    static Operand vreg(uint32_t id, Type t) {
        Operand o; o.kind = OperandKind::VREG; o.vregId = id; o.type = t; return o;
    }
    static Operand imm(int64_t val, Type t) {
        Operand o; o.kind = OperandKind::IMM; o.immVal = val; o.type = t; return o;
    }
    static Operand global(const std::string& n) {
        Operand o; o.kind = OperandKind::GLOBAL; o.name = n; o.type = Type::PTR; return o;
    }
    static Operand label(const std::string& n) {
        Operand o; o.kind = OperandKind::LABEL; o.name = n; return o;
    }
    static Operand none() { return Operand{}; }

    bool isNone() const { return kind == OperandKind::NONE; }
    bool isVreg() const { return kind == OperandKind::VREG; }
    bool isImm() const { return kind == OperandKind::IMM; }
};

// ============================================================================
// Source location (for future debug info)
// ============================================================================

struct SourceLoc {
    std::string file;
    int line = 0;
    bool valid() const { return line > 0; }
};

// ============================================================================
// Instruction
// ============================================================================

struct Inst {
    Op op = Op::NOP;
    Operand dest;               // result register (NONE for void/stores/branches)
    Operand src1;               // first operand
    Operand src2;               // second operand (NONE for unary ops)
    Type resultType = Type::VOID;

    // For CALL/CALL_INDIRECT: argument list
    std::vector<Operand> args;

    // For SWITCH: case values and labels
    std::vector<std::pair<int64_t, std::string>> switchCases;

    // For PHI: incoming values with block labels
    std::vector<std::pair<Operand, std::string>> phiIncoming;

    // For ASM_INLINE: raw assembly string
    std::string asmText;

    // Source location annotation
    SourceLoc loc;
};

// ============================================================================
// Basic Block
// ============================================================================

struct Block {
    std::string label;
    std::vector<Inst> insts;
};

// ============================================================================
// Function
// ============================================================================

struct Function {
    std::string name;                   // mangled name (e.g. "_main")
    CallConv conv = CallConv::STACK;
    Type returnType = Type::VOID;
    std::vector<Type> paramTypes;
    std::vector<std::string> paramNames; // optional, for debug info
    bool isVariadic = false;
    bool isStatic = false;

    std::vector<Block> blocks;
    uint32_t nextVreg = 0;             // counter for allocating vRegs
    std::set<uint32_t> localSlotVregs; // vRegs that are direct local/param slots

    uint32_t allocVreg() { return nextVreg++; }
};

// ============================================================================
// Module (translation unit)
// ============================================================================

struct Module {
    std::string sourceFile;
    std::vector<Function> functions;

    // Global variable declarations (for ADDR_GLOBAL references)
    struct GlobalVar {
        std::string name;
        Type type;
        int size = 0;           // total bytes (may differ from typeSize for arrays)
        bool isConst = false;
        bool isStatic = false;
        bool hasInitValue = false;
        int64_t initValue = 0;  // scalar initial value (for simple int/char globals)
    };
    std::vector<GlobalVar> globals;

    // String literals: label → content
    struct StringLiteral {
        std::string label;
        std::string value;
        bool isAscii = false;
    };
    std::vector<StringLiteral> strings;

    // External symbol references (stdlib functions, etc.)
    std::vector<std::string> externs;
};

// ============================================================================
// Text format printer
// ============================================================================

class Printer {
public:
    static void print(std::ostream& out, const Module& mod);
    static void print(std::ostream& out, const Function& fn);
    static void print(std::ostream& out, const Inst& inst);
    static std::string operandStr(const Operand& op);
};

} // namespace ir
