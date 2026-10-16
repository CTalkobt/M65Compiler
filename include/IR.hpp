#pragma once

// IR (Intermediate Representation) for cc45.
// Simple 3-address code with virtual registers.
// See doc/architecture/ir.md for the human-readable text format specification.

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
    I_N,    // Arbitrary-width integer (byte count from vregSizes/operand)
    PTR,
    F32,    // CBM 40-bit float (5 bytes: exponent + 4-byte mantissa)
};

inline int typeSize(Type t) {
    switch (t) {
        case Type::VOID: return 0;
        case Type::I8:   return 1;
        case Type::I16:  return 2;
        case Type::I32:  return 4;
        case Type::I_N:  return 0; // Must use vregSizes lookup
        case Type::PTR:  return 2;
        case Type::F32:  return 5; // CBM 40-bit float
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
    DEREF,          // %d = deref <ptr>  (read pointer value for indirect store target)

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
    VEC_BIN,        // vec_bin <ptrA>, <ptrB|scalar>: args = ptrR, op (0 + 1 - 2 & 3 | 4 ^),
                    // element size (1/2), count, B is a scalar, unrolled
    CPU_REG_READ,   // %d = cpu_reg_read <regName>
    CPU_REG_WRITE,  // cpu_reg_write <regName>, <val>
    CPU_FLAG_READ,  // %d = cpu_flag_read <flagName>
    CPU_FLAG_WRITE, // cpu_flag_write <flagName>, <val>
    PHI,            // %d = phi [<val1>, <label1>], [<val2>, <label2>], ...
    GET_FP,         // %d = get_fp
    NOP,

    // Float operations (CBM ROM via MAP)
    FADD,           // %d = fadd %a, %b
    FSUB,           // %d = fsub %a, %b
    FMUL,           // %d = fmul %a, %b
    FDIV,           // %d = fdiv %a, %b
    FCMP,           // %d = fcmp %a, %b  (returns -1, 0, 1)
    FNEG,           // %d = fneg %a
    FTOI,           // %d = ftoi %a  (float → int)
    ITOF,           // %d = itof %a  (int → float)
    FCONST,         // %d = fconst <5-byte CBM float>
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
    uint8_t byteWidth = 0; // For I_N: actual byte count (3,5,6,8,16,etc.)

    static Operand none() { return {}; }
    static Operand vreg(uint32_t id, Type t) { Operand o; o.kind = OperandKind::VREG; o.vregId = id; o.type = t; return o; }
    static Operand vregN(uint32_t id, int bytes) { Operand o; o.kind = OperandKind::VREG; o.vregId = id; o.type = Type::I_N; o.byteWidth = bytes; return o; }
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
    STATIC,     // static activation record (Phase 1: no address sharing)
};

struct Inst {
    Op op = Op::NOP;
    Operand dest;               // result register (NONE for void/stores/branches)
    Operand src1;               // first operand
    Operand src2;               // second operand (NONE for unary ops)
    Type resultType = Type::VOID;
    uint8_t resultByteWidth = 0; // For I_N: actual byte count

    // ADDR_ELEM, or a LOAD/STORE with a fused one, whose base is a global:
    // bytes added to it (a[i + c] as element i of a + c*stride)
    int elemOffset = 0;

    // For CALL/CALL_INDIRECT: argument list and convention
    std::vector<Operand> args;
    CallConv callConv = CallConv::STACK;
    bool isRegparm = false;  // first param passed in A/AX
    bool isTailCall = false; // tail call optimization (replace JSR with JMP)

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
    bool isWeak = false;
    bool isInterrupt = false;
    bool isNaked = false;
    bool isRegparm = false;
    bool isNested = false;
    // symbols this function needs without a relocation (ca45 .ref): the
    // printf conversion hooks its format strings use
    std::set<std::string> asmRefs;
    bool isRecurse = false;      // opt-out of static allocation (SAC)
    // #pragma cc45 optimize(...) in effect at the definition: catalog name -> on/off
    std::map<std::string, bool> optOverrides;
    // Labels whose address (&&label) is used from data, e.g. a static
    // table for goto *: their blocks are kept and get a global alias
    // (labelAlias()) the data can refer to
    std::set<std::string> addressTakenLabels;
    std::string labelAlias(const std::string& label) const { return name + "__lbl_" + label; }
    // __builtin_return_address(0): the entry code copies the return address
    // to __zp_scratch3, and the first instruction stores it to this slot
    int retAddrSlot = -1;
    int align = 1;          // __attribute__((aligned(N))): code address alignment
    int staticLinkVreg = -1; // vreg holding the static link to parent frame
    int declLine = 0;                   // source line of function declaration

    std::vector<Block> blocks;
    uint32_t nextVreg = 0;             // counter for allocating vRegs
    std::set<uint32_t> localSlotVregs; // vRegs that are direct local/param slots
    std::map<uint32_t, int> vregOffsets; // vregId -> final frame offset
    std::map<uint32_t, Type> vregTypes;
    std::map<uint32_t, int> vregSizes;
    std::map<uint32_t, int> vregAlign;  // locals with _Alignas / aligned (> 1)
 // override sizes for array vRegs (bytes)
    std::map<std::string, uint32_t> localNames; // name (without _l_ or _p_ prefix) -> vregId
    std::vector<std::string> localNamesOrder; // preserves declaration order of local variables
    std::set<uint32_t> memoryVregs;    // vRegs that MUST be in memory (e.g. volatile or address-taken)
    std::set<uint32_t> registerVregs;  // vRegs declared with 'register' keyword (prefer ZP allocation)
    std::set<uint32_t> registerXVregs; // vRegs marked for X-register residency (loop counters, etc)
    std::set<uint32_t> registerYVregs; // vRegs marked for Y-register residency (nested loop counters)
    std::set<uint32_t> registerZVregs; // vRegs marked for Z-register residency (deeply nested loop counters)

    // Phase 2, Phase 3: Original leaf status (before IR optimization/inlining)
    // Computed from original AST before any IR optimizations
    bool originalIsLeaf = true;         // true if original code had no function calls
    std::set<std::string> originalCallees;  // functions called in original code (before inlining)

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
    // A float passed as a variadic (or unprototyped) argument: link the
    // %f-aware printf (_printf_float), and sprintf (_sprintf_float)
    bool floatVarArg = false;
    bool floatSprintfArg = false;
    // scanf/fscanf/sscanf given a float pointer, or a v*scanf call (its
    // arguments are not visible): with float code, the %f-aware scanf
    bool floatScanfArg = false;
    bool callsVscanf = false;
    // asm("...") statements at file scope, in source order: passed to the
    // assembler at module level
    std::vector<std::string> fileAsm;
    int setBP = -1;  // -1 = don't emit; 0-255 = emit LDA #N; TAB in startup
    // CPU stack: -1 = crt0 default (16-bit stack topped at $BFFF), 0 = keep
    // the ROM's 8-bit page-1 stack (-fno-stack16), else the stack's top byte
    int stackTop = -1;
    std::set<std::string> asmRefs;   // .ref needed outside any function
    std::string overlay;    // -foverlay=name: the code is in that code overlay (.overlay)
    int stackSize = -1;     // -fstack-size=N / #pragma cc45 stack_size(N): __stack_size (-1: ln45's default)
    int bssFill = -1;       // -fbss-fill=V: crt0 fills BSS with V (-1: ln45's default, 0)
    int maxUnrollBytes = -1;   // #pragma cc45 max_unroll_bytes (last one in the file); -1 = -fmax-unroll-bytes
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
        std::vector<std::string> initLabels; // symbolic labels for pointer array elements (e.g. string literals)
        std::vector<std::string> vtableMethodNames; // Phase 3: function names for vtable entries
        int addressSpace = 0;   // Phase 97: 0=default(__abs), 1=__zp, 2=__far
        int align = 1;          // _Alignas / __attribute__((aligned)) / aligned struct type
        // Constant initializer laid out byte by byte (structs, nested arrays):
        // initBytes covers the whole object; initWords are 2-byte addresses
        // (labels) and initStrings string literal text, at their offsets
        struct ImageString { std::string value, escMask; int encoding = 0; };
        bool hasImage = false;
        std::vector<uint8_t> initBytes;
        std::map<int, std::string> initWords;
        std::map<int, ImageString> initStrings;
    };
    std::vector<GlobalVar> globals;

    struct StringLiteral {
        std::string label;
        std::string value;
        std::string escMask;  // per byte: '.', 'n', 'r', 'x' (see Token::escMask)
        int encoding = 0;
        bool isAscii = false;
        bool wide = false;    // 16-bit characters (L"...")
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
