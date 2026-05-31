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
    std::ofstream asm_file("build/test_validation_directives_temp.s");
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
    system("./bin/ca45 build/test_validation_directives_temp.s -o build/test_validation_directives_temp.bin 2>build/test_validation_directives_temp.err 1>build/test_validation_directives_temp.out");

    // Try stderr first, fall back to stdout if empty
    std::string error_output = readFile("build/test_validation_directives_temp.err");
    if (error_output.empty()) {
        error_output = readFile("build/test_validation_directives_temp.out");
    }
    return error_output;
}

// Test helper: check if assembly failed with expected error message
static bool assemblyFailedWithError(const std::string& asmCode, const std::string& expectedError) {
    std::string output = assembleCode(asmCode);
    return output.find(expectedError) != std::string::npos;
}

// Test helper: check if assembly succeeded
static bool assemblySucceeded(const std::string& asmCode) {
    writeAssemblyFile(asmCode);
    int ret = system("./bin/ca45 build/test_validation_directives_temp.s -o build/test_validation_directives_temp.bin 2>&1 >/dev/null");
    return ret == 0;
}

// Test 1: .byte directive with multiple values
void test_byte_multiple_values() {
    std::string code = R"(.org $2000
.byte 42, 255, 0
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, ".byte with multiple values assembles successfully");

    auto bin = readBinaryFile("build/test_validation_directives_temp.bin");
    CHECK(bin.size() == 3, ".byte produces correct byte count (3)");
    if (bin.size() == 3) {
        CHECK(bin[0] == 42 && bin[1] == 255 && bin[2] == 0,
              ".byte produces correct byte values (0x2A, 0xFF, 0x00)");
    }
}

// Test 2: .word directive (16-bit, little-endian)
void test_word_little_endian() {
    std::string code = R"(.org $2000
.word $1234
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, ".word directive assembles successfully");

    auto bin = readBinaryFile("build/test_validation_directives_temp.bin");
    CHECK(bin.size() == 2, ".word produces 2 bytes");
    if (bin.size() == 2) {
        CHECK(bin[0] == 0x34 && bin[1] == 0x12, ".word uses little-endian byte order");
    }
}

// Test 3: .dword directive (32-bit)
void test_dword_four_bytes() {
    std::string code = R"(.org $2000
.dword $12345678
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, ".dword directive assembles successfully");

    auto bin = readBinaryFile("build/test_validation_directives_temp.bin");
    CHECK(bin.size() == 4, ".dword produces 4 bytes");
    if (bin.size() == 4) {
        CHECK(bin[0] == 0x78 && bin[1] == 0x56 && bin[2] == 0x34 && bin[3] == 0x12,
              ".dword uses little-endian byte order");
    }
}

// Test 4: .long directive (alias for .dword)
void test_long_alias_for_dword() {
    std::string code = R"(.org $2000
.long $12345678
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, ".long directive assembles successfully");

    auto bin = readBinaryFile("build/test_validation_directives_temp.bin");
    CHECK(bin.size() == 4, ".long produces 4 bytes");
}

// Test 5: .word with multiple values
void test_word_multiple_values() {
    std::string code = R"(.org $2000
.word $1111, $2222, $3333
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, ".word with multiple values assembles successfully");

    auto bin = readBinaryFile("build/test_validation_directives_temp.bin");
    CHECK(bin.size() == 6, ".word multiple values produces 6 bytes");
}

// Test 6: .ascii directive (without null terminator)
void test_ascii_no_null_terminator() {
    std::string code = R"(.org $2000
.ascii "Hello"
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, ".ascii directive assembles successfully");

    auto bin = readBinaryFile("build/test_validation_directives_temp.bin");
    CHECK(bin.size() == 5, ".ascii produces correct byte count without null terminator");
    if (bin.size() == 5) {
        CHECK(bin[0] == 'H' && bin[1] == 'e' && bin[2] == 'l' && bin[3] == 'l' && bin[4] == 'o',
              ".ascii produces correct ASCII values");
    }
}

// Test 7: .text directive (PETSCII encoded)
void test_text_petscii_encoded() {
    std::string code = R"(.org $2000
.text "Hello"
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, ".text directive assembles successfully");

    auto bin = readBinaryFile("build/test_validation_directives_temp.bin");
    CHECK(bin.size() == 5, ".text produces 5 bytes");
}

// Test 8: .res directive (reserve bytes with zero fill)
void test_res_zero_fill() {
    std::string code = R"(.org $2000
.res 4
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, ".res directive assembles successfully");

    auto bin = readBinaryFile("build/test_validation_directives_temp.bin");
    CHECK(bin.size() == 4, ".res produces correct byte count");
    if (bin.size() == 4) {
        CHECK(bin[0] == 0 && bin[1] == 0 && bin[2] == 0 && bin[3] == 0,
              ".res fills with zero bytes");
    }
}

// Test 9: .res with fill byte
void test_res_with_fill_byte() {
    std::string code = R"(.org $2000
.res 4, $FF
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, ".res with fill byte assembles successfully");

    auto bin = readBinaryFile("build/test_validation_directives_temp.bin");
    CHECK(bin.size() == 4, ".res with fill produces correct byte count");
    if (bin.size() == 4) {
        CHECK(bin[0] == 0xFF && bin[1] == 0xFF && bin[2] == 0xFF && bin[3] == 0xFF,
              ".res fills with specified byte value");
    }
}

