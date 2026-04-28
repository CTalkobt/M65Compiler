#include "O45Writer.hpp"
#include <cassert>
#include <cstdio>
#include <cstring>

static int tests_passed = 0;
static int tests_failed = 0;

#define CHECK(cond, msg) do { \
    if (!(cond)) { \
        printf("FAIL: %s (line %d)\n", msg, __LINE__); \
        tests_failed++; \
    } else { \
        tests_passed++; \
    } \
} while(0)

static uint16_t readU16(const uint8_t* p) { return p[0] | (p[1] << 8); }
static uint32_t readU32(const uint8_t* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24); }

// Test 1: Empty object — header only (+ option terminator + empty reloc/symbol tables)
void test_empty_object() {
    O45Writer w;
    auto blob = w.emit();

    // Header: 41 bytes
    CHECK(blob.size() >= O45_HEADER_SIZE, "minimum size");

    // Markers
    CHECK(blob[0] == 0x01, "marker1");
    CHECK(blob[1] == 0x00, "marker2");

    // Magic
    CHECK(blob[2] == 0x6F, "magic o");
    CHECK(blob[3] == 0x36, "magic 6");
    CHECK(blob[4] == 0x35, "magic 5");

    // Version
    CHECK(blob[5] == 0x00, "version");

    // Mode
    uint16_t mode = readU16(&blob[6]);
    CHECK(mode == O45_MODE_DEFAULT, "mode default");
    CHECK(mode & O45_MODE_SIZE32, "SIZE32 set");
    CHECK(mode & O45_MODE_CPUEXT, "CPUEXT set");

    // All segment bases/lengths should be 0
    for (int i = 8; i < 40; i++) {
        CHECK(blob[i] == 0x00, "zero segment fields");
    }

    // CPU ID
    CHECK(blob[40] == O45_CPU_45GS02, "cpu id $45");

    // After header: option terminator ($00)
    CHECK(blob[41] == 0x00, "option terminator");

    // Text reloc table terminator
    CHECK(blob[42] == 0x00, "text reloc end");

    // Data reloc table terminator
    CHECK(blob[43] == 0x00, "data reloc end");

    // Import count = 0
    CHECK(readU32(&blob[44]) == 0, "import count 0");

    // Export count = 0
    CHECK(readU32(&blob[48]) == 0, "export count 0");

    // Total size: 41 header + 1 opt end + 0 text body + 1 text reloc + 0 data body + 1 data reloc + 4 imports + 4 exports = 52
    CHECK(blob.size() == 52, "total size empty object");
}

// Test 2: Header with populated segments
void test_segment_fields() {
    O45Writer w;
    std::vector<uint8_t> code = {0xA9, 0x42, 0x60}; // LDA #$42; RTS
    std::vector<uint8_t> data = {0xAA, 0xBB};

    w.setTextSegment(0x00002000, code);
    w.setDataSegment(0x00004000, data);
    w.setBssSegment(0x00006000, 0x100);
    w.setZpSegment(0x02, 0x08);

    auto blob = w.emit();

    // Text base/len
    CHECK(readU32(&blob[8])  == 0x2000, "tbase");
    CHECK(readU32(&blob[12]) == 3,      "tlen");

    // Data base/len
    CHECK(readU32(&blob[16]) == 0x4000, "dbase");
    CHECK(readU32(&blob[20]) == 2,      "dlen");

    // BSS base/len
    CHECK(readU32(&blob[24]) == 0x6000, "bbase");
    CHECK(readU32(&blob[28]) == 0x100,  "blen");

    // ZP base/len
    CHECK(readU32(&blob[32]) == 0x02, "zbase");
    CHECK(readU32(&blob[36]) == 0x08, "zlen");
}

