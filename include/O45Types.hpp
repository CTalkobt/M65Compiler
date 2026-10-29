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
    R_LOWX        = 0xE0, // low byte; the extra byte holds the high byte of the full value [ext]
    R_IMM8        = 0x00, // SMC: 8-bit immediate value in instruction [Phase 78]
    R_IMM16       = 0x01, // SMC: 16-bit immediate value in instruction [Phase 78]
};

// R_HIGH and R_LOWX records carry an extra byte (the other byte of the value)
constexpr bool o45RelocHasExtra(uint8_t rtype) {
    return (rtype & 0xE0) == 0x40 || (rtype & 0xE0) == 0xE0;
}

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
constexpr uint8_t  OPT_LINEINFO      = 0x11; // debug line info table
constexpr uint8_t  OPT_DEBUG_SYMBOLS = 0x12; // variable/function debug metadata
constexpr uint8_t  OPT_SAC_PARAMS    = 0x13; // SAC parameter metadata (see below)
constexpr uint8_t  OPT_SECTIONS      = 0x14; // section table: per-function/data pieces (see below)
constexpr uint8_t  OPT_SECTREF       = 0x15; // section of a relocation target / export (see below)
constexpr uint8_t  OPT_WEAKUSE       = 0x16; // weak absolute symbols the code was assembled with (see below)
constexpr uint8_t  OPT_DEBUG_TYPES   = 0x17; // debug info: struct / union layouts (text entries, #202)
constexpr uint8_t  OPT_DEBUG_VARS    = 0x18; // debug info: variables, their types and locations (text entries)

// Phase 4: Inter-TU Optimization Hints
constexpr uint8_t  OPT_IPO_HINTS     = 0x50; // Cross-module optimization hints (see below)

// OS identifier for MEGA65
constexpr uint8_t  OPT_OS_MEGA65     = 0x05;

// --- Sub-segment attribute record (OPT_SEGATTR) ---
// Describes a named sub-segment within a segment body (e.g., "init" within text).
// Payload: seg_id(1) + offset(4 LE) + length(4 LE) + name(NUL-terminated)
// The linker uses these to order sub-segments (e.g., "init" before "code").

// --- Section table (OPT_SECTIONS) and section references (OPT_SECTREF) ---
// A sectioned object divides its segment bodies into sections (a function,
// a variable, a string) that ln45 keeps or drops as a whole; bytes no
// section covers are the segment's residual, kept whenever anything of the
// object is. Sections are numbered 0..n-1 in the order of their entries over
// all OPT_SECTIONS records.
// OPT_SECTIONS payload: count(1), then count entries of
//   segflags(1): bits 0-4 the segment (SEG_TEXT..SEG_ZP), bit 7 KEEP (a root)
//   offset(4 LE), length(4 LE) within the segment body; align_log2(1)
//   name (NUL-terminated; may be empty)
// OPT_SECTREF payload: entries of kind(1) key(4 LE) section(2 LE): the
// section an internal relocation (kind 0 text, 1 data; key = its site
// offset) points into, or an export (kind 2; key = export index) belongs
// to, where its value alone does not say (one past the end of a section,
// a low byte); section $FFFF = the residual. Kind 3 (key = import index):
// the section needs that import without a relocation (ca45 .ref): ln45
// pulls its definition when the section is kept.
// Every record's payload is at most 253 bytes (option length byte = +2).
constexpr uint8_t  O45_SECT_KEEP     = 0x80;
constexpr uint16_t O45_SECT_RESIDUAL = 0xFFFF;
constexpr uint8_t  O45_SECTREF_TEXT_RELOC = 0;
constexpr uint8_t  O45_SECTREF_DATA_RELOC = 1;
constexpr uint8_t  O45_SECTREF_EXPORT     = 2;
constexpr uint8_t  O45_SECTREF_IMPORT     = 3;   // .ref: the section needs import #key (no bytes)
constexpr size_t   O45_OPT_PAYLOAD_MAX = 253;

struct O45Section {
    uint8_t segId = 0;        // SEG_TEXT, SEG_DATA, SEG_BSS, SEG_ZP
    bool keep = false;        // always kept (a root)
    uint32_t offset = 0;      // within the segment body
    uint32_t length = 0;
    uint8_t alignLog2 = 0;    // required start alignment (0: none)
    std::string name;         // for the map; may be empty
};

