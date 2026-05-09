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
// Main entry point
// =============================================================================

int main() {
    printf("Running linker error condition validation tests...\n\n");

    test_no_input_objects();
    test_duplicate_symbol();
    test_undefined_symbol_import();
    test_relocation_offset_out_of_range();
    test_invalid_symbol_index();
    test_undefined_symbol_in_relocation();
    test_relocation_patch_overflow();
    test_unknown_relocation_type();

    printf("\n=== Test Results ===\n");
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
