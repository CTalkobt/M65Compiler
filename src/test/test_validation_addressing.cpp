#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

static int tests_passed = 0;
static int tests_failed = 0;

#define CHECK(cond, msg) do { \
    if (!(cond)) { \
        printf("FAIL: %s\n", msg); \
        tests_failed++; \
    } else { \
        printf("PASS: %s\n", msg); \
        tests_passed++; \
    } \
} while(0)

// Helper: write assembly code to temp file
static void writeAssemblyFile(const std::string& asmCode) {
    system("mkdir -p build");
    std::ofstream asm_file("build/test_validation_addressing_temp.s");
    asm_file << asmCode;
}

// Helper: read file content into string
static std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    return std::string((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
}

// Helper: read binary file as vector of bytes
static std::vector<uint8_t> readBinaryFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());
}

// Test helper: assemble code using ca45 and return stderr or stdout
static std::string assembleCode(const std::string& asmCode) {
    writeAssemblyFile(asmCode);
    system("./bin/ca45 build/test_validation_addressing_temp.s -o build/test_validation_addressing_temp.bin 2>build/test_validation_addressing_temp.err 1>build/test_validation_addressing_temp.out");

    // Try stderr first, fall back to stdout if empty
    std::string error_output = readFile("build/test_validation_addressing_temp.err");
    if (error_output.empty()) {
        error_output = readFile("build/test_validation_addressing_temp.out");
    }
    return error_output;
}

// Test helper: check if assembly succeeded
static bool assemblySucceeded(const std::string& asmCode) {
    writeAssemblyFile(asmCode);
    int ret = system("./bin/ca45 build/test_validation_addressing_temp.s -o build/test_validation_addressing_temp.bin 2>/dev/null");
    return ret == 0;
}

// Test 1: Base-page with immediate load
void test_immediate_basic() {
    std::string code = R"(.org $2000
lda #$42
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, "lda #$42 immediate assembles");

    // Verify binary output: 2 bytes (opcode + immediate value)
    auto bin = readBinaryFile("build/test_validation_addressing_temp.bin");
    bool correct_size = (bin.size() == 2);
    CHECK(correct_size, "lda #$42 produces 2 bytes (immediate)");
}

// Test 2: Base-page direct
void test_basepage_direct() {
    std::string code = R"(.org $2000
lda $80
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, "lda $80 base-page assembles");

    // Verify binary output: 2 bytes (opcode + zero-page address)
    auto bin = readBinaryFile("build/test_validation_addressing_temp.bin");
    bool correct_size = (bin.size() == 2);
    CHECK(correct_size, "lda $80 produces 2 bytes (base-page direct)");
}

// Test 3: Absolute addressing
void test_absolute_direct() {
    std::string code = R"(.org $2000
lda $2080
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, "lda $2080 absolute assembles");

    // Verify binary output: 3 bytes (opcode + 16-bit address in little-endian)
    auto bin = readBinaryFile("build/test_validation_addressing_temp.bin");
    bool correct_size = (bin.size() == 3);
    CHECK(correct_size, "lda $2080 produces 3 bytes (absolute direct)");
}

// Test 4: Base-page with Y indexing (indirect indexed)
void test_indirect_indexed_basic() {
    std::string code = R"(.org $2000
lda ($20), Y
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, "lda ($20), Y indirect indexed assembles");

    // Verify binary output: 2 bytes (opcode + zero-page address)
    auto bin = readBinaryFile("build/test_validation_addressing_temp.bin");
    bool correct_size = (bin.size() == 2);
    CHECK(correct_size, "lda ($20), Y produces 2 bytes (indirect indexed)");
}

// Test 5: Zero-page with boundary at $FF/$100
void test_auto_zp_boundary() {
    std::string code = R"(.org $2000
lda $FF
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, "lda $FF (boundary) auto-selects zero-page and assembles");

    // Verify binary output: 2 bytes for zero-page addressing
    auto bin = readBinaryFile("build/test_validation_addressing_temp.bin");
    bool correct_size = (bin.size() == 2);
    CHECK(correct_size, "lda $FF auto-selects zero-page (2 bytes total)");
}

