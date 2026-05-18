#include "O45Writer.hpp"
#include "O45Reader.hpp"
#include "O45Linker.hpp"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <sstream>

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

// Test 1: No input objects to linker — link() returns false with "no input objects" error
void test_no_input_objects() {
    O45Linker linker;
    std::string errorMsg;

    // Don't add any objects — attempt to link
    auto result = linker.link(errorMsg);

    CHECK(result.empty(), "link result empty");
    CHECK(errorMsg.find("no input objects") != std::string::npos, "error contains 'no input objects'");
}

// Test 2: Duplicate symbol from two objects — link() returns false with "duplicate symbol" error
void test_duplicate_symbol() {
    // Create two objects that both export "shared_func"
    O45Writer w1;
    std::vector<uint8_t> code1 = {0x60}; // RTS
    w1.setTextSegment(0x2000, code1);
    w1.addExport("shared_func", SEG_TEXT, 0);
    auto obj1 = w1.emit();

    O45Writer w2;
    std::vector<uint8_t> code2 = {0x60}; // RTS
    w2.setTextSegment(0x2100, code2);
    w2.addExport("shared_func", SEG_TEXT, 0);
    auto obj2 = w2.emit();

    // Read them back
    O45File file1, file2;
    std::string err1, err2;
    CHECK(O45Reader::read(obj1, file1, err1), "read obj1");
    CHECK(O45Reader::read(obj2, file2, err2), "read obj2");

    // Try to link
    O45Linker linker;
    linker.addObject("obj1.o45", file1);
    linker.addObject("obj2.o45", file2);
    std::string errorMsg;

    auto result = linker.link(errorMsg);

    CHECK(result.empty(), "link result empty");
    CHECK(errorMsg.find("duplicate symbol") != std::string::npos, "error contains 'duplicate symbol'");
    CHECK(errorMsg.find("shared_func") != std::string::npos, "error mentions symbol name");
}

// Test 3: Undefined symbol import — link() returns false with "undefined symbol" error
void test_undefined_symbol_import() {
    // Create one object that imports "external_func" but doesn't export it
    O45Writer w;
    std::vector<uint8_t> code = {0x60}; // RTS
    w.setTextSegment(0x2000, code);
    w.addImport("external_func");
    auto obj = w.emit();

    O45File file;
    std::string err;
    CHECK(O45Reader::read(obj, file, err), "read object");

    // Try to link without providing the exported symbol
    O45Linker linker;
    linker.addObject("obj.o45", file);
    std::string errorMsg;

    auto result = linker.link(errorMsg);

    CHECK(result.empty(), "link result empty");
    CHECK(errorMsg.find("undefined symbol") != std::string::npos, "error contains 'undefined symbol'");
    CHECK(errorMsg.find("external_func") != std::string::npos, "error mentions symbol name");
}

// Test 4: Relocation offset out of range — link() returns false with "relocation offset" error
void test_relocation_offset_out_of_range() {
    // Create an object with a relocation that points past the segment end
    O45Writer w;
    std::vector<uint8_t> code = {0xA9, 0x42, 0x60}; // LDA #$42; RTS (3 bytes)
    w.setTextSegment(0x2000, code);

    // Add import and export
    w.addImport("external_func");
    w.addExport("test_func", SEG_TEXT, 0);

    // Create a relocation at offset 10 (way past the 3-byte segment)
    // The relocation will reference import index 0
    std::vector<O45Reloc> relocs = {
        {10, R_WORD, SEG_EXTERNAL, 0, 0}
    };
    w.setTextRelocations(O45RelocEncoder::encode(relocs));

    auto obj = w.emit();

    O45File file;
    std::string err;
    CHECK(O45Reader::read(obj, file, err), "read object");

    // Create matching export in a second object
    O45Writer w2;
    std::vector<uint8_t> code2 = {0x60};
    w2.setTextSegment(0x3000, code2);
    w2.addExport("external_func", SEG_TEXT, 0);
    auto obj2 = w2.emit();

    O45File file2;
    std::string err2;
    CHECK(O45Reader::read(obj2, file2, err2), "read obj2");

    // Try to link
    O45Linker linker;
    linker.addObject("obj1.o45", file);
    linker.addObject("obj2.o45", file2);
    linker.setTextBase(0x2000);
    std::string errorMsg;

    auto result = linker.link(errorMsg);

    CHECK(result.empty(), "link result empty");
    CHECK(errorMsg.find("relocation offset") != std::string::npos, "error contains 'relocation offset'");
}

// Test 5: Invalid symbol index in relocation — link() returns false with "invalid symbol index" error
void test_invalid_symbol_index() {
    // Create an object with a relocation that references a non-existent import index
    O45Writer w;
    std::vector<uint8_t> code = {0x00, 0x00, 0x00, 0x60};
    w.setTextSegment(0x2000, code);
    w.addExport("test_func", SEG_TEXT, 0);

    // Add one import
    w.addImport("func0");

    // Create a relocation that references import index 99 (doesn't exist)
    // Use offset 1 because offset 0 causes delta=0 which is skipped by encoder
    std::vector<O45Reloc> relocs = {
        {1, R_WORD, SEG_EXTERNAL, 99, 0}
    };
    w.setTextRelocations(O45RelocEncoder::encode(relocs));

    auto obj = w.emit();

    O45File file;
    std::string err;
    CHECK(O45Reader::read(obj, file, err), "read object");

    // Create matching export
    O45Writer w2;
    std::vector<uint8_t> code2 = {0x60};
    w2.setTextSegment(0x3000, code2);
    w2.addExport("func0", SEG_TEXT, 0);
    auto obj2 = w2.emit();

    O45File file2;
    std::string err2;
    CHECK(O45Reader::read(obj2, file2, err2), "read obj2");

    // Try to link
    O45Linker linker;
    linker.addObject("obj1.o45", file);
    linker.addObject("obj2.o45", file2);
    linker.setTextBase(0x2000);
    std::string errorMsg;

    auto result = linker.link(errorMsg);

    CHECK(result.empty(), "link result empty");
    CHECK(errorMsg.find("invalid symbol index") != std::string::npos, "error contains 'invalid symbol index'");
}

// Test 6: Undefined symbol in relocation (import not resolved) — link() returns false with "undefined symbol" error
void test_undefined_symbol_in_relocation() {
    // Create an object with a relocation referencing an unresolved import
    O45Writer w;
    std::vector<uint8_t> code = {0x00, 0x00, 0x60};
    w.setTextSegment(0x2000, code);
    w.addExport("test_func", SEG_TEXT, 0);

    w.addImport("unresolved_func");

    std::vector<O45Reloc> relocs = {
        {0, R_WORD, SEG_EXTERNAL, 0, 0}
    };
    w.setTextRelocations(O45RelocEncoder::encode(relocs));

    auto obj = w.emit();

    O45File file;
    std::string err;
    CHECK(O45Reader::read(obj, file, err), "read object");

    // Try to link without providing the import
    O45Linker linker;
    linker.addObject("obj.o45", file);
    linker.setTextBase(0x2000);
    std::string errorMsg;

    auto result = linker.link(errorMsg);

    CHECK(result.empty(), "link result empty");
    CHECK(errorMsg.find("undefined symbol") != std::string::npos, "error contains 'undefined symbol'");
}