// Test 3: Segment bodies appear in correct order
void test_segment_bodies() {
    O45Writer w;
    std::vector<uint8_t> code = {0xA9, 0x42, 0x60};
    std::vector<uint8_t> data = {0xDE, 0xAD};

    w.setTextSegment(0x2000, code);
    w.setDataSegment(0x4000, data);

    auto blob = w.emit();

    // After header (41) + option end (1) = offset 42: text body
    CHECK(blob[42] == 0xA9, "text body[0]");
    CHECK(blob[43] == 0x42, "text body[1]");
    CHECK(blob[44] == 0x60, "text body[2]");

    // Text reloc terminator at offset 45
    CHECK(blob[45] == 0x00, "text reloc end");

    // Data body at offset 46
    CHECK(blob[46] == 0xDE, "data body[0]");
    CHECK(blob[47] == 0xAD, "data body[1]");

    // Data reloc terminator at offset 48
    CHECK(blob[48] == 0x00, "data reloc end");
}

// Test 4: Option headers
void test_options() {
    O45Writer w;
    w.addOption(OPT_ASM, "ca45 1.0");

    auto blob = w.emit();

    // Option at offset 41
    uint8_t optLen = blob[41];
    CHECK(optLen == 2 + 8 + 1, "option length"); // len + type + "ca45 1.0" + NUL
    CHECK(blob[42] == OPT_ASM, "option type");
    CHECK(memcmp(&blob[43], "ca45 1.0", 8) == 0, "option value");
    CHECK(blob[51] == 0x00, "option NUL");
    CHECK(blob[52] == OPT_END, "option list end");
}

// Test 5: Mode word flags
void test_mode_flags() {
    O45Writer w;
    w.setMode(O45_MODE_DEFAULT | O45_MODE_BSSZERO);
    auto blob = w.emit();

    uint16_t mode = readU16(&blob[6]);
    CHECK(mode == 0x8820, "mode with BSSZERO");
    CHECK(mode & O45_MODE_BSSZERO, "BSSZERO bit");
}

// Test 6: Import and export tables
void test_symbol_tables() {
    O45Writer w;
    w.addImport("_printf");
    w.addImport("_puts");
    w.addExport("_main", SEG_TEXT, 0x0010);
    w.addExport("_count", SEG_DATA, 0x0000);

    auto blob = w.emit();

    // Find symbol tables: after header(41) + opt end(1) + text reloc(1) + data reloc(1) = offset 44
    size_t off = 44;

    // Import count
    uint32_t importCount = readU32(&blob[off]); off += 4;
    CHECK(importCount == 2, "import count");

    // Import 0: "_printf\0"
    CHECK(strcmp((const char*)&blob[off], "_printf") == 0, "import 0 name");
    off += 8; // strlen("_printf") + 1

    // Import 1: "_puts\0"
    CHECK(strcmp((const char*)&blob[off], "_puts") == 0, "import 1 name");
    off += 6; // strlen("_puts") + 1

    // Export count
    uint32_t exportCount = readU32(&blob[off]); off += 4;
    CHECK(exportCount == 2, "export count");

    // Export 0: "_main" + SEG_TEXT + offset
    CHECK(strcmp((const char*)&blob[off], "_main") == 0, "export 0 name");
    off += 6; // strlen("_main") + 1
    CHECK(blob[off] == SEG_TEXT, "export 0 segment");
    off += 1;
    CHECK(readU32(&blob[off]) == 0x0010, "export 0 offset");
    off += 4;

    // Export 1: "_count" + SEG_DATA + offset
    CHECK(strcmp((const char*)&blob[off], "_count") == 0, "export 1 name");
    off += 7; // strlen("_count") + 1
    CHECK(blob[off] == SEG_DATA, "export 1 segment");
    off += 1;
    CHECK(readU32(&blob[off]) == 0x0000, "export 1 offset");
}

// Test 7: 28-bit address (MEGA65 linear space)
void test_linear_address() {
    O45Writer w;
    w.setTextSegment(0x0050000, {0xEA}); // code at linear $50000
    auto blob = w.emit();

    CHECK(readU32(&blob[8]) == 0x0050000, "28-bit tbase");
    CHECK(readU32(&blob[12]) == 1, "tlen 1 byte");
}

// =============================================================================
// Relocation Encoder Tests
// =============================================================================

// Test 8: Single WORD relocation to text segment
void test_reloc_single_word() {
    std::vector<O45Reloc> relocs = {
        {0x0012, R_WORD, SEG_TEXT, 0, 0}
    };
    auto enc = O45RelocEncoder::encode(relocs);

    CHECK(enc.size() == 2, "reloc single word size");
    CHECK(enc[0] == 0x12, "delta = $12");
    CHECK(enc[1] == (R_WORD | SEG_TEXT), "type/seg = $82");
}

