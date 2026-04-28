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

    printf("\nO45 Writer + RelocEncoder: %d passed, %d failed\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