// Test 7: Relocation patch overflow — link() returns false with "relocation patch overflows" error
void test_relocation_patch_overflow() {
    // Create an object with a 4-byte relocation (R_LINEAR32) that would overflow segment
    O45Writer w;
    std::vector<uint8_t> code = {0x00, 0x00}; // Only 2 bytes
    w.setTextSegment(0x2000, code);
    w.addExport("test_func", SEG_TEXT, 0);

    w.addImport("external_func");

    // Try to place a 4-byte patch at offset 1 (would need 4 bytes but only 2 available)
    // Use offset 1 because offset 0 causes delta=0 which is skipped by encoder
    std::vector<O45Reloc> relocs = {
        {1, R_LINEAR32, SEG_EXTERNAL, 0, 0}
    };
    w.setTextRelocations(O45RelocEncoder::encode(relocs));

    auto obj = w.emit();

    O45File file;
    std::string err;
    CHECK(O45Reader::read(obj, file, err), "read object");

    // Create matching export
    O45Writer w2;
    std::vector<uint8_t> code2 = {0x60};
    w2.setTextSegment(0x3000, code2);
    w2.addExport("external_func", SEG_TEXT, 0);
    auto obj2 = w2.emit();

    O45File file2;
    std::string err2;
    CHECK(O45Reader::read(obj2, file2, err2), "read obj2");

    // Try to link
    O45Linker linker;
    linker.addObject("obj1.o45", file);
    linker.addObject("obj2.o45", file2);
    linker.setTextBase(0x2000);
    std::string errorMsg;

    auto result = linker.link(errorMsg);

    CHECK(result.empty(), "link result empty");
    CHECK(errorMsg.find("relocation patch overflows") != std::string::npos, "error contains 'relocation patch overflows'");
}

// Test 8: Unknown relocation type — link() returns false with "unknown relocation type" error
void test_unknown_relocation_type() {
    // Create a raw O45File with an invalid relocation type byte
    O45Writer w;
    std::vector<uint8_t> code = {0x00, 0x00, 0x60};
    w.setTextSegment(0x2000, code);
    w.addExport("test_func", SEG_TEXT, 0);
    w.addImport("external_func");

    auto obj = w.emit();

    // Parse and modify to inject bad reloc
    O45File file;
    std::string err;
    CHECK(O45Reader::read(obj, file, err), "read object");

    // Clear existing relocs and inject a bad one with invalid type byte
    file.textRelocs.clear();
    // Manually craft relocation: offset=1 (delta=1 to avoid offset 0 issue), invalid type byte (255)
    file.textRelocs.push_back(1); // offset delta (to get offset 1)
    file.textRelocs.push_back(255); // Invalid type/segment byte (all bits set)
    file.textRelocs.push_back(0); // symbol index (4 bytes, external)
    file.textRelocs.push_back(0);
    file.textRelocs.push_back(0);
    file.textRelocs.push_back(0); // symbol index value
    file.textRelocs.push_back(0); // end marker

    // Create matching export
    O45Writer w2;
    std::vector<uint8_t> code2 = {0x60};
    w2.setTextSegment(0x3000, code2);
    w2.addExport("external_func", SEG_TEXT, 0);
    auto obj2 = w2.emit();

    O45File file2;
    std::string err2;
    CHECK(O45Reader::read(obj2, file2, err2), "read obj2");

    // Try to link
    O45Linker linker;
    linker.addObject("obj1.o45", file);
    linker.addObject("obj2.o45", file2);
    linker.setTextBase(0x2000);
    std::string errorMsg;

    auto result = linker.link(errorMsg);

    CHECK(result.empty(), "link result empty");
    CHECK(errorMsg.find("unknown relocation type") != std::string::npos, "error contains 'unknown relocation type'");
}

// =============================================================================
// SUCCESS-PATH TESTS — Validate correct linking behavior
// =============================================================================

// Helper: create an O45File from an O45Writer's emit() output
static O45File makeFile(O45Writer& w) {
    auto bytes = w.emit();
    O45File f;
    std::string err;
    O45Reader::read(bytes, f, err);
    return f;
}

// Test: Single object links to produce correct PRG binary
void test_single_object_prg() {
    O45Writer w;
    std::vector<uint8_t> code = {0xA9, 0x42, 0x60}; // LDA #$42; RTS
    w.setTextSegment(0, code);
    w.addExport("_main", SEG_TEXT, 0);
    O45File f = makeFile(w);

    O45Linker linker;
    linker.addObject("main.o45", f);
    linker.setTextBase(0x2000);
    std::string err;
    auto result = linker.link(err, true); // isPrg=true

    CHECK(!result.empty(), "single object PRG link succeeds");
    CHECK(err.empty(), "no error message");
    // PRG header: little-endian $2000
    CHECK(result.size() >= 5, "PRG has header + code");
    CHECK(result[0] == 0x00 && result[1] == 0x20, "PRG header is $2000 LE");
    CHECK(result[2] == 0xA9 && result[3] == 0x42 && result[4] == 0x60, "code bytes correct");
    // Symbol map
    auto& syms = linker.getSymbolMap();
    CHECK(syms.count("_main") && syms.at("_main") == 0x2000, "_main at $2000");
}

// Test: Single object links to flat binary (no PRG header)
void test_single_object_flat() {
    O45Writer w;
    std::vector<uint8_t> code = {0xEA, 0xEA, 0x60}; // NOP; NOP; RTS
    w.setTextSegment(0, code);
    O45File f = makeFile(w);

    O45Linker linker;
    linker.addObject("flat.o45", f);
    linker.setTextBase(0x2000);
    std::string err;
    auto result = linker.link(err, false); // flat

    CHECK(!result.empty(), "flat binary link succeeds");
    CHECK(result.size() == 3, "flat binary is code only (no header)");
    CHECK(result[0] == 0xEA, "first byte is NOP");
}

// Test: Two objects merge text segments in order
void test_two_objects_merge() {
    O45Writer w1;
    std::vector<uint8_t> code1 = {0xA9, 0x01, 0x60}; // LDA #$01; RTS
    w1.setTextSegment(0, code1);
    w1.addExport("_func_a", SEG_TEXT, 0);
    O45File f1 = makeFile(w1);

    O45Writer w2;
    std::vector<uint8_t> code2 = {0xA9, 0x02, 0x60}; // LDA #$02; RTS
    w2.setTextSegment(0, code2);
    w2.addExport("_func_b", SEG_TEXT, 0);
    O45File f2 = makeFile(w2);

    O45Linker linker;
    linker.addObject("a.o45", f1);
    linker.addObject("b.o45", f2);
    linker.setTextBase(0x2000);
    std::string err;
    auto result = linker.link(err, false);

    CHECK(!result.empty() && err.empty(), "two-object link succeeds");
    CHECK(result.size() == 6, "merged text is 6 bytes");
    auto& syms = linker.getSymbolMap();
    CHECK(syms.count("_func_a") && syms.at("_func_a") == 0x2000, "_func_a at $2000");
    CHECK(syms.count("_func_b") && syms.at("_func_b") == 0x2003, "_func_b at $2003");
}

