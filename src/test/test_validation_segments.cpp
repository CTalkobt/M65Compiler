#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

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
    std::ofstream asm_file("build/test_validation_segments_temp.s");
    asm_file << asmCode;
}

// Test helper: check if assembly succeeded
static bool assemblySucceeded(const std::string& asmCode) {
    writeAssemblyFile(asmCode);
    int ret = system("./bin/ca45 build/test_validation_segments_temp.s -o build/test_validation_segments_temp.bin 2>/dev/null");
    return ret == 0;
}

// Test 1: .code segment switching
void test_code_segment() {
    std::string code = R"(.org 0
.code
lda #$42
sta $80
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, ".code segment switching succeeds");
}

// Test 2: .data segment switching
void test_data_segment() {
    std::string code = R"(.org 0
.data
.byte $01, $02, $03
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, ".data segment switching succeeds");
}

// Test 3: .bss segment switching
void test_bss_segment() {
    std::string code = R"(.org 0
.bss
.byte 0, 0, 0
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, ".bss segment switching succeeds");
}

// Test 4: .segment with named segment
void test_segment_named() {
    std::string code = R"(.org 0
.segment "custom"
lda #$42
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, ".segment with custom name succeeds");
}

// Test 5: .segment with curly brace block (scoped segment)
void test_segment_block() {
    std::string code = R"(.org 0
.code
lda #$42
.segment "custom" {
    lda #$99
}
sta $80
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, ".segment with block scope succeeds");
}

// Test 6: Labels in code segment
void test_labels_code_segment() {
    std::string code = R"(.org 0
.code
loop: lda #$42
sta $80
rts
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, "Labels in code segment succeed");
}

// Test 7: .segmentOrder directive
void test_segmentOrder() {
    std::string code = R"(.org 0
.segmentOrder code, data, bss
.code
lda #$42
.data
.byte $01
.bss
.byte 0
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, ".segmentOrder directive succeeds");
}

// Test 8: .global single symbol
void test_global_single() {
    std::string code = R"(.org 0
.global exported_label
exported_label: lda #$42
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, ".global single symbol succeeds");
}

// Test 9: .global multiple symbols
void test_global_multiple() {
    std::string code = R"(.org 0
.global sym1, sym2, sym3
sym1: lda #$42
sym2: sta $80
sym3: rts
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, ".global multiple symbols succeeds");
}

// Test 10: .extern single symbol
void test_extern_single() {
    std::string code = R"(.org 0
.extern external_func
jsr external_func
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, ".extern single symbol succeeds");
}

// Test 11: .extern multiple symbols
void test_extern_multiple() {
    std::string code = R"(.org 0
.extern external_func1, external_func2
jsr external_func1
jsr external_func2
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, ".extern multiple symbols succeeds");
}

// Test 12: .weak single symbol
void test_weak_single() {
    std::string code = R"(.org 0
.weak weak_label
weak_label: lda #$42
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, ".weak single symbol succeeds");
}

// Test 13: .weak multiple symbols
void test_weak_multiple() {
    std::string code = R"(.org 0
.weak weak_sym1, weak_sym2
weak_sym1: lda #$42
weak_sym2: sta $80
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, ".weak multiple symbols succeeds");
}

// Test 14: Reference to external symbol
void test_extern_reference() {
    std::string code = R"(.org $2000
.extern external_func
lda #<external_func
lda #>external_func
jsr external_func
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, "Reference to extern symbol succeeds");
}

// Test 15: Label references in data section
void test_label_in_data_section() {
    std::string code = R"(.org 0
.code
code_label: lda #$42
.data
label_ptr: .byte $00, $00
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, "Labels and data in different segments succeed");
}

// Test 16: Segment switching with org
void test_segment_with_org() {
    std::string code = R"(.segment "code" {
    .org $2000
    lda #$42
}
.segment "data" {
    .org $3000
    .byte $01
}
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, "Segment switching with .org succeeds");
}

// Test 17: Global symbol followed by definition
void test_global_then_define() {
    std::string code = R"(.org 0
.global my_func
my_func:
    lda #$42
    rts
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, ".global followed by definition succeeds");
}

// Test 18: Multiple segment order declarations (last should win)
void test_multiple_segmentOrder() {
    std::string code = R"(.org 0
.segmentOrder code, data
.segmentOrder data, code, bss
.code
lda #$42
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, "Multiple .segmentOrder declarations succeeds");
}

// Test 19: Nested block segment with multiple instructions
void test_nested_block_instructions() {
    std::string code = R"(.org 0
.code
start: lda #$42
.segment "temp" {
    sta $80
    lda $80
    rts
}
lda #$99
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, "Nested block segment with instructions succeeds");
}

// Test 20: Empty segment is allowed
void test_empty_segment() {
    std::string code = R"(.org 0
.code
lda #$42
.data
.code
sta $80
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, "Empty segment switching succeeds");
}

int main() {
    printf("Testing segment and visibility directive validation...\n\n");

    // Segment switching tests
    test_code_segment();
    test_data_segment();
    test_bss_segment();
    test_segment_named();
    test_segment_block();
    test_labels_code_segment();

    // Segment ordering tests
    test_segmentOrder();
    test_multiple_segmentOrder();
    test_segment_with_org();

    // Visibility directive tests
    test_global_single();
    test_global_multiple();
    test_global_then_define();
    test_extern_single();
    test_extern_multiple();
    test_extern_reference();
    test_weak_single();
    test_weak_multiple();

    // Complex tests
    test_label_in_data_section();
    test_nested_block_instructions();
    test_empty_segment();

    printf("\nSegment and Visibility Directive Validation Tests: %d passed, %d failed\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
