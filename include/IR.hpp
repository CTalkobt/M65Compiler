#pragma once

// IR (Intermediate Representation) for cc45.
// Simple 3-address code with virtual registers.
// See doc/ir.md for the human-readable text format specification.

#include <cstdint>
#include <map>
#include <set>
#include <list>
#include <string>
#include <vector>
#include <memory>
#include <ostream>

namespace ir {

// ============================================================================
// Types
// ============================================================================

enum class Type : uint8_t {
    VOID,
    I8,
    I16,
    I32,
    PTR,
};

inline int typeSize(Type t) {
    switch (t) {
        case Type::VOID: return 0;
        case Type::I8:   return 1;
        case Type::I16:  return 2;
        case Type::I32:  return 4;
        case Type::PTR:  return 2;
    }
    return 0;
}

// ============================================================================
// Opcodes
// ============================================================================

enum class Op : uint8_t {
    CONST,          // %d = const <type> <val>
    ADD, SUB, MUL, DIV, MOD, // binary arithmetic
    MUL_U, DIV_U, MOD_U,
    AND, OR, XOR, LSL, LSR, ASR, // binary bitwise
    SHL, SHR,
    NEG, NOT,       // unary
    CMP_EQ, CMP_NE, CMP_LT, CMP_LE, CMP_GT, CMP_GE, // comparison (signed)
    CMP_LTU, CMP_LEU, CMP_GTU, CMP_GEU,             // comparison (unsigned)
    LOAD,           // %d = load <type> <addr>
    STORE,          // store <type> <val>, <addr>
    LOAD_ZP,        // %d = load_zp <type> <zpAddr>
    STORE_ZP,       // store_zp <type> <val>, <zpAddr>

    // Address computation
    ADDR_GLOBAL,    // %d = addr_global @name        (address of global/static)
    ADDR_LOCAL,     // %d = addr_local <frameOffset>  (address of stack local)
    ADDR_LABEL,     // %d = addr_label @label         (address of local label)
    ADDR_UPLEVEL,   // %d = addr_uplevel <levels>, <frameOffset> (access parent frame)
    ADDR_ELEM,      // %d = addr_elem <base>, <index>, <elemSize>

    // Type conversion
    SEXT,           // %d = sext <srcType> <src> to <destType>  (sign-extend)
    ZEXT,           // %d = zext <srcType> <src> to <destType>  (zero-extend)
    TRUNC,          // %d = trunc <srcType> <src> to <destType> (truncate)

    // Bitfield access
    BFEXT,          // %d = bfext <src>, offset, width
    BFINS,          // %d = bfins <dest>, <src>, offset, width

    // Register/memory movement
    COPY,           // %d = copy <type> <src>

    // Control flow
    BR,             // br <label>
    BR_COND,        // br_cond <cond>, <trueLabel>, <falseLabel>
    BR_INDIRECT,    // br_indirect <funcPtr>  (computed goto)
    SWITCH,         // switch <val>, <defaultLabel>, [case1: label1, ...]
    RET,            // ret <val>  (or ret void)
    RET_VOID,       // ret void

    // Calls
    CALL,           // %d = call @name(<args...>) -> <type>
    CALL_INDIRECT,  // %d = call_indirect <funcPtr>(<args...>) -> <type>
    CALL_VOID,      // call_void @name(<args...>)

    // Special
    ASM_INLINE,     // asm("...")
    VA_START,       // %d = va_start(lastParamName) — compute stack addr past last named param
    TRAMPOLINE,     // %d = trampoline <target>, <link>
    CPU_REG_READ,   // %d = cpu_reg_read <regName>
    CPU_REG_WRITE,  // cpu_reg_write <regName>, <val>
    CPU_FLAG_READ,  // %d = cpu_flag_read <flagName>
    CPU_FLAG_WRITE, // cpu_flag_write <flagName>, <val>
    PHI,            // %d = phi [<val1>, <label1>], [<val2>, <label2>], ...
    GET_FP,         // %d = get_fp
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
    FRAME_RELATIVE, // offset relative to a frame pointer
};

struct Operand {
    OperandKind kind = OperandKind::NONE;
    Type type = Type::VOID;
    uint32_t vregId = 0;
    int64_t immVal = 0;
    std::string name;