// Test 9: Spec example 1 — JSR external_func at offset $0012
void test_reloc_external_word() {
    std::vector<O45Reloc> relocs = {
        {0x0012, R_WORD, SEG_EXTERNAL, 3, 0}
    };
    auto enc = O45RelocEncoder::encode(relocs);

    // delta($12) + type/seg($80) + sym_index(03 00 00 00) = 6 bytes
    CHECK(enc.size() == 6, "reloc external word size");
    CHECK(enc[0] == 0x12, "delta");
    CHECK(enc[1] == (R_WORD | SEG_EXTERNAL), "type/seg = $80");
    CHECK(readU32(&enc[2]) == 3, "symbol index 3");
}

// Test 10: Spec example 2 — two relocations with correct deltas
void test_reloc_two_entries() {
    std::vector<O45Reloc> relocs = {
        {0x0012, R_WORD, SEG_EXTERNAL, 3, 0},
        {0x0030, R_LINEAR24, SEG_DATA, 0, 0}
    };
    auto enc = O45RelocEncoder::encode(relocs);

    // Entry 1: delta($12) + type/seg($80) + sym(4 bytes) = 6
    CHECK(enc[0] == 0x12, "entry1 delta");
    CHECK(enc[1] == (R_WORD | SEG_EXTERNAL), "entry1 type/seg");
    CHECK(readU32(&enc[2]) == 3, "entry1 sym index");

    // Entry 2: delta($1E = 0x30-0x12) + type/seg($63)
    CHECK(enc[6] == 0x1E, "entry2 delta");
    CHECK(enc[7] == (R_LINEAR24 | SEG_DATA), "entry2 type/seg = $63");
    CHECK(enc.size() == 8, "two entries total size");
}

// Test 11: R_HIGH with extra byte
void test_reloc_high_extra() {
    std::vector<O45Reloc> relocs = {
        {0x05, R_HIGH, SEG_TEXT, 0, 0xAB}
    };
    auto enc = O45RelocEncoder::encode(relocs);

    CHECK(enc.size() == 3, "reloc HIGH size");
    CHECK(enc[0] == 0x05, "delta");
    CHECK(enc[1] == (R_HIGH | SEG_TEXT), "type/seg = $42");
    CHECK(enc[2] == 0xAB, "extra byte");
}

// Test 12: Large gap requiring $FF escape bytes
void test_reloc_large_gap() {
    // Offset 600 requires: 600 / 254 = 2 escapes (2*254=508), remainder = 92
    std::vector<O45Reloc> relocs = {
        {600, R_WORD, SEG_DATA, 0, 0}
    };
    auto enc = O45RelocEncoder::encode(relocs);

    CHECK(enc[0] == 0xFF, "escape 1");
    CHECK(enc[1] == 0xFF, "escape 2");
    CHECK(enc[2] == (600 - 508), "remaining delta = 92");
    CHECK(enc[3] == (R_WORD | SEG_DATA), "type/seg");
    CHECK(enc.size() == 4, "large gap size");
}

// Test 13: Exactly 254-byte gap (single delta, no escape)
void test_reloc_exact_254() {
    std::vector<O45Reloc> relocs = {
        {254, R_LOW, SEG_TEXT, 0, 0}
    };
    auto enc = O45RelocEncoder::encode(relocs);

    CHECK(enc.size() == 2, "exact 254 size");
    CHECK(enc[0] == 0xFE, "delta = $FE (254)");
    CHECK(enc[1] == (R_LOW | SEG_TEXT), "type/seg");
}

// Test 14: 255-byte gap (needs one $FF escape + delta 1)
void test_reloc_255_gap() {
    std::vector<O45Reloc> relocs = {
        {255, R_LOW, SEG_TEXT, 0, 0}
    };
    auto enc = O45RelocEncoder::encode(relocs);

    CHECK(enc.size() == 3, "255 gap size");
    CHECK(enc[0] == 0xFF, "escape");
    CHECK(enc[1] == 1, "remaining delta = 1");
    CHECK(enc[2] == (R_LOW | SEG_TEXT), "type/seg");
}

