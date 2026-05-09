#include <cstdio>
#include <cstdlib>
#include <cstring>
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

struct RunResult {
    int exitCode;
    std::string output;
};

// Helper function to shell out to cc45 and capture output
static RunResult runCC45(const char* src) {
    // Write source to temp file
    const char* tempFile = "src/test/build/test_validation_preprocessor_temp.c";
    FILE* f = fopen(tempFile, "w");
    if (!f) {
        fprintf(stderr, "Failed to open temp file %s\n", tempFile);
        return {-1, ""};
    }
    fprintf(f, "%s", src);
    fclose(f);

    // Run cc45 and capture output via popen
    std::string cmd = "./bin/cc45 " + std::string(tempFile) + " -o src/test/build/test_validation_preprocessor_temp.o 2>&1";
    FILE* p = popen(cmd.c_str(), "r");
    if (!p) {
        fprintf(stderr, "Failed to run cc45\n");
        return {-1, ""};
    }

    std::string output;
    char buf[1024];
    while (fgets(buf, sizeof(buf), p)) {
        output += buf;
    }

    int exitCode = pclose(p) >> 8;  // WEXITSTATUS
    return {exitCode, output};
}

// Test 1: Unexpected #elif without preceding #if
void test_unexpected_elif() {
    const char* src = R"(
#elif 1
int x;
)";
    RunResult res = runCC45(src);
    CHECK(res.exitCode != 0 && res.output.find("Unexpected #elif") != std::string::npos,
          "Unexpected #elif without #if");
}

// Test 2: #elif after #else
void test_elif_after_else() {
    const char* src = R"(
#if 1
int x;
#else
int y;
#elif 0
int z;
#endif
)";
    RunResult res = runCC45(src);
    CHECK(res.exitCode != 0 && res.output.find("Unexpected #elif after #else") != std::string::npos,
          "#elif after #else");
}

// Test 3: Unexpected #else without preceding #if
void test_unexpected_else() {
    const char* src = R"(
#else
int x;
)";
    RunResult res = runCC45(src);
    CHECK(res.exitCode != 0 && res.output.find("Unexpected #else") != std::string::npos,
          "Unexpected #else without #if");
}

// Test 4: Multiple #else in one block
void test_multiple_else() {
    const char* src = R"(
#if 1
int x;
#else
int y;
#else
int z;
#endif
)";
    RunResult res = runCC45(src);
    CHECK(res.exitCode != 0 && res.output.find("Multiple #else") != std::string::npos,
          "Multiple #else in one block");
}

// Test 5: Unexpected #endif without preceding #if
void test_unexpected_endif() {
    const char* src = R"(
int x;
#endif
)";
    RunResult res = runCC45(src);
    CHECK(res.exitCode != 0 && res.output.find("Unexpected #endif") != std::string::npos,
          "Unexpected #endif without #if");
}

// Test 6: Invalid #include directive syntax
void test_invalid_include_syntax() {
    const char* src = R"(
#include something
int x;
)";
    RunResult res = runCC45(src);
    CHECK(res.exitCode != 0 && res.output.find("Invalid #include directive") != std::string::npos,
          "Invalid #include directive syntax");
}

// Test 7: #include file not found
void test_include_file_not_found() {
    const char* src = R"(
#include "nonexistent_file_12345.h"
int x;
)";
    RunResult res = runCC45(src);
    CHECK(res.exitCode != 0 && res.output.find("Could not find include file") != std::string::npos,
          "#include file not found");
}

// Test 8: #error directive triggers error
void test_error_directive() {
    const char* src = R"(
#error "custom error message"
int x;
)";
    RunResult res = runCC45(src);
    CHECK(res.exitCode != 0 && res.output.find("#error:") != std::string::npos,
          "#error directive triggers error");
}

// Test 9: Valid preprocessor code succeeds
void test_valid_preprocessor() {
    const char* src = R"(
#ifdef UNDEFINED
int x;
#else
int y;
#endif

#if 1
int z;
#endif
)";
    RunResult res = runCC45(src);
    CHECK(res.exitCode == 0, "Valid preprocessor code succeeds");
}

// Test 10: Nested #if/#endif succeeds
void test_nested_conditionals() {
    const char* src = R"(
#if 1
#ifdef UNDEFINED
int x;
#else
#if 1
int y;
#endif
#endif
#endif
)";
    RunResult res = runCC45(src);
    CHECK(res.exitCode == 0, "Nested conditionals succeed");
}

// Test 11: #ifdef with defined macro succeeds
void test_ifdef_with_macro() {
    const char* src = R"(
#define TEST 1
#ifdef TEST
int x;
#endif
)";
    RunResult res = runCC45(src);
    CHECK(res.exitCode == 0, "#ifdef with defined macro succeeds");
}

// Test 12: Complex conditional nesting
void test_complex_conditionals() {
    const char* src = R"(
#define FEATURE 1
#if 1
#ifdef FEATURE
int a;
#endif
#if FEATURE
int b;
#else
int c;
#endif
#else
int d;
#endif
)";
    RunResult res = runCC45(src);
    CHECK(res.exitCode == 0, "Complex conditional nesting succeeds");
}

int main() {
    printf("Running Preprocessor Error Validation Tests...\n\n");

    test_unexpected_elif();
    test_elif_after_else();
    test_unexpected_else();
    test_multiple_else();
    test_unexpected_endif();
    test_invalid_include_syntax();
    test_include_file_not_found();
    test_error_directive();
    test_valid_preprocessor();
    test_nested_conditionals();
    test_ifdef_with_macro();
    test_complex_conditionals();

    printf("\n========================================\n");
    printf("Tests Passed: %d\n", tests_passed);
    printf("Tests Failed: %d\n", tests_failed);
    printf("========================================\n");

    return (tests_failed > 0) ? 1 : 0;
}