// Test: R_WORD relocation patches external symbol address
void test_reloc_r_word() {
    // Object 1: JSR _helper (3 bytes: $20, $00, $00 — reloc patches bytes 1-2)
    O45Writer w1;
    std::vector<uint8_t> code1 = {0x20, 0x00, 0x00, 0x60}; // JSR $0000; RTS
    w1.setTextSegment(0, code1);
    w1.addImport("_helper");
    w1.addExport("_main", SEG_TEXT, 0);
    std::vector<O45Reloc> relocs1 = {{1, R_WORD, SEG_EXTERNAL, 0, 0}};
    w1.setTextRelocations(O45RelocEncoder::encode(relocs1));
    O45File f1 = makeFile(w1);

    // Object 2: exports _helper
    O45Writer w2;
    std::vector<uint8_t> code2 = {0xA9, 0xFF, 0x60}; // LDA #$FF; RTS
    w2.setTextSegment(0, code2);
    w2.addExport("_helper", SEG_TEXT, 0);
    O45File f2 = makeFile(w2);

    O45Linker linker;
    linker.addObject("main.o45", f1);
    linker.addObject("helper.o45", f2);
    linker.setTextBase(0x2000);
    std::string err;
    auto result = linker.link(err, false);

    CHECK(!result.empty() && err.empty(), "R_WORD relocation link succeeds");
    // _helper is at $2000 + 4 (after main's 4 bytes) = $2004
    CHECK(result[1] == 0x04, "R_WORD low byte patched");
    CHECK(result[2] == 0x20, "R_WORD high byte patched");
}

// Test: R_LOW relocation patches low byte only
void test_reloc_r_low() {
    O45Writer w1;
    std::vector<uint8_t> code1 = {0xA9, 0x00, 0x60}; // LDA #$00; RTS (byte 1 = reloc)
    w1.setTextSegment(0, code1);
    w1.addImport("_data");
    std::vector<O45Reloc> relocs = {{1, R_LOW, SEG_EXTERNAL, 0, 0}};
    w1.setTextRelocations(O45RelocEncoder::encode(relocs));
    O45File f1 = makeFile(w1);

    O45Writer w2;
    std::vector<uint8_t> code2 = {0x60};
    w2.setTextSegment(0, code2);
    w2.addExport("_data", SEG_TEXT, 0);
    O45File f2 = makeFile(w2);

    O45Linker linker;
    linker.addObject("a.o45", f1);
    linker.addObject("b.o45", f2);
    linker.setTextBase(0x2000);
    std::string err;
    auto result = linker.link(err, false);

    CHECK(!result.empty() && err.empty(), "R_LOW relocation link succeeds");
    // _data at $2003 — low byte = $03
    CHECK(result[1] == 0x03, "R_LOW patches low byte to $03");
}

// Test: R_HIGH relocation patches high byte
void test_reloc_r_high() {
    O45Writer w1;
    std::vector<uint8_t> code1 = {0xA9, 0x00, 0x60}; // LDA #$00; RTS (byte 1 = reloc)
    w1.setTextSegment(0, code1);
    w1.addImport("_data");
    std::vector<O45Reloc> relocs = {{1, R_HIGH, SEG_EXTERNAL, 0, 0}};
    w1.setTextRelocations(O45RelocEncoder::encode(relocs));
    O45File f1 = makeFile(w1);

    O45Writer w2;
    std::vector<uint8_t> code2 = {0x60};
    w2.setTextSegment(0, code2);
    w2.addExport("_data", SEG_TEXT, 0);
    O45File f2 = makeFile(w2);

    O45Linker linker;
    linker.addObject("a.o45", f1);
    linker.addObject("b.o45", f2);
    linker.setTextBase(0x2000);
    std::string err;
    auto result = linker.link(err, false);

    CHECK(!result.empty() && err.empty(), "R_HIGH relocation link succeeds");
    // _data at $2003 — high byte = $20
    CHECK(result[1] == 0x20, "R_HIGH patches high byte to $20");
}

// Test: Internal (SEG_TEXT) relocation resolves within same object
void test_reloc_internal_text() {
    O45Writer w;
    // Code: JSR $0004 (bytes 0-2), NOP (byte 3), LDA #$42 (bytes 4-5), RTS (byte 6)
    // The assembler places the assembly-time address ($0004) at the patch site.
    // The linker reads it, subtracts tbase (0), adds final textBase ($2000).
    std::vector<uint8_t> code = {0x20, 0x04, 0x00, 0xEA, 0xA9, 0x42, 0x60};
    w.setTextSegment(0, code);
    w.addExport("_main", SEG_TEXT, 0);
    w.addExport("_sub", SEG_TEXT, 4);
    // Internal text reloc: offset 1, R_WORD, SEG_TEXT
    std::vector<O45Reloc> relocs = {{1, R_WORD, SEG_TEXT, 0, 0}};
    w.setTextRelocations(O45RelocEncoder::encode(relocs));
    O45File f = makeFile(w);

    O45Linker linker;
    linker.addObject("test.o45", f);
    linker.setTextBase(0x2000);
    std::string err;
    auto result = linker.link(err, false);

    CHECK(!result.empty() && err.empty(), "internal text reloc link succeeds");
    // Existing val $0004 - tbase $0000 = offset 4, + textBase $2000 = $2004
    CHECK(result[1] == 0x04, "internal reloc low byte = $04");
    CHECK(result[2] == 0x20, "internal reloc high byte = $20");
}

// Test: Data segment merging
void test_data_segment_merge() {
    O45Writer w1;
    std::vector<uint8_t> code1 = {0x60};
    w1.setTextSegment(0, code1);
    std::vector<uint8_t> data1 = {0x01, 0x02, 0x03};
    w1.setDataSegment(0, data1);
    w1.addExport("_main", SEG_TEXT, 0);
    w1.addExport("_data1", SEG_DATA, 0);
    O45File f1 = makeFile(w1);

    O45Writer w2;
    std::vector<uint8_t> code2 = {0x60};
    w2.setTextSegment(0, code2);
    std::vector<uint8_t> data2 = {0x04, 0x05};
    w2.setDataSegment(0, data2);
    w2.addExport("_data2", SEG_DATA, 0);
    O45File f2 = makeFile(w2);

    O45Linker linker;
    linker.addObject("a.o45", f1);
    linker.addObject("b.o45", f2);
    linker.setTextBase(0x2000);
    linker.setDataBase(0x3000);
    std::string err;
    auto result = linker.link(err, false);

    CHECK(!result.empty() && err.empty(), "data segment merge succeeds");
    auto& syms = linker.getSymbolMap();
    CHECK(syms.count("_data1") && syms.at("_data1") == 0x3000, "_data1 at $3000");
    CHECK(syms.count("_data2") && syms.at("_data2") == 0x3003, "_data2 at $3003");
}