// Test 15: Multiple entries across large code
void test_reloc_multiple_mixed() {
    std::vector<O45Reloc> relocs = {
        {0x0001, R_WORD, SEG_TEXT, 0, 0},     // JSR at +1
        {0x0004, R_LOW, SEG_DATA, 0, 0},      // LDA #<data at +4
        {0x0006, R_HIGH, SEG_DATA, 0, 0x20},  // LDA #>data at +6
        {0x0100, R_LINEAR32, SEG_TEXT, 0, 0},  // 32-bit ptr at +256
    };
    auto enc = O45RelocEncoder::encode(relocs);

    size_t off = 0;

    // Entry 1: delta=1, R_WORD|SEG_TEXT
    CHECK(enc[off++] == 0x01, "e1 delta");
    CHECK(enc[off++] == (R_WORD | SEG_TEXT), "e1 type/seg");

    // Entry 2: delta=3, R_LOW|SEG_DATA
    CHECK(enc[off++] == 0x03, "e2 delta");
    CHECK(enc[off++] == (R_LOW | SEG_DATA), "e2 type/seg");

    // Entry 3: delta=2, R_HIGH|SEG_DATA + extra
    CHECK(enc[off++] == 0x02, "e3 delta");
    CHECK(enc[off++] == (R_HIGH | SEG_DATA), "e3 type/seg");
    CHECK(enc[off++] == 0x20, "e3 extra byte");

    // Entry 4: delta=0x100-0x06=250, R_LINEAR32|SEG_TEXT
    CHECK(enc[off++] == 250, "e4 delta = 250");
    CHECK(enc[off++] == (R_LINEAR32 | SEG_TEXT), "e4 type/seg");

    CHECK(off == enc.size(), "total encoded size");
}

// Test 16: Empty relocation list
void test_reloc_empty() {
    std::vector<O45Reloc> relocs;
    auto enc = O45RelocEncoder::encode(relocs);
    CHECK(enc.empty(), "empty relocs produce empty stream");
}

// Test 17: Encoder output integrates with O45Writer
void test_reloc_integration() {
    // Build a simple object: text segment with one external JSR
    std::vector<uint8_t> code = {0x20, 0x00, 0x00, 0x60}; // JSR $0000; RTS
    std::vector<O45Reloc> relocs = {
        {0x0001, R_WORD, SEG_EXTERNAL, 0, 0} // patch JSR operand
    };

    O45Writer w;
    w.setTextSegment(0x2000, code);
    w.setTextRelocations(O45RelocEncoder::encode(relocs));
    w.addImport("_printf");
    w.addExport("_main", SEG_TEXT, 0x0000);

    auto blob = w.emit();

    // Verify the text relocation table is in the file
    // Header(41) + opt end(1) + text body(4) = offset 46
    size_t relocStart = 46;
    CHECK(blob[relocStart] == 0x01, "integ: delta=1");
    CHECK(blob[relocStart + 1] == (R_WORD | SEG_EXTERNAL), "integ: type/seg");
    CHECK(readU32(&blob[relocStart + 2]) == 0, "integ: sym index 0");
    CHECK(blob[relocStart + 6] == 0x00, "integ: reloc table end");

    // Verify import table has _printf
    // After text reloc end: data body(0) + data reloc end(1) = +2 from reloc end
    size_t importOff = relocStart + 8; // +6 reloc data + 1 text reloc end + 1 data reloc end
    uint32_t ic = readU32(&blob[importOff]);
    CHECK(ic == 1, "integ: import count");
    CHECK(strcmp((const char*)&blob[importOff + 4], "_printf") == 0, "integ: import name");
}

// =============================================================================
// Symbol Table Tests
// =============================================================================

