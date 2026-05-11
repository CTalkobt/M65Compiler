#pragma once
#include <cstdint>

// =============================================================================
// .o45 Relocatable Object Format — Constants and Types
//
// The .o45 format is a 32-bit extension of Andre Fachat's .o65 relocatable
// object format. All values below reuse .o65 definitions unless marked [ext].
// See doc/lib45.md for the full specification.
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

// OS identifier for MEGA65
constexpr uint8_t  OPT_OS_MEGA65     = 0x05;

// --- Sub-segment attribute record (OPT_SEGATTR) ---
// Describes a named sub-segment within a segment body (e.g., "init" within text).
// Payload: seg_id(1) + offset(4 LE) + length(4 LE) + name(NUL-terminated)
// The linker uses these to order sub-segments (e.g., "init" before "code").

// --- Function attribute record ---
// Appended after an export entry in the export table when the function has
// ZP calling convention metadata. Identified by the $FA marker byte.
constexpr uint8_t  O45_FUNCATTR_MARKER = 0xFA;
constexpr int      O45_FUNCATTR_SIZE   = 16;   // total bytes per record (including marker)

struct O45FuncAttr {
    uint8_t flags = 0;           // see FUNC_FLAG_* constants
    uint8_t regClobbers = 0;     // bit 0=A, 1=X, 2=Y, 3=Z
    uint8_t flagClobbers = 0;    // bit 0=C, 1=N, 2=Z, 3=V
    uint32_t zpUses = 0;         // bitmask: ZP slots read as params
    uint32_t zpClobbers = 0;     // bitmask: ZP slots written
    uint32_t zpRelease = 0;      // bitmask: ZP slots consumed
};

// Bit values for O45FuncAttr::flags
constexpr uint8_t FUNC_FLAG_LEAF      = 0x01;  // no calls to other functions
constexpr uint8_t FUNC_FLAG_REENTRANT = 0x02;  // re-entrant safe (no global state, stack-only locals)
constexpr uint8_t FUNC_FLAG_ZP_CONV   = 0x04;  // ZP calling convention (0 = stack-based)

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