// Test: BSS segment merge and __bss_start/__bss_end symbols
void test_bss_merge_and_symbols() {
    O45Writer w1;
    std::vector<uint8_t> code1 = {0x60};
    w1.setTextSegment(0, code1);
    w1.setBssSegment(0, 100); // 100 bytes BSS
    w1.addExport("_main", SEG_TEXT, 0);
    O45File f1 = makeFile(w1);

    O45Writer w2;
    std::vector<uint8_t> code2 = {0x60};
    w2.setTextSegment(0, code2);
    w2.setBssSegment(0, 50); // 50 bytes BSS
    O45File f2 = makeFile(w2);

    O45Linker linker;
    linker.addObject("a.o45", f1);
    linker.addObject("b.o45", f2);
    linker.setTextBase(0x2000);
    linker.setBssBase(0x4000);
    std::string err;
    auto result = linker.link(err, false);

    CHECK(!result.empty() && err.empty(), "BSS merge succeeds");
    auto& syms = linker.getSymbolMap();
    CHECK(syms.count("__bss_start") && syms.at("__bss_start") == 0x4000, "__bss_start at $4000");
    CHECK(syms.count("__bss_end") && syms.at("__bss_end") == 0x4096, "__bss_end at $4096");
}

// Test: Weak symbol overridden by strong
void test_weak_symbol_override() {
    O45Writer w1;
    std::vector<uint8_t> code1 = {0xEA, 0x60}; // NOP; RTS
    w1.setTextSegment(0, code1);
    w1.addExport("_func", SEG_TEXT, 0, true); // weak
    O45File f1 = makeFile(w1);

    O45Writer w2;
    std::vector<uint8_t> code2 = {0xA9, 0x42, 0x60}; // LDA #$42; RTS
    w2.setTextSegment(0, code2);
    w2.addExport("_func", SEG_TEXT, 0, false); // strong
    O45File f2 = makeFile(w2);

    O45Linker linker;
    linker.addObject("weak.o45", f1);
    linker.addObject("strong.o45", f2);
    linker.setTextBase(0x2000);
    std::string err;
    auto result = linker.link(err, false);

    CHECK(!result.empty() && err.empty(), "weak+strong link succeeds");
    auto& syms = linker.getSymbolMap();
    // Strong is in object 2, which starts after object 1's 2 bytes
    CHECK(syms.count("_func") && syms.at("_func") == 0x2002, "strong _func wins at $2002");
}

// Test: Unused global symbol warning
void test_unused_global_warning() {
    O45Writer w;
    std::vector<uint8_t> code = {0x60};
    w.setTextSegment(0, code);
    w.addExport("_main", SEG_TEXT, 0);
    w.addExport("_unused", SEG_TEXT, 0);
    O45File f = makeFile(w);

    O45Linker linker;
    linker.addObject("test.o45", f);
    linker.setTextBase(0x2000);
    std::ostringstream warns;
    linker.setWarningStream(&warns);
    std::string err;
    linker.link(err, false);

    std::string w_str = warns.str();
    CHECK(w_str.find("unused global symbol") != std::string::npos, "unused symbol warning emitted");
    CHECK(w_str.find("_unused") != std::string::npos, "warning names _unused");
}

// Test: Import suppresses unused warning
void test_import_suppresses_unused_warning() {
    O45Writer w1;
    std::vector<uint8_t> code1 = {0x20, 0x00, 0x00, 0x60}; // JSR; RTS
    w1.setTextSegment(0, code1);
    w1.addImport("_helper");
    w1.addExport("_main", SEG_TEXT, 0);
    std::vector<O45Reloc> relocs = {{1, R_WORD, SEG_EXTERNAL, 0, 0}};
    w1.setTextRelocations(O45RelocEncoder::encode(relocs));
    O45File f1 = makeFile(w1);

    O45Writer w2;
    std::vector<uint8_t> code2 = {0x60};
    w2.setTextSegment(0, code2);
    w2.addExport("_helper", SEG_TEXT, 0);
    O45File f2 = makeFile(w2);

    O45Linker linker;
    linker.addObject("main.o45", f1);
    linker.addObject("helper.o45", f2);
    linker.setTextBase(0x2000);
    std::ostringstream warns;
    linker.setWarningStream(&warns);
    std::string err;
    linker.link(err, false);

    CHECK(warns.str().find("_helper") == std::string::npos, "no warning for imported _helper");
}

// Test: Map file generation
void test_map_file_output() {
    O45Writer w;
    std::vector<uint8_t> code = {0xA9, 0x42, 0x60};
    w.setTextSegment(0, code);
    w.addExport("_main", SEG_TEXT, 0);
    O45File f = makeFile(w);

    O45Linker linker;
    linker.addObject("test.o45", f);
    linker.setTextBase(0x2000);
    std::string err;
    linker.link(err, false);

    std::ostringstream map;
    linker.writeMap(map);
    std::string mapStr = map.str();

    CHECK(mapStr.find("Linker Map") != std::string::npos, "map has header");
    CHECK(mapStr.find("TEXT") != std::string::npos, "map shows TEXT segment");
    CHECK(mapStr.find("_main") != std::string::npos, "map shows _main symbol");
    CHECK(mapStr.find("test.o45") != std::string::npos, "map shows source filename");
}

// Test: Auto data base placement after text
void test_auto_data_base_placement() {
    O45Writer w;
    std::vector<uint8_t> code(100, 0xEA); // 100 NOPs
    w.setTextSegment(0, code);
    std::vector<uint8_t> data = {0x01, 0x02};
    w.setDataSegment(0, data);
    w.addExport("_main", SEG_TEXT, 0);
    w.addExport("_mydata", SEG_DATA, 0);
    O45File f = makeFile(w);

    O45Linker linker;
    linker.addObject("test.o45", f);
    linker.setTextBase(0x2000);
    // Don't set dataBase — should auto-place after text
    std::string err;
    linker.link(err, false);

    auto& syms = linker.getSymbolMap();
    CHECK(syms.count("_mydata"), "_mydata symbol exists");
    // Data should be placed at textBase + textLen = $2000 + 100 = $2064
    CHECK(syms.at("_mydata") == 0x2064, "_mydata auto-placed at $2064");
}

// Test: ZP segment merge
void test_zp_segment_merge() {
    O45Writer w1;
    std::vector<uint8_t> code1 = {0x60};
    w1.setTextSegment(0, code1);
    w1.setZpSegment(0, 4); // 4 bytes ZP
    w1.addExport("_main", SEG_TEXT, 0);
    w1.addExport("_zp1", SEG_ZP, 0);
    O45File f1 = makeFile(w1);

    O45Writer w2;
    std::vector<uint8_t> code2 = {0x60};
    w2.setTextSegment(0, code2);
    w2.setZpSegment(0, 2); // 2 bytes ZP
    w2.addExport("_zp2", SEG_ZP, 0);
    O45File f2 = makeFile(w2);

    O45Linker linker;
    linker.addObject("a.o45", f1);
    linker.addObject("b.o45", f2);
    linker.setTextBase(0x2000);
    linker.setZpBase(0x10);
    std::string err;
    linker.link(err, false);

    auto& syms = linker.getSymbolMap();
    CHECK(syms.count("_zp1") && syms.at("_zp1") == 0x10, "_zp1 at ZP $10");
    CHECK(syms.count("_zp2") && syms.at("_zp2") == 0x14, "_zp2 at ZP $14");
}

