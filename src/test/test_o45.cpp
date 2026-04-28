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

int main() {
    test_empty_object();
    test_segment_fields();
    test_segment_bodies();
    test_options();
    test_mode_flags();
    test_symbol_tables();
    test_linear_address();

    printf("\nO45 Writer: %d passed, %d failed\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