// Test 18: Import management and index assignment
void test_symtab_imports() {
    O45SymbolTable syms;
    uint32_t i0 = syms.addImport("_printf");
    uint32_t i1 = syms.addImport("_puts");
    uint32_t i2 = syms.addImport("_malloc");

    CHECK(i0 == 0, "import 0 index");
    CHECK(i1 == 1, "import 1 index");
    CHECK(i2 == 2, "import 2 index");
    CHECK(syms.importCount() == 3, "import count 3");
    CHECK(syms.isImported("_printf"), "printf is imported");
    CHECK(!syms.isImported("_exit"), "exit not imported");
    CHECK(syms.getImportIndex("_puts") == 1, "lookup puts index");
    CHECK(syms.getImportIndex("_unknown") == (uint32_t)-1, "unknown returns -1");
}

// Test 19: Export management
void test_symtab_exports() {
    O45SymbolTable syms;
    bool ok1 = syms.addExport("_main", SEG_TEXT, 0x0000);
    bool ok2 = syms.addExport("_count", SEG_DATA, 0x0010);
    bool ok3 = syms.addExport("_bss_buf", SEG_BSS, 0x0000);

    CHECK(ok1, "export _main ok");
    CHECK(ok2, "export _count ok");
    CHECK(ok3, "export _bss_buf ok");
    CHECK(syms.exportCount() == 3, "export count 3");
    CHECK(syms.isExported("_main"), "main is exported");
    CHECK(!syms.isExported("_unknown"), "unknown not exported");

    const auto& exports = syms.getExports();
    CHECK(exports[0].name == "_main", "export 0 name");
    CHECK(exports[0].segment == SEG_TEXT, "export 0 seg");
    CHECK(exports[0].offset == 0x0000, "export 0 offset");
    CHECK(exports[1].name == "_count", "export 1 name");
    CHECK(exports[1].segment == SEG_DATA, "export 1 seg");
    CHECK(exports[1].offset == 0x0010, "export 1 offset");
}

// Test 20: Duplicate import returns existing index
void test_symtab_dedup_import() {
    O45SymbolTable syms;
    uint32_t i0 = syms.addImport("_printf");
    uint32_t i1 = syms.addImport("_puts");
    uint32_t i2 = syms.addImport("_printf"); // duplicate

    CHECK(i0 == 0, "first printf = 0");
    CHECK(i1 == 1, "puts = 1");
    CHECK(i2 == 0, "second printf = 0 (dedup)");
    CHECK(syms.importCount() == 2, "only 2 unique imports");
}

// Test 21: Duplicate export is rejected
void test_symtab_dup_export() {
    O45SymbolTable syms;
    bool ok1 = syms.addExport("_main", SEG_TEXT, 0x0000);
    bool ok2 = syms.addExport("_main", SEG_TEXT, 0x0010); // duplicate

    CHECK(ok1, "first export ok");
    CHECK(!ok2, "duplicate export rejected");
    CHECK(syms.exportCount() == 1, "still 1 export");
}

// Test 22: Validate catches import+export conflict
void test_symtab_validate() {
    O45SymbolTable syms;
    syms.addImport("_printf");
    syms.addExport("_main", SEG_TEXT, 0);

    auto errors = syms.validate();
    CHECK(errors.empty(), "no errors for clean table");

    // Now create a conflict
    O45SymbolTable syms2;
    syms2.addImport("_main");
    syms2.addExport("_main", SEG_TEXT, 0);

    auto errors2 = syms2.validate();
    CHECK(errors2.size() == 1, "one conflict error");
    CHECK(errors2[0].find("_main") != std::string::npos, "error mentions _main");
}