struct O45SectRef {
    uint8_t kind = 0;         // O45_SECTREF_*
    uint32_t key = 0;         // relocation site offset, or export index
    uint16_t section = O45_SECT_RESIDUAL;
};

// The OPT_SECTIONS payloads for a section table (as many records as needed)
inline std::vector<std::vector<uint8_t>> o45EncodeSections(const std::vector<O45Section>& secs) {
    std::vector<std::vector<uint8_t>> out;
    std::vector<uint8_t> cur{0};
    for (const auto& s : secs) {
        std::string name = s.name.substr(0, 200);
        std::vector<uint8_t> e;
        e.push_back((uint8_t)((s.segId & 0x1F) | (s.keep ? O45_SECT_KEEP : 0)));
        for (int i = 0; i < 4; i++) e.push_back((uint8_t)(s.offset >> (8 * i)));
        for (int i = 0; i < 4; i++) e.push_back((uint8_t)(s.length >> (8 * i)));
        e.push_back(s.alignLog2);
        e.insert(e.end(), name.begin(), name.end());
        e.push_back(0);
        if (cur.size() + e.size() > O45_OPT_PAYLOAD_MAX || cur[0] == 255) { out.push_back(cur); cur = {0}; }
        cur.insert(cur.end(), e.begin(), e.end());
        cur[0]++;
    }
    if (cur[0]) out.push_back(cur);
    return out;
}

// Append the entries of one OPT_SECTIONS payload; false if malformed
inline bool o45DecodeSections(const std::vector<uint8_t>& d, std::vector<O45Section>& out) {
    if (d.empty()) return false;
    size_t p = 1;
    for (unsigned n = 0; n < d[0]; n++) {
        if (p + 11 > d.size()) return false;
        O45Section s;
        s.segId = d[p] & 0x1F;
        s.keep = (d[p] & O45_SECT_KEEP) != 0;
        s.offset = d[p+1] | (d[p+2] << 8) | (d[p+3] << 16) | ((uint32_t)d[p+4] << 24);
        s.length = d[p+5] | (d[p+6] << 8) | (d[p+7] << 16) | ((uint32_t)d[p+8] << 24);
        s.alignLog2 = d[p+9];
        p += 10;
        size_t e = p;
        while (e < d.size() && d[e]) e++;
        if (e >= d.size()) return false;
        s.name.assign(d.begin() + p, d.begin() + e);
        p = e + 1;
        out.push_back(s);
    }
    return p == d.size();
}