// Test: PRG with data gap filling
void test_prg_data_gap() {
    O45Writer w;
    std::vector<uint8_t> code = {0xA9, 0x42, 0x60}; // 3 bytes
    w.setTextSegment(0, code);
    std::vector<uint8_t> data = {0xDE, 0xAD};
    w.setDataSegment(0, data);
    w.addExport("_main", SEG_TEXT, 0);
    O45File f = makeFile(w);

    O45Linker linker;
    linker.addObject("test.o45", f);
    linker.setTextBase(0x2000);
    linker.setDataBase(0x2010); // gap of 13 bytes after text
    std::string err;
    auto result = linker.link(err, true);

    CHECK(!result.empty() && err.empty(), "PRG with data gap succeeds");
    // header(2) + text(3) + gap(13) + data(2) = 20
    CHECK(result.size() == 20, "PRG size includes gap fill");
    CHECK(result[18] == 0xDE && result[19] == 0xAD, "data bytes at end");
}

// =============================================================================
// ROUND 2: Deeper linker coverage — relocation types, func attrs, call graph,
//          thunks, diagnostics, line maps, and map file detail
// =============================================================================

// Test: R_LINEAR24 relocation (3-byte address)
void test_reloc_r_linear24() {
    O45Writer w1;
    // pad byte + 3 patch bytes + RTS (offset 1 avoids delta=0 encoding issue)
    std::vector<uint8_t> code1 = {0xEA, 0x00, 0x00, 0x00, 0x60};
    w1.setTextSegment(0, code1);
    w1.addImport("_target");
    std::vector<O45Reloc> relocs = {{1, R_LINEAR24, SEG_EXTERNAL, 0, 0}};
    w1.setTextRelocations(O45RelocEncoder::encode(relocs));
    O45File f1 = makeFile(w1);

    O45Writer w2;
    std::vector<uint8_t> code2 = {0x60};
    w2.setTextSegment(0, code2);
    w2.addExport("_target", SEG_TEXT, 0);
    O45File f2 = makeFile(w2);

    O45Linker linker;
    linker.addObject("a.o45", f1);
    linker.addObject("b.o45", f2);
    linker.setTextBase(0x2000);
    std::string err;
    auto result = linker.link(err, false);

    CHECK(!result.empty() && err.empty(), "R_LINEAR24 link succeeds");
    // _target at $2005: lo=$05, mid=$20, hi=$00
    CHECK(result[1] == 0x05, "R_LINEAR24 byte 0");
    CHECK(result[2] == 0x20, "R_LINEAR24 byte 1");
    CHECK(result[3] == 0x00, "R_LINEAR24 byte 2");
}

// Test: R_LINEAR32 relocation (4-byte address)
void test_reloc_r_linear32() {
    O45Writer w1;
    std::vector<uint8_t> code1 = {0xEA, 0x00, 0x00, 0x00, 0x00, 0x60}; // pad + 4 patch + RTS
    w1.setTextSegment(0, code1);
    w1.addImport("_target");
    std::vector<O45Reloc> relocs = {{1, R_LINEAR32, SEG_EXTERNAL, 0, 0}};
    w1.setTextRelocations(O45RelocEncoder::encode(relocs));
    O45File f1 = makeFile(w1);

    O45Writer w2;
    std::vector<uint8_t> code2 = {0x60};
    w2.setTextSegment(0, code2);
    w2.addExport("_target", SEG_TEXT, 0);
    O45File f2 = makeFile(w2);

    O45Linker linker;
    linker.addObject("a.o45", f1);
    linker.addObject("b.o45", f2);
    linker.setTextBase(0x2000);
    std::string err;
    auto result = linker.link(err, false);

    CHECK(!result.empty() && err.empty(), "R_LINEAR32 link succeeds");
    // _target at $2006
    CHECK(result[1] == 0x06, "R_LINEAR32 byte 0");
    CHECK(result[2] == 0x20, "R_LINEAR32 byte 1");
    CHECK(result[3] == 0x00, "R_LINEAR32 byte 2");
    CHECK(result[4] == 0x00, "R_LINEAR32 byte 3");
}

// Test: R_SEGADR relocation (3-byte segment address)
void test_reloc_r_segadr() {
    O45Writer w1;
    std::vector<uint8_t> code1 = {0xEA, 0x00, 0x00, 0x00, 0x60}; // pad + 3 patch + RTS
    w1.setTextSegment(0, code1);
    w1.addImport("_target");
    std::vector<O45Reloc> relocs = {{1, R_SEGADR, SEG_EXTERNAL, 0, 0}};
    w1.setTextRelocations(O45RelocEncoder::encode(relocs));
    O45File f1 = makeFile(w1);

    O45Writer w2;
    std::vector<uint8_t> code2 = {0x60};
    w2.setTextSegment(0, code2);
    w2.addExport("_target", SEG_TEXT, 0);
    O45File f2 = makeFile(w2);

    O45Linker linker;
    linker.addObject("a.o45", f1);
    linker.addObject("b.o45", f2);
    linker.setTextBase(0x2000);
    std::string err;
    auto result = linker.link(err, false);

    CHECK(!result.empty() && err.empty(), "R_SEGADR link succeeds");
    // _target at $002005: lo=$05, hi=$20, bank=$00
    CHECK(result[1] == 0x05, "R_SEGADR lo byte");
    CHECK(result[2] == 0x20, "R_SEGADR hi byte");
    CHECK(result[3] == 0x00, "R_SEGADR bank byte");
}

// Test: Function attributes extracted from exports
void test_func_attrs_extraction() {
    O45Writer w;
    std::vector<uint8_t> code = {0xA9, 0x42, 0x60}; // LDA #$42; RTS
    w.setTextSegment(0, code);
    w.addExport("_myfunc", SEG_TEXT, 0);
    O45FuncAttr attr;
    attr.flags = FUNC_FLAG_ZP_CONV | FUNC_FLAG_LEAF;
    attr.regClobbers = 0x03; // A, X
    attr.flagClobbers = 0x01; // C
    attr.zpUses = 0x07; // bits 0-2
    attr.zpClobbers = 0x03; // bits 0-1
    attr.paramSize = 4;
    w.setFuncAttr("_myfunc", attr);
    O45File f = makeFile(w);

    O45Linker linker;
    linker.addObject("test.o45", f);
    linker.setTextBase(0x2000);
    std::ostringstream warns;
    linker.setWarningStream(&warns);
    std::string err;
    linker.link(err, false);

    auto& attrs = linker.getFuncAttrs();
    CHECK(attrs.count("_myfunc"), "func attr extracted for _myfunc");
    if (attrs.count("_myfunc")) {
        CHECK(attrs.at("_myfunc").flags == (FUNC_FLAG_ZP_CONV | FUNC_FLAG_LEAF), "flags match");
        CHECK(attrs.at("_myfunc").regClobbers == 0x03, "regClobbers match");
        CHECK(attrs.at("_myfunc").paramSize == 4, "paramSize match");
    }
}

