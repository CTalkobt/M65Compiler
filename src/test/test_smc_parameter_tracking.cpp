#include <iostream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <fstream>

// Phase 78: SMC Parameter Tracking Tests
// Tests for Self-Modifying Code parameter access counting and decision logic

std::string readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) return "";
    std::stringstream buf;
    buf << f.rdbuf();
    return buf.str();
}

int compileTest(const std::string& code, const std::string& outputFile) {
    std::string codeFile = "build/test_smc_" + outputFile + ".c";
    std::string asmFile = "build/test_smc_" + outputFile + ".s45";

    // Write test code
    std::ofstream out(codeFile);
    out << code;
    out.close();

    // Compile with SAC mode
    std::string cmd = "./bin/cc45 " + codeFile + " -fstaticalloc -S -o " + asmFile + " 2>&1";
    int ret = system(cmd.c_str());

    return ret;
}

int countParameterLoads(const std::string& asmContent, const std::string& paramName) {
    int count = 0;
    // Look for pattern like "lda _func__param_X" or "ldx _func__param_X"
    // The pattern is: instruction mnemonic + space + function name + "__param_" + param name

    std::string pattern = "__param_" + paramName;
    size_t pos = 0;

    while ((pos = asmContent.find(pattern, pos)) != std::string::npos) {
        // Check if preceded by lda, ldx, ldy, or ldz
        // Look backwards to find the instruction
        if (pos > 10) {
            // Find the line start
            size_t lineStart = asmContent.rfind('\n', pos);
            if (lineStart == std::string::npos) lineStart = 0;
            else lineStart++;

            // Skip whitespace
            while (lineStart < pos && std::isspace(asmContent[lineStart])) {
                lineStart++;
            }

            // Get the instruction
            size_t spacePos = asmContent.find(' ', lineStart);
            if (spacePos != std::string::npos && spacePos < pos) {
                std::string mnem = asmContent.substr(lineStart, spacePos - lineStart);
                if (mnem == "lda" || mnem == "ldx" || mnem == "ldy" || mnem == "ldz") {
                    count++;
                }
            }
        }
        pos++;
    }

    return count;
}

// Test 1: Parameter accessed 1 time
bool test_single_access() {
    std::string code = R"(
int func1(int x) {
    return x;
}

int main(void) {
    return func1(5);
}
)";

    if (compileTest(code, "single_access") != 0) {
        std::cerr << "FAIL: test_single_access - compilation failed\n";
        return false;
    }

    std::string asm_content = readFile("build/test_smc_single_access.s45");
    int loads = countParameterLoads(asm_content, "x");

    if (loads >= 1) {
        std::cout << "PASS: test_single_access - parameter accessed " << loads << " time(s)\n";
        return true;
    }

    std::cerr << "FAIL: test_single_access - expected at least 1 load, got " << loads << "\n";
    return false;
}

// Test 2: Parameter accessed 3 times (boundary case)
bool test_three_accesses() {
    std::string code = R"(
int func3(int x) {
    int a = x + x;
    int b = a + x;
    return b;
}

int main(void) {
    return func3(5);
}
)";

    if (compileTest(code, "three_access") != 0) {
        std::cerr << "FAIL: test_three_accesses - compilation failed\n";
        return false;
    }

    std::string asm_content = readFile("build/test_smc_three_access.s45");
    int loads = countParameterLoads(asm_content, "x");

    // At boundary (3 accesses), should NOT use SMC (saves 0 bytes)
    // Still generates lda/ldx for each access
    if (loads >= 3) {
        std::cout << "PASS: test_three_accesses - parameter accessed " << loads << " time(s) (boundary case)\n";
        return true;
    }

    std::cerr << "FAIL: test_three_accesses - expected at least 3 loads, got " << loads << "\n";
    return false;
}