// Test 23: applyTo populates O45Writer correctly
void test_symtab_apply_to_writer() {
    O45SymbolTable syms;
    syms.addImport("_printf");
    syms.addImport("_puts");
    syms.addExport("_main", SEG_TEXT, 0x0000);
    syms.addExport("_count", SEG_DATA, 0x0020);

    O45Writer w;
    syms.applyTo(w);
    auto blob = w.emit();

    // Find import table: header(41) + opt_end(1) + text_reloc(1) + data_reloc(1) = 44
    size_t off = 44;
    uint32_t ic = readU32(&blob[off]); off += 4;
    CHECK(ic == 2, "apply: 2 imports");

    CHECK(strcmp((const char*)&blob[off], "_printf") == 0, "apply: import 0");
    off += 8;
    CHECK(strcmp((const char*)&blob[off], "_puts") == 0, "apply: import 1");
    off += 6;

    uint32_t ec = readU32(&blob[off]); off += 4;
    CHECK(ec == 2, "apply: 2 exports");

    CHECK(strcmp((const char*)&blob[off], "_main") == 0, "apply: export 0 name");
    off += 6;
    CHECK(blob[off] == SEG_TEXT, "apply: export 0 seg");
    off += 1;
    CHECK(readU32(&blob[off]) == 0x0000, "apply: export 0 offset");
    off += 4;

    CHECK(strcmp((const char*)&blob[off], "_count") == 0, "apply: export 1 name");
    off += 7;
    CHECK(blob[off] == SEG_DATA, "apply: export 1 seg");
    off += 1;
    CHECK(readU32(&blob[off]) == 0x0020, "apply: export 1 offset");
}

// Test 24: Full pipeline — symtab + reloc encoder + writer
void test_symtab_full_integration() {
    // Simulate: text segment calls _printf (external) and exports _main
    std::vector<uint8_t> code = {
        0x20, 0x00, 0x00,  // JSR $0000 (to be relocated)
        0x60                // RTS
    };

    O45SymbolTable syms;
    uint32_t printfIdx = syms.addImport("_printf");
    syms.addExport("_main", SEG_TEXT, 0x0000);

    // Relocation: patch JSR operand at offset 1
    std::vector<O45Reloc> relocs = {
        {0x0001, R_WORD, SEG_EXTERNAL, printfIdx, 0}
    };

    O45Writer w;
    w.setTextSegment(0x2000, code);
    w.setTextRelocations(O45RelocEncoder::encode(relocs));
    w.addOption(OPT_ASM, "ca45");
    syms.applyTo(w);

    auto blob = w.emit();

    // Verify header
    CHECK(readU32(&blob[8]) == 0x2000, "full: tbase");
    CHECK(readU32(&blob[12]) == 4, "full: tlen");

    // Verify text body present
    size_t bodyOff = 41 + 7 + 1; // header + option("ca45" = len7) + opt_end
    CHECK(blob[bodyOff] == 0x20, "full: JSR opcode");

    // Verify the file is well-formed by checking import/export at the end
    // Find imports by scanning for "_printf"
    bool foundPrintf = false;
    bool foundMain = false;
    for (size_t i = 0; i + 7 < blob.size(); i++) {
        if (memcmp(&blob[i], "_printf", 7) == 0) foundPrintf = true;
        if (memcmp(&blob[i], "_main", 5) == 0) foundMain = true;
    }
    CHECK(foundPrintf, "full: _printf in output");
    CHECK(foundMain, "full: _main in output");
}

// =============================================================================
// Option Header Tests
// =============================================================================

// Test 25: OPT_OS with raw byte payload
void test_opt_os_raw() {
    O45Writer w;
    w.addOptionRaw(OPT_OS, {OPT_OS_MEGA65});
    auto blob = w.emit();

    // Option at offset 41: len=3 (len+type+1byte), type=$02, data=$05
    CHECK(blob[41] == 3, "OPT_OS length = 3");
    CHECK(blob[42] == OPT_OS, "OPT_OS type");
    CHECK(blob[43] == OPT_OS_MEGA65, "OPT_OS payload = $05");
    CHECK(blob[44] == OPT_END, "option list end");
}

// Test 26: addDefaultOptions produces OPT_OS + OPT_ASM
void test_opt_defaults() {
    O45Writer w;
    w.addDefaultOptions("ca45 1.0");
    auto blob = w.emit();

    size_t off = 41;

    // First option: OPT_OS (raw byte)
    uint8_t osLen = blob[off++];
    CHECK(osLen == 3, "default OPT_OS len = 3");
    CHECK(blob[off++] == OPT_OS, "default OPT_OS type");
    CHECK(blob[off++] == OPT_OS_MEGA65, "default OPT_OS = MEGA65");

    // Second option: OPT_ASM (string "ca45 1.0\0")
    uint8_t asmLen = blob[off++];
    CHECK(asmLen == 2 + 8 + 1, "default OPT_ASM len"); // len+type + "ca45 1.0" + NUL
    CHECK(blob[off++] == OPT_ASM, "default OPT_ASM type");
    CHECK(memcmp(&blob[off], "ca45 1.0", 8) == 0, "default OPT_ASM value");
    off += 8;
    CHECK(blob[off++] == 0x00, "default OPT_ASM NUL");

    // End of options
    CHECK(blob[off] == OPT_END, "default options end");
}

