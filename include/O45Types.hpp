#pragma once
#include <cstdint>
#include <string>
#include <vector>

// =============================================================================
// .o45 Relocatable Object Format — Constants and Types
//
// The .o45 format is a 32-bit extension of Andre Fachat's .o65 relocatable
// object format. All values below reuse .o65 definitions unless marked [ext].
// See doc/architecture/lib45.md for the full specification.
// =============================================================================

// --- File markers ---
constexpr uint8_t  O45_MARKER1       = 0x01;
constexpr uint8_t  O45_MARKER2       = 0x00;
constexpr uint8_t  O45_MAGIC[3]      = { 0x6F, 0x34, 0x35 }; // "o45"
constexpr uint8_t  O45_VERSION       = 0x00;

// --- Fixed header size (32-bit mode + CPU ext byte) ---
constexpr int      O45_HEADER_SIZE   = 41;

// --- Mode word bits (reused from .o65) ---
constexpr uint16_t O45_MODE_PAGED    = 0x0001;
constexpr uint16_t O45_MODE_OBJ      = 0x0000; // bits 1-2 = 00
constexpr uint16_t O45_MODE_EXE      = 0x0002; // bits 1-2 = 01
constexpr uint16_t O45_MODE_LIB      = 0x0004; // bits 1-2 = 10
constexpr uint16_t O45_MODE_SIMPLE   = 0x0008;
constexpr uint16_t O45_MODE_CHAIN    = 0x0010;
constexpr uint16_t O45_MODE_BSSZERO  = 0x0020;
constexpr uint16_t O45_MODE_CPUEXT   = 0x0800;
constexpr uint16_t O45_MODE_SIZE32   = 0x8000;

// Canonical .o45 mode: 32-bit addresses + extended CPU byte
constexpr uint16_t O45_MODE_DEFAULT  = O45_MODE_SIZE32 | O45_MODE_CPUEXT;

// --- CPU IDs (extended byte, when MODE_CPUEXT is set) ---
constexpr uint8_t  O45_CPU_6502      = 0x01;
constexpr uint8_t  O45_CPU_65C02     = 0x02;
constexpr uint8_t  O45_CPU_65CE02    = 0x03;
constexpr uint8_t  O45_CPU_65816     = 0x04;
constexpr uint8_t  O45_CPU_45GS02    = 0x45;

// --- Segment IDs (bits 4-0 of relocation type/seg byte) ---
enum O45Segment : uint8_t {
    SEG_EXTERNAL  = 0,   // external symbol (index follows)
    SEG_ABSOLUTE  = 1,   // absolute / no relocation
    SEG_TEXT      = 2,   // .text (code)
    SEG_DATA      = 3,   // .data (initialized)
    SEG_BSS       = 4,   // .bss  (uninitialized)
    SEG_ZP        = 5,   // .zp   (zero/direct page)
};

// Export flag: high bit of the segment byte in the export table
constexpr uint8_t  O45_EXPORT_WEAK    = 0x80;
constexpr uint8_t  O45_EXPORT_SEG_MASK = 0x7F; // mask to extract segment from flagged byte

// --- Relocation types (bits 7-5 of relocation type/seg byte) ---
enum O45RelocType : uint8_t {
    R_LOW         = 0x20, // low byte of 16-bit address
    R_HIGH        = 0x40, // high byte of 16-bit address
    R_LINEAR24    = 0x60, // 24-bit linear address (3 bytes) [ext]
    R_WORD        = 0x80, // full 16-bit address (2 bytes)
    R_LINEAR32    = 0xA0, // 32-bit linear address (4 bytes) [ext]
    R_SEGADR      = 0xC0, // segment address (bank:addr, 3 bytes)
};

// Masks for splitting the type/seg byte
constexpr uint8_t  O45_RTYPE_MASK    = 0xE0; // bits 7-5
constexpr uint8_t  O45_RSEG_MASK     = 0x1F; // bits 4-0

// --- Relocation offset encoding ---
constexpr uint8_t  O45_RELOC_END     = 0x00; // end of relocation table
constexpr uint8_t  O45_RELOC_ESCAPE  = 0xFF; // add 254, read next byte
constexpr int      O45_RELOC_SKIP    = 254;  // bytes skipped per escape

// --- Option types (reused from .o65) ---
constexpr uint8_t  OPT_END           = 0x00;
constexpr uint8_t  OPT_FNAME         = 0x01;
constexpr uint8_t  OPT_OS            = 0x02;
constexpr uint8_t  OPT_ASM           = 0x03;
constexpr uint8_t  OPT_AUTHOR        = 0x04;
constexpr uint8_t  OPT_CREATED       = 0x05;

// Extended option types (.o45 specific)
constexpr uint8_t  OPT_SEGATTR       = 0x10; // sub-segment attribute (see below)
constexpr uint8_t  OPT_SAC_PARAMS    = 0x13; // SAC parameter metadata (see below)