// Test 6: Absolute addressing with larger address
void test_auto_abs_large_address() {
    std::string code = R"(.org $2000
lda $1000
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, "lda $1000 (large address) auto-selects absolute and assembles");

    // Verify binary output: 3 bytes for absolute addressing
    auto bin = readBinaryFile("build/test_validation_addressing_temp.bin");
    bool correct_size = (bin.size() == 3);
    CHECK(correct_size, "lda $1000 auto-selects absolute (3 bytes total)");
}

// Test 7: Indexed indirect addressing (zp,X)
void test_indexed_indirect_x() {
    std::string code = R"(.org $2000
lda ($10, X)
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, "lda ($10, X) indexed indirect addressing assembles");

    // Verify binary output: 2 bytes (opcode + zero-page address)
    auto bin = readBinaryFile("build/test_validation_addressing_temp.bin");
    bool correct_size = (bin.size() == 2);
    CHECK(correct_size, "lda ($10, X) produces 2 bytes");
}

// Test 8: Indirect indexed addressing (zp),Y
void test_indirect_indexed_y() {
    std::string code = R"(.org $2000
lda ($10), Y
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, "lda ($10), Y indirect indexed addressing assembles");

    // Verify binary output: 2 bytes (opcode + zero-page address)
    auto bin = readBinaryFile("build/test_validation_addressing_temp.bin");
    bool correct_size = (bin.size() == 2);
    CHECK(correct_size, "lda ($10), Y produces 2 bytes");
}

// Test 9: MEGA65 indirect indexed Z addressing (zp),Z
void test_indirect_indexed_z() {
    std::string code = R"(.org $2000
lda ($10), Z
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, "lda ($10), Z MEGA65 indirect indexed Z assembles");

    // Verify binary output: 2 bytes (opcode + zero-page address)
    auto bin = readBinaryFile("build/test_validation_addressing_temp.bin");
    bool correct_size = (bin.size() == 2);
    CHECK(correct_size, "lda ($10), Z produces 2 bytes");
}

// Test 10: MEGA65 flat-indirect Z addressing [zp],Z
void test_flat_indirect_z() {
    std::string code = R"(.org $2000
lda [$10], Z
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, "lda [$10], Z MEGA65 flat-indirect addressing assembles");

    // Verify binary output: 3 bytes (EOM prefix EA + opcode + zero-page address)
    auto bin = readBinaryFile("build/test_validation_addressing_temp.bin");
    bool correct_size = (bin.size() == 3);
    CHECK(correct_size, "lda [$10], Z produces 3 bytes (EA prefix + opcode + address)");

    if (correct_size) {
        bool has_prefix = (bin[0] == 0xEA);
        CHECK(has_prefix, "lda [$10], Z has EA prefix for flat memory");
    }
}

// Test 11: Stack-indirect addressing (zp,SP),Y
void test_stack_indirect_y() {
    std::string code = R"(.org $2000
lda ($10, SP), Y
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, "lda ($10, SP), Y stack-indirect addressing assembles");

    // Verify binary output: 2 bytes (opcode + zero-page address)
    auto bin = readBinaryFile("build/test_validation_addressing_temp.bin");
    bool correct_size = (bin.size() == 2);
    CHECK(correct_size, "lda ($10, SP), Y produces 2 bytes");
}

// Test 12: Stack-relative addressing (offset,SP)
void test_stack_relative_lda() {
    std::string code = R"(.org $2000
lda $04, SP
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, "lda $04, SP stack-relative addressing assembles");

    // Stack-relative is a simulated op that expands to multiple bytes
    // (typically expands to a simulated instruction sequence)
    auto bin = readBinaryFile("build/test_validation_addressing_temp.bin");
    bool has_output = (bin.size() > 0);
    CHECK(has_output, "lda $04, SP produces output bytes (simulated op expansion)");
}

// Test 13: Stack-relative store
void test_stack_relative_sta() {
    std::string code = R"(.org $2000
sta $08, SP
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, "sta $08, SP stack-relative store assembles");

    // Stack-relative store also expands
    auto bin = readBinaryFile("build/test_validation_addressing_temp.bin");
    bool has_output = (bin.size() > 0);
    CHECK(has_output, "sta $08, SP produces output bytes (simulated op expansion)");
}

