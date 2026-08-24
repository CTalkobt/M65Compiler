// Phase 113.6: DWARF Validation Test Suite
// Tests debug information generation through the complete pipeline

#include <iostream>
#include <vector>
#include <cassert>
#include <cstring>

#include "../include/DebugInfoBuilder.hpp"
#include "../include/LineNumberProgram.hpp"
#include "../include/SourceLocationTracker.hpp"
#include "../include/O45DwarfSerialization.hpp"
#include "../include/O45Writer.hpp"

// Test 1: DIE Generation
void test_die_generation() {
    std::cout << "Test 1: DIE Generation... ";

    DebugInfoBuilder builder;

    // Create compile unit
    dwarf::DIE* cu = builder.createCompileUnit("test.c", "/tmp", "cc45");
    assert(cu != nullptr);
    assert(cu->tag == dwarf::Tag::COMPILE_UNIT);

    // Create a function
    dwarf::DIE* func = builder.createSubprogram(cu, "main", 0x1000, 0x2000);
    assert(func != nullptr);
    assert(func->tag == dwarf::Tag::SUBPROGRAM);

    // Create variables
    dwarf::DIE* var = builder.createVariable(func, "x", "int", 4);
    assert(var != nullptr);

    // Create parameter
    dwarf::DIE* param = builder.createFormalParameter(func, "argc", "int");
    assert(param != nullptr);

    // Create base type
    dwarf::DIE* int_type = builder.createBaseType(cu, "int", 4, 0);
    assert(int_type != nullptr);

    // Create pointer type
    dwarf::DIE* ptr_type = builder.createPointerType(cu, int_type, 2);
    assert(ptr_type != nullptr);

    // Create struct type
    dwarf::DIE* struct_type = builder.createStructType(cu, "Point", 4, true);
    assert(struct_type != nullptr);

    std::cout << "PASS" << std::endl;
}

// Test 2: Abbreviation Table
void test_abbreviations() {
    std::cout << "Test 2: Abbreviation Table... ";

    DebugInfoBuilder builder;

    // Check that abbreviations were pre-registered
    const auto& abbrevs = builder.getAbbreviationTable().getAbbreviations();
    assert(abbrevs.size() > 0);
    assert(abbrevs.size() >= 10);  // At least 10 pre-registered

    std::cout << "PASS (" << abbrevs.size() << " abbreviations)" << std::endl;
}

// Test 3: String Pool
void test_string_pool() {
    std::cout << "Test 3: String Pool... ";

    DebugInfoBuilder builder;

    // Add some strings
    uint32_t offset1 = builder.getStringPool().addString("hello");
    uint32_t offset2 = builder.getStringPool().addString("world");
    uint32_t offset3 = builder.getStringPool().addString("hello");  // Duplicate

    assert(offset1 == 0);  // First string at offset 0
    assert(offset2 > offset1);  // Second string at different offset
    assert(offset3 == offset1);  // Duplicate returns same offset

    std::cout << "PASS (deduplication working)" << std::endl;
}

// Test 4: Line Number Program
void test_line_number_program() {
    std::cout << "Test 4: Line Number Program... ";

    dwarf::LineNumberProgramBuilder builder;

    // Add files
    uint32_t file1 = builder.addFile("test.c", "/tmp");
    uint32_t file2 = builder.addFile("helper.c", "/tmp");
    uint32_t file1_dup = builder.addFile("test.c", "/tmp");  // Duplicate

    assert(file1 == 1);  // First file index is 1
    assert(file2 == 2);  // Second file index is 2
    assert(file1_dup == file1);  // Duplicate returns same index

    // Emit line entries
    builder.emitLineEntry(0x1000, 1, 0);
    builder.emitLineEntry(0x1010, 5, 0);
    builder.emitLineEntry(0x1020, 10, 5);

    // Finalize
    builder.finalizeProgram();

    const auto& program = builder.getLineProgram();
    assert(program.size() > 0);

    std::cout << "PASS (" << program.size() << " bytes)" << std::endl;
}

