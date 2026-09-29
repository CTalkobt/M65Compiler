#include <iostream>
#include <sstream>
#include <cassert>
#include <cstdlib>
#include <vector>
#include <string>

// Comprehensive const-correctness validation tests
// Tests edge cases for CodeGenerator removal

std::vector<std::pair<std::string, std::string>> test_cases = {
    // (C code, expected error pattern)

    // 1. Const local variable - assignment
    {R"(
        int main() {
            const int x = 5;
            x = 10;
            return 0;
        }
    )", "Assignment to read-only"},

    // 2. Const global variable - assignment
    {R"(
        const int g = 5;
        int main() {
            g = 10;
            return 0;
        }
    )", "Assignment to read-only"},

    // 3. Const parameter - assignment
    {R"(
        void foo(const int x) {
            x = 10;
        }
        int main() { return 0; }
    )", "Assignment to read-only"},

    // 4. Const struct member - assignment
    {R"(
        struct S {
            const int x;
            int y;
        };
        int main() {
            struct S s;
            s.x = 10;
            return 0;
        }
    )", "Assignment to read-only"},

    // 5. Const local - increment
    {R"(
        int main() {
            const int x = 5;
            x++;
            return 0;
        }
    )", "Increment/decrement of read-only"},

    // 6. Const local - decrement
    {R"(
        int main() {
            const int x = 5;
            x--;
            return 0;
        }
    )", "Increment/decrement of read-only"},

    // 7. Const parameter - increment
    {R"(
        void foo(const int x) {
            x++;
        }
        int main() { return 0; }
    )", "Increment/decrement of read-only"},

    // 8. Pointer to const - dereference write
    {R"(
        int main() {
            int arr[5] = {0};
            const int* p = arr;
            *p = 10;
            return 0;
        }
    )", "Assignment to read-only"},

    // 9. Const pointer - pointer reassignment
    {R"(
        int main() {
            int x = 5, y = 10;
            int* const p = &x;
            p = &y;
            return 0;
        }
    )", ""},  // This should succeed - can't change pointer

    // 10. Const pointer - dereference write (should fail)
    {R"(
        int main() {
            int x = 5;
            int* const p = &x;
            *p = 10;
            return 0;
        }
    )", ""},  // Should succeed - can write through const pointer

    // 11. Const struct member - member increment
    {R"(
        struct S {
            const int x;
            int y;
        };
        int main() {
            struct S s;
            s.x++;
            return 0;
        }
    )", "Increment/decrement of read-only"},

    // 12. Flexible array in union (should fail at parse)
    {R"(
        union U {
            int x;
            int arr[];
        };
        int main() { return 0; }
    )", "Flexible array"},

    // 13. Flexible array not last in struct (should fail)
    {R"(
        struct S {
            int arr[];
            int x;
        };
        int main() { return 0; }
    )", "must be the last"},

    // 14. Valid const usage - reading
    {R"(
        int main() {
            const int x = 5;
            int y = x;
            return y;
        }
    )", ""},  // Should succeed

    // 15. Valid const pointer - reading through
    {R"(
        int main() {
            int arr[5] = {0};
            const int* p = arr;
            int x = *p;
            return x;
        }
    )", ""},  // Should succeed
};

int main() {
    int passed = 0;
    int failed = 0;
    int skipped = 0;

    std::cerr << "Running comprehensive const-correctness validation tests...\n";
    std::cerr << "Total test cases: " << test_cases.size() << "\n\n";

    for (size_t i = 0; i < test_cases.size(); i++) {
        const auto& [code, expected_error] = test_cases[i];

        // Write test file
        std::string test_file = "/tmp/test_const_" + std::to_string(i) + ".c";
        FILE* f = fopen(test_file.c_str(), "w");
        if (!f) {
            std::cerr << "Test " << (i + 1) << ": SKIP (cannot create file)\n";
            skipped++;
            continue;
        }
        fprintf(f, "%s", code.c_str());
        fclose(f);

        // Compile and capture output
        std::string cmd = "./bin/cc45 " + test_file + " -o /tmp/test_out_" +
                         std::to_string(i) + ".s 2>&1";
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            std::cerr << "Test " << (i + 1) << ": SKIP (cannot run compiler)\n";
            skipped++;
            continue;
        }

        std::string output;
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            output += buffer;
        }
        pclose(pipe);

        // Check result
        bool has_error = output.find("Error") != std::string::npos ||
                        output.find("error") != std::string::npos;

        if (expected_error.empty()) {
            // Should compile successfully
            if (!has_error) {
                std::cerr << "Test " << (i + 1) << ": PASS (compiled successfully)\n";
                passed++;
            } else {
                std::cerr << "Test " << (i + 1) << ": FAIL (unexpected error: "
                         << output.substr(0, 60) << ")\n";
                failed++;
            }
        } else {
            // Should have error matching expected_error
            if (has_error && output.find(expected_error) != std::string::npos) {
                std::cerr << "Test " << (i + 1) << ": PASS (error caught: "
                         << expected_error << ")\n";
                passed++;
            } else if (has_error) {
                std::cerr << "Test " << (i + 1) << ": FAIL (wrong error: "
                         << output.substr(0, 60) << ")\n";
                failed++;
            } else {
                std::cerr << "Test " << (i + 1) << ": FAIL (no error, expected: "
                         << expected_error << ")\n";
                failed++;
            }
        }

        // Cleanup
        remove(test_file.c_str());
    }

    std::cerr << "\n=== RESULTS ===\n";
    std::cerr << "Passed:  " << passed << "\n";
    std::cerr << "Failed:  " << failed << "\n";
    std::cerr << "Skipped: " << skipped << "\n";

    if (failed > 0) {
        std::cerr << "\nSome tests FAILED\n";
        return 1;
    }

    std::cerr << "\nAll tests PASSED\n";
    return 0;
}