// Test 14: Multiple stack-relative instructions
void test_multiple_stack_relative() {
    std::string code = R"(.org $2000
lda $04, SP
sta $06, SP
ldx $08, SP
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, "Multiple stack-relative instructions assemble");

    // Each stack-relative instruction expands, total should be multiple bytes
    auto bin = readBinaryFile("build/test_validation_addressing_temp.bin");
    bool has_output = (bin.size() >= 12);  // Each expands to ~4 bytes
    CHECK(has_output, "Three stack-relative instructions produce multiple bytes");
}

// Test 15: Zero-page with X indexing
void test_zp_x_indexing() {
    std::string code = R"(.org $2000
lda $80, X
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, "lda $80, X zero-page with X indexing assembles");

    // Verify binary output: 2 bytes (opcode + zero-page address)
    auto bin = readBinaryFile("build/test_validation_addressing_temp.bin");
    bool correct_size = (bin.size() == 2);
    CHECK(correct_size, "lda $80, X produces 2 bytes");
}

// Test 16: Absolute with X indexing
void test_abs_x_indexing() {
    std::string code = R"(.org $2000
lda $1000, X
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, "lda $1000, X absolute with X indexing assembles");

    // Verify binary output: 3 bytes (opcode + 16-bit address)
    auto bin = readBinaryFile("build/test_validation_addressing_temp.bin");
    bool correct_size = (bin.size() == 3);
    CHECK(correct_size, "lda $1000, X produces 3 bytes");
}

// Test 17: Accumulator addressing (no operand)
void test_accumulator_addressing() {
    std::string code = R"(.org $2000
asl
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, "asl accumulator addressing assembles");

    // Verify binary output: 1 byte (opcode only)
    auto bin = readBinaryFile("build/test_validation_addressing_temp.bin");
    bool correct_size = (bin.size() == 1);
    CHECK(correct_size, "asl produces 1 byte (accumulator addressing)");
}

// Test 18: Mixed addressing modes in sequence
void test_mixed_addressing_sequence() {
    std::string code = R"(.org $2000
lda #$42
sta $80
ldx $1000, Y
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, "Mixed addressing mode sequence assembles");

    // lda #$42 = 2 bytes, sta $80 = 2 bytes, ldx $1000, Y = 3 bytes = 7 bytes total
    auto bin = readBinaryFile("build/test_validation_addressing_temp.bin");
    bool correct_size = (bin.size() == 7);
    CHECK(correct_size, "Mixed addressing sequence produces correct byte count (7)");
}

// Test 19: Store instructions with stack-relative
void test_store_stack_relative() {
    std::string code = R"(.org $2000
sta $10, S
sty $12, S
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, "Multiple store stack-relative instructions assemble");

    // Each instruction should produce bytes
    auto bin = readBinaryFile("build/test_validation_addressing_temp.bin");
    bool has_content = (bin.size() > 0);
    CHECK(has_content, "Store stack-relative instructions produce output");
}

int main() {
    printf("Testing addressing mode validation...\n\n");

    // Basic addressing mode tests
    test_immediate_basic();
    test_basepage_direct();
    test_absolute_direct();
    test_indirect_indexed_basic();

    // Base-page auto-selection tests
    test_auto_zp_boundary();
    test_auto_abs_large_address();

    // Indexed indirect addressing mode tests
    test_indexed_indirect_x();
    test_indirect_indexed_y();
    test_indirect_indexed_z();
    test_flat_indirect_z();
    test_stack_indirect_y();

    // Stack-relative addressing tests
    test_stack_relative_lda();
    test_stack_relative_sta();
    test_multiple_stack_relative();

    // Other addressing modes
    test_zp_x_indexing();
    test_abs_x_indexing();
    test_accumulator_addressing();

    // Store tests
    test_store_stack_relative();

    // Mixed sequence test
    test_mixed_addressing_sequence();

    printf("\nAddressing Mode Validation Tests: %d passed, %d failed\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