// OS identifier for MEGA65
constexpr uint8_t  OPT_OS_MEGA65     = 0x05;

// --- Sub-segment attribute record (OPT_SEGATTR) ---
// Describes a named sub-segment within a segment body (e.g., "init" within text).
// Payload: seg_id(1) + offset(4 LE) + length(4 LE) + name(NUL-terminated)
// The linker uses these to order sub-segments (e.g., "init" before "code").

// --- SAC parameter metadata record (OPT_SAC_PARAMS) ---
// Describes parameters for a SAC (Static Allocation Convention) function.
// Payload: func_name_len(1) + func_name(string) + param_count(1) + parameters
// Each parameter: offset(2 LE) + size(1) + sym_name_len(1) + sym_name(string)
// The linker uses this to validate parameter passing and potentially initialize storage.

// --- Function attribute record ---
// Appended after an export entry in the export table when the function has
// ZP calling convention metadata or SAC metadata. Identified by the $FA marker byte.
// Phase 2 addition: frameSize tracks activation record size for call-graph overlay coloring.
constexpr uint8_t  O45_FUNCATTR_MARKER = 0xFA;
constexpr int      O45_FUNCATTR_SIZE   = 19;   // total bytes per record (including marker, now with frameSize)

// Phase 49: Content flag record for exports
// Appended after an export entry to indicate what type of content it has (native code, IR, etc.)
// Identified by the $FB marker byte.
constexpr uint8_t  O45_CONTENTFLAG_MARKER = 0xFB;

struct O45SACParam {
    uint16_t offset = 0;         // offset in activation record
    uint8_t size = 0;            // parameter size in bytes
    std::string symbolName;      // symbol name (e.g., "_add_short__param_a")
    bool isConstant = false;     // true if all call sites pass same constant value
    int64_t constantValue = 0;   // value if isConstant is true
};

struct O45SACMetadata {
    std::string functionName;    // function name
    std::vector<O45SACParam> parameters;  // parameter list
};

struct O45FuncAttr {
    uint8_t flags = 0;           // see FUNC_FLAG_* constants
    uint8_t regClobbers = 0;     // bit 0=A, 1=X, 2=Y, 3=Z
    uint8_t flagClobbers = 0;    // bit 0=C, 1=N, 2=Z, 3=V
    uint32_t zpUses = 0;         // bitmask: ZP slots read as params
    uint32_t zpClobbers = 0;     // bitmask: ZP slots written
    uint32_t zpRelease = 0;      // bitmask: ZP slots consumed
    uint8_t paramSize = 0;       // total parameter bytes (for thunk generation)
    uint16_t frameSize = 0;      // activation record / frame size in bytes (Phase 2)
    O45SACMetadata sacMetadata;  // SAC parameter metadata (Phase 3)
};

// Bit values for O45FuncAttr::flags
constexpr uint8_t OPT_LINEINFO        = 0x11;  // debug line info table
constexpr uint8_t OPT_DEBUG_SYMBOLS   = 0x12;  // variable/function debug metadata

constexpr uint8_t FUNC_FLAG_LEAF         = 0x01;  // no calls to other functions
constexpr uint8_t FUNC_FLAG_REENTRANT    = 0x02;  // re-entrant safe (no global state, stack-only locals)
constexpr uint8_t FUNC_FLAG_ZP_CONV      = 0x04;  // ZP calling convention (0 = stack-based)
constexpr uint8_t FUNC_FLAG_STATIC_ALLOC = 0x08;  // uses SAC (static activation record)
constexpr uint8_t FUNC_FLAG_ISR          = 0x10;  // interrupt handler
constexpr uint8_t FUNC_FLAG_ZERO_ALLOC   = 0x20;  // SAC leaf with no locals and all constant params

// --- Patch sizes per relocation type ---
constexpr int o45RelocPatchSize(uint8_t rtype) {
    switch (rtype & O45_RTYPE_MASK) {
        case R_LOW:       return 1;
        case R_HIGH:      return 1;
        case R_WORD:      return 2;
        case R_LINEAR24:  return 3;
        case R_SEGADR:    return 3;
        case R_LINEAR32:  return 4;
        default:          return 0;
    }
}

// --- Segment name <-> ID mapping ---
constexpr O45Segment o45SegmentFromName(const char* name) {
    if (name[0] == 't') return SEG_TEXT;
    if (name[0] == 'c') return SEG_TEXT;  // "code" alias
    if (name[0] == 'i') return SEG_TEXT;  // "init" maps to text body
    if (name[0] == 'd') return SEG_DATA;
    if (name[0] == 'b') return SEG_BSS;
    if (name[0] == 'z') return SEG_ZP;
    return SEG_ABSOLUTE;
}

constexpr const char* o45SegmentName(O45Segment seg) {
    switch (seg) {
        case SEG_TEXT: return "text";
        case SEG_DATA: return "data";
        case SEG_BSS:  return "bss";
        case SEG_ZP:   return "zp";
        default:       return "???";
    }
}