// Test 3: Parameter accessed 5 times (should trigger SMC)
bool test_five_accesses() {
    std::string code = R"(
int func5(int x) {
    int result = x + x;
    result = result + x;
    result = result + x;
    result = result + x;
    return result;
}

int main(void) {
    return func5(10);
}
)";

    if (compileTest(code, "five_access") != 0) {
        std::cerr << "FAIL: test_five_accesses - compilation failed\n";
        return false;
    }

    std::string asm_content = readFile("build/test_smc_five_access.s45");
    int loads = countParameterLoads(asm_content, "x");

    // 5 accesses should trigger SMC (saves 2 bytes)
    if (loads >= 5) {
        std::cout << "PASS: test_five_accesses - parameter accessed " << loads << " time(s) (SMC eligible)\n";
        return true;
    }

    std::cerr << "FAIL: test_five_accesses - expected at least 5 loads, got " << loads << "\n";
    return false;
}

// Test 4: Multiple parameters with different access patterns
bool test_multiple_params() {
    std::string code = R"(
int func_multi(int a, int b, int c) {
    // a: accessed 5 times (SMC eligible)
    // b: accessed 2 times (not SMC eligible)
    // c: accessed 1 time (not SMC eligible)

    int r = a + a;
    r = r + a;
    r = r + a;
    r = r + a;
    r = r + b;
    r = r + b;
    r = r + c;
    return r;
}

int main(void) {
    return func_multi(1, 2, 3);
}
)";

    if (compileTest(code, "multi_param") != 0) {
        std::cerr << "FAIL: test_multiple_params - compilation failed\n";
        return false;
    }

    std::string asm_content = readFile("build/test_smc_multi_param.s45");
    int loads_a = countParameterLoads(asm_content, "a");
    int loads_b = countParameterLoads(asm_content, "b");
    int loads_c = countParameterLoads(asm_content, "c");

    bool correct_a = loads_a >= 5;
    bool correct_b = loads_b >= 2;
    bool correct_c = loads_c >= 1;

    if (correct_a && correct_b && correct_c) {
        std::cout << "PASS: test_multiple_params - a:" << loads_a
                  << ", b:" << loads_b << ", c:" << loads_c << "\n";
        return true;
    }

    std::cerr << "FAIL: test_multiple_params - expected a>=5, b>=2, c>=1; got a:" << loads_a
              << ", b:" << loads_b << ", c:" << loads_c << "\n";
    return false;
}

// Test 5: Parameter with complex access patterns
bool test_complex_pattern() {
    std::string code = R"(
int fibonacci_acc(int n, int a, int b) {
    // a and b are accumulated 6+ times in real code
    // This tests realistic parameter usage patterns
    int i = 0;
    int c;
    while (i < n) {
        c = a + b;     // a access 1, b access 1
        a = b;         // a access 2, b access 2
        b = c;         // b access 3
        i = i + 1;     // loop
    }
    return a;
}

int main(void) {
    return fibonacci_acc(10, 0, 1);
}
)";

    if (compileTest(code, "complex") != 0) {
        std::cerr << "FAIL: test_complex_pattern - compilation failed\n";
        return false;
    }

    std::string asm_content = readFile("build/test_smc_complex.s45");

    // Just verify it compiles and generates parameter loads
    if (asm_content.find("__param_") != std::string::npos) {
        std::cout << "PASS: test_complex_pattern - complex parameter patterns handled\n";
        return true;
    }

    std::cerr << "FAIL: test_complex_pattern - no parameter loads found\n";
    return false;
}

int main() {
    std::cout << "=== Phase 78: SMC Parameter Access Tracking Tests ===\n\n";

    int passed = 0;
    int failed = 0;

    if (test_single_access()) passed++; else failed++;
    if (test_three_accesses()) passed++; else failed++;
    if (test_five_accesses()) passed++; else failed++;
    if (test_multiple_params()) passed++; else failed++;
    if (test_complex_pattern()) passed++; else failed++;

    std::cout << "\n=== Test Summary ===\n";
    std::cout << "Passed: " << passed << "\n";
    std::cout << "Failed: " << failed << "\n";

    return (failed == 0) ? 0 : 1;
}