// Test: Call graph discovery from JSR relocations
void test_call_graph_discovery() {
    // Object 1: _main contains JSR to external _helper
    O45Writer w1;
    std::vector<uint8_t> code1 = {0x20, 0x00, 0x00, 0x60}; // JSR $0000; RTS
    w1.setTextSegment(0, code1);
    w1.addExport("_main", SEG_TEXT, 0);
    w1.addImport("_helper");
    O45FuncAttr mainAttr;
    mainAttr.flags = 0;
    mainAttr.regClobbers = 0x0F;
    w1.setFuncAttr("_main", mainAttr);
    std::vector<O45Reloc> relocs = {{1, R_WORD, SEG_EXTERNAL, 0, 0}};
    w1.setTextRelocations(O45RelocEncoder::encode(relocs));
    O45File f1 = makeFile(w1);

    // Object 2: exports _helper
    O45Writer w2;
    std::vector<uint8_t> code2 = {0xA9, 0x00, 0x60};
    w2.setTextSegment(0, code2);
    w2.addExport("_helper", SEG_TEXT, 0);
    O45FuncAttr helperAttr;
    helperAttr.flags = 0;
    helperAttr.regClobbers = 0x01; // A only
    w2.setFuncAttr("_helper", helperAttr);
    O45File f2 = makeFile(w2);

    O45Linker linker;
    linker.addObject("main.o45", f1);
    linker.addObject("helper.o45", f2);
    linker.setTextBase(0x2000);
    std::ostringstream warns;
    linker.setWarningStream(&warns);
    std::string err;
    linker.link(err, false);

    auto& cg = linker.getCallGraph();
    CHECK(cg.count("_main"), "call graph has _main");
    if (cg.count("_main")) {
        CHECK(cg.at("_main").count("_helper"), "_main calls _helper");
    }
}

// Test: Transitive clobber propagation through call chain
void test_transitive_clobbers() {
    // _main calls _func_a, _func_a calls _func_b
    // _func_a clobbers A, _func_b clobbers X
    // → transitive clobbers for _main should include A|X

    // Object 1: _main → JSR _func_a
    O45Writer w1;
    std::vector<uint8_t> code1 = {0x20, 0x00, 0x00, 0x60};
    w1.setTextSegment(0, code1);
    w1.addExport("_main", SEG_TEXT, 0);
    w1.addImport("_func_a");
    O45FuncAttr mainAttr; mainAttr.regClobbers = 0;
    w1.setFuncAttr("_main", mainAttr);
    std::vector<O45Reloc> r1 = {{1, R_WORD, SEG_EXTERNAL, 0, 0}};
    w1.setTextRelocations(O45RelocEncoder::encode(r1));
    O45File f1 = makeFile(w1);

    // Object 2: _func_a → JSR _func_b
    O45Writer w2;
    std::vector<uint8_t> code2 = {0x20, 0x00, 0x00, 0x60};
    w2.setTextSegment(0, code2);
    w2.addExport("_func_a", SEG_TEXT, 0);
    w2.addImport("_func_b");
    O45FuncAttr faAttr; faAttr.regClobbers = 0x01; // A
    w2.setFuncAttr("_func_a", faAttr);
    std::vector<O45Reloc> r2 = {{1, R_WORD, SEG_EXTERNAL, 0, 0}};
    w2.setTextRelocations(O45RelocEncoder::encode(r2));
    O45File f2 = makeFile(w2);

    // Object 3: _func_b (leaf)
    O45Writer w3;
    std::vector<uint8_t> code3 = {0xA9, 0x42, 0x60};
    w3.setTextSegment(0, code3);
    w3.addExport("_func_b", SEG_TEXT, 0);
    O45FuncAttr fbAttr; fbAttr.regClobbers = 0x02; // X
    w3.setFuncAttr("_func_b", fbAttr);
    O45File f3 = makeFile(w3);

    O45Linker linker;
    linker.addObject("main.o45", f1);
    linker.addObject("fa.o45", f2);
    linker.addObject("fb.o45", f3);
    linker.setTextBase(0x2000);
    std::ostringstream warns;
    linker.setWarningStream(&warns);
    std::string err;
    linker.link(err, false);

    auto& tc = linker.getTransitiveClobbers();
    CHECK(tc.count("_main"), "transitive clobbers has _main");
    if (tc.count("_main")) {
        // _main transitively clobbers A (from _func_a) and X (from _func_b)
        CHECK((tc.at("_main").regClobbers & 0x03) == 0x03, "_main transitive clobbers A|X");
    }
}

// Test: Map file shows BSS, ZP, data, and per-object contributions
void test_map_file_all_segments() {
    O45Writer w;
    std::vector<uint8_t> code = {0x60};
    w.setTextSegment(0, code);
    std::vector<uint8_t> data = {0xAA, 0xBB};
    w.setDataSegment(0, data);
    w.setBssSegment(0, 64);
    w.setZpSegment(0, 4);
    w.addExport("_main", SEG_TEXT, 0);
    O45File f = makeFile(w);

    O45Linker linker;
    linker.addObject("full.o45", f);
    linker.setTextBase(0x2000);
    linker.setDataBase(0x3000);
    linker.setBssBase(0x4000);
    linker.setZpBase(0x10);
    std::string err;
    linker.link(err, false);

    std::ostringstream map;
    linker.writeMap(map);
    std::string m = map.str();

    CHECK(m.find("TEXT") != std::string::npos, "map shows TEXT");
    CHECK(m.find("DATA") != std::string::npos, "map shows DATA");
    CHECK(m.find("BSS") != std::string::npos, "map shows BSS");
    CHECK(m.find("ZP") != std::string::npos, "map shows ZP");
    CHECK(m.find("full.o45") != std::string::npos, "map shows object filename");
}

// Test: Map file shows function attributes
void test_map_file_func_attrs() {
    O45Writer w;
    std::vector<uint8_t> code = {0x60};
    w.setTextSegment(0, code);
    w.addExport("_myfunc", SEG_TEXT, 0);
    O45FuncAttr attr;
    attr.flags = FUNC_FLAG_ZP_CONV;
    attr.regClobbers = 0x03; // A, X
    attr.zpUses = 0x01;
    attr.zpClobbers = 0x01;
    w.setFuncAttr("_myfunc", attr);
    O45File f = makeFile(w);

    O45Linker linker;
    linker.addObject("test.o45", f);
    linker.setTextBase(0x2000);
    std::ostringstream warns;
    linker.setWarningStream(&warns);
    std::string err;
    linker.link(err, false);

    std::ostringstream map;
    linker.writeMap(map);
    std::string m = map.str();

    CHECK(m.find("uses:") != std::string::npos, "map shows uses: field");
    CHECK(m.find("clob:") != std::string::npos, "map shows clob: field");
    CHECK(m.find("regs:") != std::string::npos, "map shows regs: field");
    CHECK(m.find("AX") != std::string::npos, "map shows clobbered regs AX");
}