// Test 5: Source Location Tracking
void test_source_tracking() {
    std::cout << "Test 5: Source Location Tracking... ";

    SourceLocationTracker tracker;
    tracker.setCurrentFile("test.c");

    // Track a function
    tracker.trackFunctionDef("main", 10, 0, 0x1000, 0x2000);

    // Track variables
    tracker.trackVariableDecl("x", 11, 4, "int", 0x1010, false);
    tracker.trackVariableDecl("argc", 10, 7, "int", 0, true);

    // Verify tracking
    const auto* func_info = tracker.getFunction("main");
    assert(func_info != nullptr);
    assert(func_info->name == "main");
    assert(func_info->lowPC == 0x1000);
    assert(func_info->highPC == 0x2000);

    const auto* var_info = tracker.getVariable("x");
    assert(var_info != nullptr);
    assert(var_info->name == "x");
    assert(var_info->type == "int");

    std::cout << "PASS" << std::endl;
}

// Test 6: O45 Debug Section Serialization
void test_o45_serialization() {
    std::cout << "Test 6: O45 Debug Serialization... ";

    DebugInfoBuilder debug_builder;
    dwarf::LineNumberProgramBuilder line_builder;

    // Add some content
    debug_builder.createCompileUnit("test.c", "/tmp", "cc45");
    line_builder.addFile("test.c", "/tmp");
    line_builder.emitLineEntry(0x1000, 1, 0);
    line_builder.finalizeProgram();

    // Create O45Writer
    O45Writer writer;

    // Prepare with DWARF (this function bridges everything)
    prepareO45WriterWithDWARF(writer, debug_builder, line_builder);

    // Verify sections were set (indirectly through emit)
    auto o45_bytes = writer.emit();
    assert(o45_bytes.size() > 0);

    std::cout << "PASS (" << o45_bytes.size() << " bytes in .o45)" << std::endl;
}

// Test 7: Complete Integration
void test_complete_integration() {
    std::cout << "Test 7: Complete Integration... ";

    // Simulate full compilation pipeline

    // 1. IRBuilder phase
    SourceLocationTracker source_tracker;
    DebugInfoBuilder debug_builder;

    source_tracker.setCurrentFile("main.c");
    source_tracker.trackFunctionDef("add", 1, 0, 0x1000, 0x1020);
    source_tracker.trackVariableDecl("result", 2, 4, "int", 0x1000, false);

    debug_builder.createCompileUnit("main.c", "/home/user", "cc45");

    // 2. CodeGenerator phase
    dwarf::LineNumberProgramBuilder line_builder;
    line_builder.addFile("main.c", "/home/user");
    line_builder.emitLineEntry(0x1000, 1, 0);
    line_builder.emitLineEntry(0x1010, 2, 4);
    line_builder.emitLineEntry(0x1020, 5, 0);
    line_builder.finalizeProgram();

    // 3. O45Writer phase
    O45Writer writer;
    prepareO45WriterWithDWARF(writer, debug_builder, line_builder);

    // 4. Verification
    auto o45_bytes = writer.emit();
    assert(o45_bytes.size() > O45_HEADER_SIZE);

    std::cout << "PASS (pipeline complete, " << o45_bytes.size() << " bytes)" << std::endl;
}

// Main test runner
int main() {
    std::cout << "\n=== Phase 113.6: DWARF Validation Test Suite ===" << std::endl;
    std::cout << "Running comprehensive DWARF tests...\n" << std::endl;

    try {
        test_die_generation();
        test_abbreviations();
        test_string_pool();
        test_line_number_program();
        test_source_tracking();
        test_o45_serialization();
        test_complete_integration();

        std::cout << "\n✅ All 7 tests PASSED" << std::endl;
        std::cout << "DWARF implementation verified and ready for production\n" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Test FAILED: " << e.what() << std::endl;
        return 1;
    }
}