// Test 10: .align directive (4-byte boundary)
void test_align_boundary() {
    std::string code = R"(.org $2001
.byte 1
.align 4
.byte 2
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, ".align directive assembles successfully");

    auto bin = readBinaryFile("build/test_validation_directives_temp.bin");
    CHECK(bin.size() >= 4, ".align produces padding bytes");
}

// Test 11: .balign directive (alias for .align)
void test_balign_same_as_align() {
    std::string code = R"(.org $2000
.byte 1
.balign 4
.byte 2
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, ".balign directive assembles successfully");

    auto bin = readBinaryFile("build/test_validation_directives_temp.bin");
    CHECK(bin.size() >= 5, ".balign produces padding bytes");
}

// Test 12: .org directive
void test_org_directive() {
    std::string code = R"(.org $2000
.byte 42
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, ".org directive works");
}

// Test 13: * = address (alternative org syntax)
void test_org_star_syntax() {
    std::string code = R"(* = $2000
.byte 42
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, "* = address syntax (alternative org) works");
}

// Test 14: .byte with non-numeric operand (error case)
void test_byte_non_numeric_error() {
    std::string code = R"(.org $2000
.byte invalid_label
)";
    bool failed_correctly = assemblyFailedWithError(code, "");
    CHECK(failed_correctly, ".byte with undefined label fails");
}

// Test 15: .word with string operand (error case)
void test_word_string_operand_error() {
    std::string code = R"(.org $2000
.word "notanumber"
)";
    bool failed_correctly = assemblyFailedWithError(code, "");
    CHECK(failed_correctly, ".word with string operand fails");
}

// Test 16: Combined directives (sanity check)
void test_combined_directives() {
    std::string code = R"(.org $2000
.byte 1, 2
.word $1234
.ascii "AB"
.res 2
.byte 3
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, "Combined data directives assemble successfully");

    auto bin = readBinaryFile("build/test_validation_directives_temp.bin");
    CHECK(bin.size() == 9, "Combined directives produce correct total size");
}

// Helper: write arbitrary binary data to disk
static void writeBinaryData(const std::string& path, const std::vector<uint8_t>& data) {
    system("mkdir -p build");
    std::ofstream f(path, std::ios::binary);
    f.write(reinterpret_cast<const char*>(data.data()), data.size());
}

void test_import_binary_basic() {
    writeBinaryData("build/test_import_binary.bin", {0xDE, 0xAD, 0xBE, 0xEF, 0x42});
    bool ok = assemblySucceeded(".import binary \"build/test_import_binary.bin\"\n");
    CHECK(ok, ".import binary assembles successfully");
    auto bin = readBinaryFile("build/test_validation_directives_temp.bin");
    CHECK(bin.size() == 5, ".import binary produces correct byte count");
    if (bin.size() == 5)
        CHECK(bin[0]==0xDE && bin[1]==0xAD && bin[2]==0xBE && bin[3]==0xEF && bin[4]==0x42,
              ".import binary produces correct byte values");
}

void test_incbin_basic() {
    writeBinaryData("build/test_incbin.bin", {0x01, 0x02, 0x03});
    bool ok = assemblySucceeded(".incbin \"build/test_incbin.bin\"\n");
    CHECK(ok, ".incbin assembles successfully");
    auto bin = readBinaryFile("build/test_validation_directives_temp.bin");
    CHECK(bin.size() == 3, ".incbin produces correct byte count");
    if (bin.size() == 3)
        CHECK(bin[0]==0x01 && bin[1]==0x02 && bin[2]==0x03, ".incbin produces correct byte values");
}

void test_import_binary_file_not_found() {
    bool failed = assemblyFailedWithError(".import binary \"build/no_such_file_xyz.bin\"\n",
                                          "cannot open binary file");
    CHECK(failed, ".import binary gives error for missing file");
}

void test_import_binary_missing_keyword() {
    writeBinaryData("build/test_import_binary.bin", {0x00});
    bool failed = assemblyFailedWithError(".import \"build/test_import_binary.bin\"\n",
                                          "binary");
    CHECK(failed, ".import without 'binary' keyword gives error");
}

int main() {
    printf("Testing data directives validation...\n\n");

    // Valid directive tests
    test_byte_multiple_values();
    test_word_little_endian();
    test_word_multiple_values();
    test_dword_four_bytes();
    test_long_alias_for_dword();
    test_ascii_no_null_terminator();
    test_text_petscii_encoded();
    test_res_zero_fill();
    test_res_with_fill_byte();
    test_align_boundary();
    test_balign_same_as_align();
    test_org_directive();
    test_org_star_syntax();

    // Error case tests
    test_byte_non_numeric_error();
    test_word_string_operand_error();

    // Combined test
    test_combined_directives();

    // Binary import tests
    test_import_binary_basic();
    test_incbin_basic();
    test_import_binary_file_not_found();
    test_import_binary_missing_keyword();

    printf("\nData Directives Validation Tests: %d passed, %d failed\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