// Test: Map file shows call graph
void test_map_file_call_graph() {
    O45Writer w1;
    std::vector<uint8_t> code1 = {0x20, 0x00, 0x00, 0x60};
    w1.setTextSegment(0, code1);
    w1.addExport("_caller", SEG_TEXT, 0);
    w1.addImport("_callee");
    O45FuncAttr a1; a1.regClobbers = 0x01;
    w1.setFuncAttr("_caller", a1);
    std::vector<O45Reloc> relocs = {{1, R_WORD, SEG_EXTERNAL, 0, 0}};
    w1.setTextRelocations(O45RelocEncoder::encode(relocs));
    O45File f1 = makeFile(w1);

    O45Writer w2;
    std::vector<uint8_t> code2 = {0x60};
    w2.setTextSegment(0, code2);
    w2.addExport("_callee", SEG_TEXT, 0);
    O45FuncAttr a2; a2.regClobbers = 0x01;
    w2.setFuncAttr("_callee", a2);
    O45File f2 = makeFile(w2);

    O45Linker linker;
    linker.addObject("a.o45", f1);
    linker.addObject("b.o45", f2);
    linker.setTextBase(0x2000);
    std::ostringstream warns;
    linker.setWarningStream(&warns);
    std::string err;
    linker.link(err, false);

    std::ostringstream map;
    linker.writeMap(map);
    std::string m = map.str();

    CHECK(m.find("Call Graph") != std::string::npos, "map has Call Graph section");
    CHECK(m.find("_caller") != std::string::npos, "map shows _caller");
    CHECK(m.find("_callee") != std::string::npos, "map shows _callee");
}

// Test: Convention mismatch diagnostics
void test_convention_mismatch_diagnostic() {
    // _stack_func (stack conv) calls _zp_func (zp conv) — mismatch
    O45Writer w1;
    std::vector<uint8_t> code1 = {0x20, 0x00, 0x00, 0x60};
    w1.setTextSegment(0, code1);
    w1.addExport("_stack_func", SEG_TEXT, 0);
    w1.addImport("_zp_func");
    O45FuncAttr a1; a1.flags = 0; // stack convention
    a1.regClobbers = 0x01;
    w1.setFuncAttr("_stack_func", a1);
    std::vector<O45Reloc> relocs = {{1, R_WORD, SEG_EXTERNAL, 0, 0}};
    w1.setTextRelocations(O45RelocEncoder::encode(relocs));
    O45File f1 = makeFile(w1);

    O45Writer w2;
    std::vector<uint8_t> code2 = {0x60};
    w2.setTextSegment(0, code2);
    w2.addExport("_zp_func", SEG_TEXT, 0);
    O45FuncAttr a2; a2.flags = FUNC_FLAG_ZP_CONV; // zp convention
    a2.regClobbers = 0x01;
    a2.paramSize = 2;
    w2.setFuncAttr("_zp_func", a2);
    O45File f2 = makeFile(w2);

    // Test THUNK_WARN mode
    O45Linker linker;
    linker.addObject("stack.o45", f1);
    linker.addObject("zp.o45", f2);
    linker.setTextBase(0x2000);
    linker.setThunkMode(O45Linker::THUNK_WARN);
    std::ostringstream warns;
    linker.setWarningStream(&warns);
    std::string err;
    auto result = linker.link(err, false);

    CHECK(!result.empty(), "THUNK_WARN link succeeds");
    CHECK(warns.str().find("calling convention mismatch") != std::string::npos, "mismatch warning emitted");
}

// Test: Convention mismatch in THUNK_ERROR mode
void test_convention_mismatch_error() {
    O45Writer w1;
    std::vector<uint8_t> code1 = {0x20, 0x00, 0x00, 0x60};
    w1.setTextSegment(0, code1);
    w1.addExport("_stack_func", SEG_TEXT, 0);
    w1.addImport("_zp_func");
    O45FuncAttr a1; a1.flags = 0;
    a1.regClobbers = 0x01;
    w1.setFuncAttr("_stack_func", a1);
    std::vector<O45Reloc> relocs = {{1, R_WORD, SEG_EXTERNAL, 0, 0}};
    w1.setTextRelocations(O45RelocEncoder::encode(relocs));
    O45File f1 = makeFile(w1);

    O45Writer w2;
    std::vector<uint8_t> code2 = {0x60};
    w2.setTextSegment(0, code2);
    w2.addExport("_zp_func", SEG_TEXT, 0);
    O45FuncAttr a2; a2.flags = FUNC_FLAG_ZP_CONV;
    a2.regClobbers = 0x01;
    a2.paramSize = 2;
    w2.setFuncAttr("_zp_func", a2);
    O45File f2 = makeFile(w2);

    O45Linker linker;
    linker.addObject("stack.o45", f1);
    linker.addObject("zp.o45", f2);
    linker.setTextBase(0x2000);
    linker.setThunkMode(O45Linker::THUNK_ERROR);
    std::string err;
    auto result = linker.link(err, false);

    CHECK(result.empty(), "THUNK_ERROR link fails");
    CHECK(err.find("calling convention mismatch") != std::string::npos, "error mentions mismatch");
}

// Test: Stack-to-ZP thunk generation (THUNK_AUTO)
void test_thunk_s2z_generation() {
    O45Writer w1;
    std::vector<uint8_t> code1 = {0x20, 0x00, 0x00, 0x60};
    w1.setTextSegment(0, code1);
    w1.addExport("_stack_caller", SEG_TEXT, 0);
    w1.addImport("_zp_callee");
    O45FuncAttr a1; a1.flags = 0; // stack
    a1.regClobbers = 0x01;
    w1.setFuncAttr("_stack_caller", a1);
    std::vector<O45Reloc> relocs = {{1, R_WORD, SEG_EXTERNAL, 0, 0}};
    w1.setTextRelocations(O45RelocEncoder::encode(relocs));
    O45File f1 = makeFile(w1);

    O45Writer w2;
    std::vector<uint8_t> code2 = {0x60};
    w2.setTextSegment(0, code2);
    w2.addExport("_zp_callee", SEG_TEXT, 0);
    O45FuncAttr a2; a2.flags = FUNC_FLAG_ZP_CONV;
    a2.regClobbers = 0x01;
    a2.paramSize = 2;
    w2.setFuncAttr("_zp_callee", a2);
    O45File f2 = makeFile(w2);

    O45Linker linker;
    linker.addObject("s.o45", f1);
    linker.addObject("z.o45", f2);
    linker.setTextBase(0x2000);
    linker.setThunkMode(O45Linker::THUNK_AUTO);
    std::ostringstream warns;
    linker.setWarningStream(&warns);
    std::string err;
    auto result = linker.link(err, false);

    CHECK(!result.empty() && err.empty(), "s2z thunk link succeeds");
    auto& syms = linker.getSymbolMap();
    CHECK(syms.count("__thunk_s2z__zp_callee"), "s2z thunk symbol created");
    // Thunk should be appended after original code
    CHECK(result.size() > 5, "binary larger than original code (thunk appended)");
}