    static Operand none() { return {}; }
    static Operand vreg(uint32_t id, Type t) { Operand o; o.kind = OperandKind::VREG; o.vregId = id; o.type = t; return o; }
    static Operand imm(int64_t val, Type t) { Operand o; o.kind = OperandKind::IMM; o.immVal = val; o.type = t; return o; }
    static Operand global(const std::string& name) { Operand o; o.kind = OperandKind::GLOBAL; o.name = name; o.type = Type::PTR; return o; }
    static Operand label(const std::string& name) { Operand o; o.kind = OperandKind::LABEL; o.name = name; o.type = Type::PTR; return o; }

    bool isNone() const { return kind == OperandKind::NONE; }
    bool isVreg() const { return kind == OperandKind::VREG; }
    bool isImm() const { return kind == OperandKind::IMM; }
};

// ============================================================================
// Instruction
// ============================================================================

struct SourceLoc {
    std::string file;
    int line = 0;
    int column = 0;

    bool valid() const { return line > 0; }
};

enum class CallConv : uint8_t {
    STACK,      // standard right-to-left push
    ZP,         // parameters in zero-page block
};

struct Inst {
    Op op = Op::NOP;
    Operand dest;               // result register (NONE for void/stores/branches)
    Operand src1;               // first operand
    Operand src2;               // second operand (NONE for unary ops)
    Type resultType = Type::VOID;

    // For CALL/CALL_INDIRECT: argument list and convention
    std::vector<Operand> args;
    CallConv callConv = CallConv::STACK;
    bool isRegparm = false;  // first param passed in A/AX

    // For SWITCH: case values and labels
    std::vector<std::pair<int64_t, std::string>> switchCases;

    // For PHI: incoming values with block labels
    std::vector<std::pair<Operand, std::string>> phiIncoming;

    // For ASM_INLINE: raw assembly string
    std::string asmText;

    SourceLoc loc;
};

// ============================================================================
// Function and Module
// ============================================================================

struct Block {
    std::string label;
    std::vector<Inst> insts;
};

struct Function {
    std::string name;
    Type returnType = Type::VOID;
    CallConv conv = CallConv::STACK;
    std::vector<Type> paramTypes;
    std::vector<std::string> paramNames;
    bool isVariadic = false;
    bool isStatic = false;
    bool isInterrupt = false;
    bool isNaked = false;
    bool isRegparm = false;
    bool isNested = false;
    int staticLinkVreg = -1; // vreg holding the static link to parent frame
    int declLine = 0;                   // source line of function declaration

    std::vector<Block> blocks;
    uint32_t nextVreg = 0;             // counter for allocating vRegs
    std::set<uint32_t> localSlotVregs; // vRegs that are direct local/param slots
    std::map<uint32_t, int> vregOffsets; // vregId -> final frame offset
    std::map<uint32_t, Type> vregTypes;
    std::map<uint32_t, int> vregSizes;
 // override sizes for array vRegs (bytes)
    std::map<std::string, uint32_t> localNames; // name (without _l_ or _p_ prefix) -> vregId
    std::set<uint32_t> memoryVregs;    // vRegs that MUST be in memory (e.g. volatile or address-taken)

    uint32_t allocVreg() { return nextVreg++; }

    struct VarInfo {
        std::string name;
        Type type;
        int size = 0;
    };
};

struct Module {
    std::string sourceFile;
    bool saveZP = true;
    std::list<Function> functions;
    std::vector<std::string> externs;

    // Global variable declarations (for ADDR_GLOBAL references)
    struct GlobalVar {
        std::string name;
        Type type;
        int size = 0;           // total bytes (may differ from typeSize for arrays)
        bool isConst = false;
        bool isStatic = false;
        bool hasInitValue = false;
        int64_t initValue = 0;
        std::vector<int64_t> initList;
    };
    std::vector<GlobalVar> globals;

    struct StringLiteral {
        std::string label;
        std::string value;
        int encoding = 0;
        bool isAscii = false;
    };
    std::vector<StringLiteral> strings;
};

class Printer {
public:
    static std::string operandStr(const Operand& op);
    static void print(std::ostream& out, const Inst& inst);
    static void print(std::ostream& out, const Function& fn);
    static void print(std::ostream& out, const Module& mod);
};

const char* opName(Op op);
const char* typeName(Type t);

} // namespace ir