// The OPT_SECTREF payloads for a list of references
inline std::vector<std::vector<uint8_t>> o45EncodeSectRefs(const std::vector<O45SectRef>& refs) {
    std::vector<std::vector<uint8_t>> out;
    std::vector<uint8_t> cur;
    for (const auto& r : refs) {
        if (cur.size() + 7 > O45_OPT_PAYLOAD_MAX) { out.push_back(cur); cur.clear(); }
        cur.push_back(r.kind);
        for (int i = 0; i < 4; i++) cur.push_back((uint8_t)(r.key >> (8 * i)));
        cur.push_back((uint8_t)(r.section & 0xFF));
        cur.push_back((uint8_t)(r.section >> 8));
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

inline bool o45DecodeSectRefs(const std::vector<uint8_t>& d, std::vector<O45SectRef>& out) {
    if (d.size() % 7) return false;
    for (size_t p = 0; p < d.size(); p += 7) {
        O45SectRef r;
        r.kind = d[p];
        r.key = d[p+1] | (d[p+2] << 8) | (d[p+3] << 16) | ((uint32_t)d[p+4] << 24);
        r.section = (uint16_t)(d[p+5] | (d[p+6] << 8));
        out.push_back(r);
    }
    return true;
}

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
// #360: the record with the base-page masks as addresses: $F9, flags,
// register and flag clobbers, then for uses, clobbers and release a count of
// ranges and (first, last) address pairs, then paramSize and frameSize
// (u16). The $FA record (32-bit masks, bit n = zeroPageStart + 1 + n, the
// base not recorded) is read with ca45's default base, $03
constexpr uint8_t  O45_FUNCATTR2_MARKER = 0xF9;

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

    // Phase 78: SMC (Self-Modifying Code) metadata
    bool useSMC = false;         // true if parameter should be embedded in instructions
    uint32_t accessCount = 0;    // number of times parameter is accessed in function
    std::vector<uint32_t> accessOffsets;  // offsets in code where parameter is accessed
    std::vector<uint8_t> accessSizes;     // size of each access (1=I8, 2=I16, etc.)
};

struct O45SACMetadata {
    std::string functionName;    // function name
    std::vector<O45SACParam> parameters;  // parameter list
};

// A set of base-page addresses $00-$FF, a bit each (#360: the address
// itself, so no base is needed and the whole page fits)
struct ZpMask {
    uint32_t w[8] = {};
    void set(unsigned a) { if (a < 256) w[a >> 5] |= 1u << (a & 31); }
    bool test(unsigned a) const { return a < 256 && ((w[a >> 5] >> (a & 31)) & 1); }
    bool any() const { for (uint32_t x : w) if (x) return true; return false; }
    ZpMask& operator|=(const ZpMask& o) { for (int i = 0; i < 8; i++) w[i] |= o.w[i]; return *this; }
    friend ZpMask operator|(ZpMask a, const ZpMask& b) { a |= b; return a; }
    bool operator==(const ZpMask& o) const { for (int i = 0; i < 8; i++) if (w[i] != o.w[i]) return false; return true; }
    bool operator!=(const ZpMask& o) const { return !(*this == o); }
    // the old 32-bit mask (the $FA record): bit n is base + n
    static ZpMask legacy(uint32_t bits, unsigned base = 0x03) {
        ZpMask m;
        for (unsigned n = 0; n < 32; n++) if (bits & (1u << n)) m.set(base + n);
        return m;
    }
    // "10-11,F2-F5" ("-" when empty)
    std::string str() const {
        std::string s;
        for (unsigned a = 0; a < 256; a++) {
            if (!test(a)) continue;
            unsigned e = a;
            while (e + 1 < 256 && test(e + 1)) e++;
            char b[8];
            if (e == a) snprintf(b, sizeof b, "%02X", a); else snprintf(b, sizeof b, "%02X-%02X", a, e);
            if (!s.empty()) s += ",";
            s += b;
            a = e;
        }
        return s.empty() ? "-" : s;
    }
};

struct O45FuncAttr {
    uint8_t flags = 0;           // see FUNC_FLAG_* constants
    uint8_t regClobbers = 0;     // bit 0=A, 1=X, 2=Y, 3=Z
    uint8_t flagClobbers = 0;    // bit 0=C, 1=N, 2=Z, 3=V
    ZpMask zpUses;               // base-page bytes read as parameters
    ZpMask zpClobbers;           // base-page bytes written
    ZpMask zpRelease;            // base-page bytes consumed
    uint8_t paramSize = 0;       // total parameter bytes (for thunk generation)
    uint16_t frameSize = 0;      // activation record / frame size in bytes (Phase 2)
    O45SACMetadata sacMetadata;  // SAC parameter metadata (Phase 3)
};

// Bit values for O45FuncAttr::flags
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
        case R_LOWX:      return 1;
        case R_WORD:      return 2;
        case R_LINEAR24:  return 3;
        case R_SEGADR:    return 3;
        case R_LINEAR32:  return 4;
        case R_IMM8:      return 1;  // Phase 78: 8-bit immediate in instruction
        case R_IMM16:     return 2;  // Phase 78: 16-bit immediate in instruction
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
// Phase 4: Inter-TU Optimization Hints
// =============================================================================

// Format version for OPT_IPO_HINTS
constexpr uint8_t O45_IPO_HINTS_VERSION = 0x01;

// Specialization pattern hint: constant arguments for a specific call pattern
struct O45IPOSpecPattern {
    std::vector<int64_t> argumentValues;  // Constant values for parameters
    uint8_t frequency = 0;                // % of calls matching this pattern (0-100)
};

// Inter-TU optimization hints for a single function
struct O45IPOFunctionHints {
    std::string functionName;              // Function name
    uint16_t callCount = 0;                // Total calls to this function (global)
    uint16_t estimatedCodeSize = 0;        // Estimated code size (bytes)
    uint8_t flags = 0;                     // Optimization flags (FUNC_FLAG_*)
    std::vector<O45IPOSpecPattern> specializations;  // Specialization patterns
    uint8_t externalCallCount = 0;         // Call sites from external modules
};

// Complete inter-TU optimization hints for a module
struct O45IPOHints {
    uint8_t version = O45_IPO_HINTS_VERSION;  // Format version
    std::vector<O45IPOFunctionHints> functions;  // Hints for each function

    bool isValid() const { return version == O45_IPO_HINTS_VERSION; }
};

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

// =============================================================================
// Call Site Routing (Phase 54)
// =============================================================================

// Information about a specific call site that can be routed
struct CallSiteInfo {
    uint32_t callSiteOffset = 0;        // Offset of JSR instruction in code
    std::string calleeName;             // Called function
    SpecializationPattern argumentPattern; // Detected constant arguments
    std::string targetFunction;         // Specialized function to call
    bool isConstantPattern = false;     // All arguments are constant
    bool hasDispatcher = false;         // Uses dispatcher stub
};

// Routing table for a function
struct FunctionRoutingTable {
    std::string functionName;           // Function being called
    std::vector<std::string> specializedVersions;  // Available specializations
    std::vector<CallSiteInfo> callSites;           // Call sites for this function
    bool usesDispatcher = false;        // Whether to use dispatcher
    std::string dispatcherName;         // Generated dispatcher function name
};

// Call routing analysis: per-function information about where calls go
struct CallRoutingAnalysis {
    std::string functionName;           // Function being analyzed
    std::string dispatcherName;         // Generated dispatcher function name
    std::vector<CallSiteInfo> routableCalls;       // Calls that can be routed
    std::vector<CallSiteInfo> dynamicCalls;        // Calls with non-constant args
    std::vector<std::string> specializedVersions;  // Available specializations
    int totalCalls = 0;                 // Total call count
    float routablePercentage = 0.0f;    // % of calls that are routable
    bool needsDispatcher = false;       // Whether dispatcher is needed
};

// =============================================================================
// Cross-Module Inlining (Phase 55)
// =============================================================================

// Inlining candidate information
struct InliningCandidate {
    std::string functionName;           // Function to inline
    std::string specializationName;     // Specialized version (if applicable)
    int callCount = 0;                  // How many times called
    int estimatedCodeSize = 0;          // Estimated size of inlined code
    int callSiteOverhead = 0;           // Size of JSR + params setup
    int estimatedSavings = 0;           // Estimated bytes saved by inlining
    float benefitRatio = 0.0f;          // Savings / code size ratio
    bool isProfitable = false;          // Worth inlining
    bool isSpecialized = false;         // Is this a specialized version
};

// Inlining analysis results
struct InliningAnalysis {
    std::string callingSite;            // Where the call is (context)
    std::vector<InliningCandidate> candidates;  // Functions to consider inlining
    int totalSavingsPotential = 0;      // Total bytes saveable by inlining
    int selectableCount = 0;            // Count of profitable candidates
    float averageBenefit = 0.0f;        // Average benefit ratio
};

// =============================================================================
// Dispatcher Generation (Phase 56)
// =============================================================================

// Dispatcher route: maps argument pattern to target function
struct DispatcherRoute {
    std::vector<int64_t> argumentPattern;   // Constant argument values (SpecializationPattern)
    std::string targetFunction;             // Specialized version to call
    uint32_t callCount = 0;                 // Times this pattern appears
};

// Dispatcher stub information
struct DispatcherStub {
    std::string dispatcherName;             // Name of dispatcher (_func__dispatch)
    std::string genericFunction;            // Fallback generic function
    std::vector<DispatcherRoute> routes;    // Pattern→target mappings
    int totalRoutes = 0;                    // Number of routes
    int routableCalls = 0;                  // Total calls to routable versions
    int dynamicCalls = 0;                   // Calls handled by dispatcher
    float estimatedCodeSize = 0.0f;         // Estimated dispatcher size
    bool generateDispatcher = false;        // Should emit dispatcher code
};

// Dispatcher generation analysis results
struct DispatcherAnalysis {
    std::string functionName;               // Function being dispatched
    std::vector<DispatcherStub> dispatchers;  // Dispatcher stubs to generate
    int totalSpecializations = 0;           // Number of specialized versions
    int dispatchersNeeded = 0;              // Count of stubs needed
    int totalDispatchCodeSize = 0;          // Total bytes for all dispatchers
    bool usesStaticRouting = false;         // All calls route to known versions
    bool usesDynamicDispatch = false;       // Needs runtime dispatch logic
};