// Test: ZP-to-stack thunk generation
void test_thunk_z2s_generation() {
    O45Writer w1;
    std::vector<uint8_t> code1 = {0x20, 0x00, 0x00, 0x60};
    w1.setTextSegment(0, code1);
    w1.addExport("_zp_caller", SEG_TEXT, 0);
    w1.addImport("_stack_callee");
    O45FuncAttr a1; a1.flags = FUNC_FLAG_ZP_CONV; // zp
    a1.regClobbers = 0x01;
    w1.setFuncAttr("_zp_caller", a1);
    std::vector<O45Reloc> relocs = {{1, R_WORD, SEG_EXTERNAL, 0, 0}};
    w1.setTextRelocations(O45RelocEncoder::encode(relocs));
    O45File f1 = makeFile(w1);

    O45Writer w2;
    std::vector<uint8_t> code2 = {0x60};
    w2.setTextSegment(0, code2);
    w2.addExport("_stack_callee", SEG_TEXT, 0);
    O45FuncAttr a2; a2.flags = 0; // stack
    a2.regClobbers = 0x01;
    a2.paramSize = 3;
    w2.setFuncAttr("_stack_callee", a2);
    O45File f2 = makeFile(w2);

    O45Linker linker;
    linker.addObject("z.o45", f1);
    linker.addObject("s.o45", f2);
    linker.setTextBase(0x2000);
    linker.setThunkMode(O45Linker::THUNK_AUTO);
    std::ostringstream warns;
    linker.setWarningStream(&warns);
    std::string err;
    auto result = linker.link(err, false);

    CHECK(!result.empty() && err.empty(), "z2s thunk link succeeds");
    auto& syms = linker.getSymbolMap();
    CHECK(syms.count("__thunk_z2s__stack_callee"), "z2s thunk symbol created");
    CHECK(result.size() > 5, "binary larger (thunk appended)");
}

// Test: Line map merge and JSON output
void test_line_map_output() {
    O45Writer w;
    std::vector<uint8_t> code = {0xA9, 0x42, 0x60};
    w.setTextSegment(0, code);
    w.addExport("_main", SEG_TEXT, 0);
    O45File f = makeFile(w);

    // Manually add line info
    f.lineFiles.push_back("test.c");
    f.lineInfos.push_back({0, 0, 10}); // offset 0, file 0, line 10
    f.lineInfos.push_back({2, 0, 12}); // offset 2, file 0, line 12

    O45Linker linker;
    linker.addObject("test.o45", f);
    linker.setTextBase(0x2000);
    std::ostringstream warns;
    linker.setWarningStream(&warns);
    std::string err;
    linker.link(err, false);

    CHECK(linker.hasLineMap(), "line map populated");

    std::ostringstream json;
    linker.writeLineMap(json);
    std::string j = json.str();
    CHECK(j.find("\"addr\"") != std::string::npos, "JSON has addr field");
    CHECK(j.find("\"file\"") != std::string::npos, "JSON has file field");
    CHECK(j.find("\"line\"") != std::string::npos, "JSON has line field");
    CHECK(j.find("test.c") != std::string::npos, "JSON has filename");
}

// Test: Data relocation (relocs in data segment)
void test_data_relocation() {
    O45Writer w1;
    std::vector<uint8_t> code1 = {0x60};
    w1.setTextSegment(0, code1);
    // Data: pad byte + pointer to _func (offset 1 avoids delta=0)
    std::vector<uint8_t> data1 = {0xFF, 0x00, 0x00};
    w1.setDataSegment(0, data1);
    w1.addExport("_main", SEG_TEXT, 0);
    w1.addImport("_func");
    std::vector<O45Reloc> drelocs = {{1, R_WORD, SEG_EXTERNAL, 0, 0}};
    w1.setDataRelocations(O45RelocEncoder::encode(drelocs));
    O45File f1 = makeFile(w1);

    O45Writer w2;
    std::vector<uint8_t> code2 = {0xA9, 0xFF, 0x60};
    w2.setTextSegment(0, code2);
    w2.addExport("_func", SEG_TEXT, 0);
    O45File f2 = makeFile(w2);

    O45Linker linker;
    linker.addObject("main.o45", f1);
    linker.addObject("func.o45", f2);
    linker.setTextBase(0x2000);
    linker.setDataBase(0x3000);
    std::string err;
    auto result = linker.link(err, true);

    CHECK(!result.empty() && err.empty(), "data relocation link succeeds");
    // _func at $2001 (after main's 1 byte). Data at $3000.
    // header(2) + text(4) + gap to $3000 + data(3)
    size_t dataOff = 2 + (0x3000 - 0x2000); // header + (gap from textEnd to dataBase)
    CHECK(result.size() > dataOff + 2, "result large enough for data");
    // Byte at data+1 should be patched: lo byte of _func ($01)
    CHECK(result[dataOff + 1] == 0x01, "data reloc low byte");
    CHECK(result[dataOff + 2] == 0x20, "data reloc high byte");
}

// (segmentBase is private — verified indirectly via symbol addresses)

// =============================================================================
// Main entry point
// =============================================================================

int main() {
    printf("Running linker validation tests...\n\n");

    // Error condition tests
    test_no_input_objects();
    test_duplicate_symbol();
    test_undefined_symbol_import();
    test_relocation_offset_out_of_range();
    test_invalid_symbol_index();
    test_undefined_symbol_in_relocation();
    test_relocation_patch_overflow();
    test_unknown_relocation_type();

    // Success-path tests (round 1)
    test_single_object_prg();
    test_single_object_flat();
    test_two_objects_merge();
    test_reloc_r_word();
    test_reloc_r_low();
    test_reloc_r_high();
    test_reloc_internal_text();
    test_data_segment_merge();
    test_bss_merge_and_symbols();
    test_weak_symbol_override();
    test_unused_global_warning();
    test_import_suppresses_unused_warning();
    test_map_file_output();
    test_auto_data_base_placement();
    test_zp_segment_merge();
    test_prg_data_gap();

    // Round 2: deeper coverage
    test_reloc_r_linear24();
    test_reloc_r_linear32();
    test_reloc_r_segadr();
    test_func_attrs_extraction();
    test_call_graph_discovery();
    test_transitive_clobbers();
    test_map_file_all_segments();
    test_map_file_func_attrs();
    test_map_file_call_graph();
    test_convention_mismatch_diagnostic();
    test_convention_mismatch_error();
    test_thunk_s2z_generation();
    test_thunk_z2s_generation();
    test_line_map_output();
    test_data_relocation();

    printf("\n=== Linker Validation Tests ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    if (tests_failed == 0) {
        printf("All tests passed!\n");
        return 0;
    } else {
        printf("Some tests failed!\n");
        return 1;
    }
}
