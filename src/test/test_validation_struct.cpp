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
        printf("FAIL: %s (line %d)\n", msg, __LINE__); \
        tests_failed++; \
    } else { \
        tests_passed++; \
    } \
} while(0)

// Helper: Run the C compiler and capture stderr
static std::pair<int, std::string> runCC45(const std::string& cCode) {
    // Write C code to temp file
    std::ofstream c_file("/tmp/test_struct_temp.c");
    c_file << cCode;
    c_file.close();

    // Run cc45 and capture stderr only
    int ret = system("./bin/cc45 /tmp/test_struct_temp.c -o /tmp/test_struct_temp.s 2>/tmp/test_struct_stderr.txt >/dev/null");

    // Read captured stderr
    std::string stderr_content;
    std::ifstream stderr_file("/tmp/test_struct_stderr.txt");
    if (stderr_file) {
        std::stringstream buffer;
        buffer << stderr_file.rdbuf();
        stderr_content = buffer.str();
        stderr_file.close();
    }

    return {ret, stderr_content};
}

// Test 1: Flexible array member in union (should fail)
void test_fam_in_union() {
    std::string code = R"(
union U {
    int x;
    int data[];
};
int main() { return 0; }
)";
    auto [ret, stderr] = runCC45(code);
    CHECK(ret != 0, "FAM in union compilation fails");
    CHECK(stderr.find("not allowed in union") != std::string::npos,
          "FAM in union error message correct");
}

// Test 2: Flexible array member not last (should fail)
void test_fam_not_last() {
    std::string code = R"(
struct S {
    int data[];
    int x;
};
int main() { return 0; }
)";
    auto [ret, stderr] = runCC45(code);
    CHECK(ret != 0, "FAM not last compilation fails");
    CHECK(stderr.find("must be the last member") != std::string::npos,
          "FAM not last error message correct");
}

// Test 3: Struct with only flexible array member (should fail)
void test_struct_only_fam() {
    std::string code = R"(
struct S {
    int data[];
};
int main() { return 0; }
)";
    auto [ret, stderr] = runCC45(code);
    CHECK(ret != 0, "Struct with only FAM compilation fails");
    CHECK(stderr.find("must have at least one other member") != std::string::npos,
          "Struct only FAM error message correct");
}

// Test 4: Unknown struct/union type in member layout (should fail)
void test_unknown_struct_member() {
    std::string code = R"(
struct S {
    struct Unknown u;
};
int main() { return 0; }
)";
    auto [ret, stderr] = runCC45(code);
    CHECK(ret != 0, "Unknown struct member compilation fails");
    CHECK(stderr.find("Unknown struct/union type") != std::string::npos,
          "Unknown struct member error message correct");
}

// Test 5: Dot operator on non-struct type (should fail)
void test_dot_on_non_struct() {
    std::string code = R"(
int main() {
    int x = 5;
    int y = x.member;
    return 0;
}
)";
    auto [ret, stderr] = runCC45(code);
    CHECK(ret != 0, "Dot on non-struct compilation fails");
    CHECK(stderr.find("Dot/Arrow operator on non-struct type") != std::string::npos,
          "Dot on non-struct error message correct");
}

// Test 6: Unknown struct type in member access (should fail)
void test_unknown_struct_access() {
    std::string code = R"(
int main() {
    struct Unknown s;
    int x = s.member;
    return 0;
}
)";
    auto [ret, stderr] = runCC45(code);
    CHECK(ret != 0, "Unknown struct access compilation fails");
    CHECK(stderr.find("Unknown struct/union type") != std::string::npos,
          "Unknown struct access error message correct");
}

// Test 7: Member not found in struct (should fail)
void test_member_not_found() {
    std::string code = R"(
struct S {
    int x;
};
int main() {
    struct S s;
    int y = s.z;
    return 0;
}
)";
    auto [ret, stderr] = runCC45(code);
    CHECK(ret != 0, "Member not found compilation fails");
    CHECK(stderr.find("not found") != std::string::npos,
          "Member not found error message correct");
}

// Test 8: Unknown struct in compound literal init (should fail)
void test_unknown_struct_compound_literal() {
    std::string code = R"(
int main() {
    struct Unknown *s = &(struct Unknown){};
    return 0;
}
)";
    auto [ret, stderr] = runCC45(code);
    CHECK(ret != 0, "Unknown struct compound literal compilation fails");
    CHECK(stderr.find("Unknown struct") != std::string::npos,
          "Unknown struct compound literal error message correct");
}

// Test 9: Unknown struct in variable declaration init (should fail)
void test_unknown_struct_decl_init() {
    std::string code = R"(
int main() {
    struct Unknown s = {};
    return 0;
}
)";
    auto [ret, stderr] = runCC45(code);
    CHECK(ret != 0, "Unknown struct decl init compilation fails");
    CHECK(stderr.find("Unknown struct type:") != std::string::npos,
          "Unknown struct decl init error message correct");
}

// Test 10: Valid struct usage succeeds (sanity check)
void test_valid_struct() {
    std::string code = R"(
struct Point {
    int x;
    int y;
};

int main() {
    struct Point p;
    p.x = 5;
    p.y = 10;
    return p.x + p.y;
}
)";
    auto [ret, stderr] = runCC45(code);
    CHECK(ret == 0, "Valid struct compilation succeeds");
}

// Test 11: Valid flexible array member in struct (sanity check)
void test_valid_fam() {
    std::string code = R"(
struct S {
    int x;
    int data[];
};

int main() {
    return 0;
}
)";
    auto [ret, stderr] = runCC45(code);
    CHECK(ret == 0, "Valid FAM compilation succeeds");
}

// Test 12: Valid union usage (sanity check)
void test_valid_union() {
    std::string code = R"(
union U {
    int x;
    char c;
};

int main() {
    union U u;
    u.x = 5;
    return 0;
}
)";
    auto [ret, stderr] = runCC45(code);
    CHECK(ret == 0, "Valid union compilation succeeds");
}

int main() {
    // Ensure compiler is built
    if (system("test -f ./bin/cc45") != 0) {
        printf("FAIL: Compiler ./bin/cc45 not found\n");
        return 1;
    }

    // Error validation tests
    test_fam_in_union();
    test_fam_not_last();
    test_struct_only_fam();
    test_unknown_struct_member();
    test_dot_on_non_struct();
    test_unknown_struct_access();
    test_member_not_found();
    test_unknown_struct_compound_literal();
    test_unknown_struct_decl_init();

    // Sanity check tests
    test_valid_struct();
    test_valid_fam();
    test_valid_union();

    printf("\nStruct/Union Validation Tests: %d passed, %d failed\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