// Test 27: addDefaultOptions with default version string
void test_opt_defaults_no_arg() {
    O45Writer w;
    w.addDefaultOptions();
    auto blob = w.emit();

    size_t off = 41;

    // OPT_OS
    CHECK(blob[off] == 3, "noarg OPT_OS len");
    off += 3;

    // OPT_ASM: "ca45\0"
    uint8_t asmLen = blob[off++];
    CHECK(asmLen == 2 + 4 + 1, "noarg OPT_ASM len"); // len+type + "ca45" + NUL
    CHECK(blob[off++] == OPT_ASM, "noarg OPT_ASM type");
    CHECK(memcmp(&blob[off], "ca45", 4) == 0, "noarg OPT_ASM value");
    off += 4;
    CHECK(blob[off++] == 0x00, "noarg OPT_ASM NUL");
    CHECK(blob[off] == OPT_END, "noarg options end");
}

// Test 28: Multiple options in order
void test_opt_multiple() {
    O45Writer w;
    w.addOption(OPT_FNAME, "hello.o45");
    w.addOptionRaw(OPT_OS, {OPT_OS_MEGA65});
    w.addOption(OPT_ASM, "ca45");
    w.addOption(OPT_AUTHOR, "duck");
    auto blob = w.emit();

    size_t off = 41;

    // OPT_FNAME: "hello.o45\0"
    uint8_t fnLen = blob[off++];
    CHECK(fnLen == 2 + 9 + 1, "fname opt len");
    CHECK(blob[off] == OPT_FNAME, "fname type");
    off += fnLen - 1; // skip rest of this option

    // OPT_OS
    CHECK(blob[off + 1] == OPT_OS, "os type in sequence");
    off += blob[off]; // skip by length

    // OPT_ASM
    CHECK(blob[off + 1] == OPT_ASM, "asm type in sequence");
    off += blob[off];

    // OPT_AUTHOR
    CHECK(blob[off + 1] == OPT_AUTHOR, "author type in sequence");
    off += blob[off];

    // End
    CHECK(blob[off] == OPT_END, "multi options end");
}

// Test 29: Option offsets don't break segment body positions
void test_opt_body_offset() {
    O45Writer w;
    w.addDefaultOptions("ca45");
    std::vector<uint8_t> code = {0xEA}; // NOP
    w.setTextSegment(0x2000, code);
    auto blob = w.emit();

    // Calculate expected offset: header(41) + OPT_OS(3) + OPT_ASM(2+4+1=7) + OPT_END(1) = 52
    size_t expectedBodyOff = 41 + 3 + 7 + 1;
    CHECK(blob[expectedBodyOff] == 0xEA, "body after options: NOP");
}

int main() {
    test_empty_object();
    test_segment_fields();
    test_segment_bodies();
    test_options();
    test_mode_flags();
    test_symbol_tables();
    test_linear_address();

    // Relocation encoder tests
    test_reloc_single_word();
    test_reloc_external_word();
    test_reloc_two_entries();
    test_reloc_high_extra();
    test_reloc_large_gap();
    test_reloc_exact_254();
    test_reloc_255_gap();
    test_reloc_multiple_mixed();
    test_reloc_empty();
    test_reloc_integration();

    // Symbol table tests
    test_symtab_imports();
    test_symtab_exports();
    test_symtab_dedup_import();
    test_symtab_dup_export();
    test_symtab_validate();
    test_symtab_apply_to_writer();
    test_symtab_full_integration();

    // Option header tests
    test_opt_os_raw();
    test_opt_defaults();
    test_opt_defaults_no_arg();
    test_opt_multiple();
    test_opt_body_offset();

    printf("\nO45 Format Tests: %d passed, %d failed\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