// =============================================================================
// IR Serialization Support (Phase 47 - Extended .o45 Format)
// =============================================================================

// IR Version tracking: Major.Minor
// Major version mismatch = incompatible
// Minor version mismatch = forward compatible
constexpr uint8_t O45_IR_VERSION_MAJOR = 0;  // Initial version
constexpr uint8_t O45_IR_VERSION_MINOR = 1;

// Content type flags for exports
// Indicates what data is present for each symbol
constexpr uint8_t O45_CONTENT_FLAG_NATIVE_CODE = 0x01;  // Has native 6502 code
constexpr uint8_t O45_CONTENT_FLAG_HAS_IR      = 0x02;  // Has IR metadata
constexpr uint8_t O45_CONTENT_FLAG_RESERVED1   = 0x04;
constexpr uint8_t O45_CONTENT_FLAG_RESERVED2   = 0x08;
constexpr uint8_t O45_CONTENT_FLAG_IR_ENCODING = 0xF0;  // Bits 4-7: encoding type
                                                       // 0 = uncompressed, 1 = RLE, 2 = LZ4

// IR Type IDs for parameters and return values
enum O45IRType : uint8_t {
    IR_TYPE_VOID      = 0x00,
    IR_TYPE_I8        = 0x01,
    IR_TYPE_I16       = 0x02,
    IR_TYPE_I32       = 0x03,
    IR_TYPE_I64       = 0x04,
    IR_TYPE_FLOAT     = 0x05,
    IR_TYPE_PTR       = 0x06,
    IR_TYPE_STRUCT    = 0x07,
    IR_TYPE_UNKNOWN   = 0xFF,
};

// IR Parameter Flags
constexpr uint8_t O45_IR_PARAM_IS_CONST   = 0x01;  // Parameter always receives constant
constexpr uint8_t O45_IR_PARAM_IS_USED    = 0x02;  // Parameter is actually used in function
constexpr uint8_t O45_IR_PARAM_IS_MODIFIED = 0x04; // Parameter is modified (written)

// IR Call Site Information
struct O45IRCallSite {
    uint32_t instructionOffset = 0;      // Offset of JSR in code
    std::string calleeName;              // Name of called function
    std::vector<int64_t> paramValues;    // Actual parameter values passed (for constants)
    std::vector<uint8_t> paramIsConst;   // Which parameters are constant
};

// IR Parameter Information
struct O45IRParam {
    O45IRType type = IR_TYPE_UNKNOWN;    // Parameter type
    uint8_t flags = O45_IR_PARAM_IS_USED; // Flags (const, used, modified)
    int64_t constValue = 0;              // Value if is_const flag set
    std::string name;                    // Parameter name (optional)
};

// IR Call Graph Entry
struct O45IRCallGraphEntry {
    std::string calleeName;              // Called function name
    uint16_t callCount = 0;              // Number of times called
    bool allCallsConstant = false;       // All calls pass same constants
};

// IR Function Metadata
struct O45IRFunction {
    std::string functionName;            // Function name
    uint32_t signatureHash = 0;          // Quick hash of signature for validation
    std::vector<O45IRParam> parameters;  // Parameter information
    std::vector<O45IRCallSite> callSites; // All call sites in this function
    std::vector<O45IRCallGraphEntry> callGraph; // Functions this calls

    // Quick validation
    bool isValid() const { return !functionName.empty() && signatureHash != 0; }
};

// Complete IR metadata for a single object file
struct O45IRMetadata {
    uint8_t majorVersion = O45_IR_VERSION_MAJOR;
    uint8_t minorVersion = O45_IR_VERSION_MINOR;
    std::vector<O45IRFunction> functions;  // IR for each function

    bool isCompatible() const {
        return majorVersion == O45_IR_VERSION_MAJOR;  // Major must match
    }
};

// =============================================================================
// Function Specialization Support (Phase 52)
// =============================================================================

// A specialization pattern: vector of constant values for parameters
// E.g., for function(int a, int b): pattern {10, 2} or {5, 2}
using SpecializationPattern = std::vector<int64_t>;

// Specialization profile for a function
struct FunctionSpecialization {
    std::string originalName;           // Original function name (e.g., "_calculate")
    SpecializationPattern pattern;      // Constant parameter pattern (e.g., {10, 2})
    std::string specializedName;        // Generated name (e.g., "_calculate_10_2")
    int callCount = 0;                  // How many times this pattern is called
    double estimatedBenefit = 0.0;      // Expected code size reduction (%)
    bool isGenerated = false;           // Whether this specialization was created
};

// Specialization analysis results
struct SpecializationAnalysis {
    std::string functionName;           // Original function name
    std::vector<SpecializationPattern> patterns;  // Observed call patterns
    std::vector<int> patternCounts;     // Call count for each pattern
    int totalCalls = 0;                 // Total call count
    bool isProfitable = false;          // Worth generating specialization
    float topPatternFrequency = 0.0f;   // Frequency of most common pattern
};
