// test_harness.hpp — the shared part of the validation tests (#96)
//
// Each test_validation_<category>.cpp (and test_segment_emission.cpp,
// test_move_fill.cpp) is a category of bin/test_validation: its main() is
// compiled as tv_<category>_main (-Dmain=...) and test_validation_main.cpp
// runs the categories asked for. A category counts its own checks (these are
// static: one count per category) and prints its summary.
//
//   CHECK(cond, msg)   a check: FAIL lines always (with the file and line),
//                      PASS lines with TV_VERBOSE=1
//   readFile(path)     a file's contents ("" when it cannot be read)
#pragma once

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>

static int tests_passed = 0;
static int tests_failed = 0;

inline bool tvVerbose() {
    static const bool v = std::getenv("TV_VERBOSE") != nullptr;
    return v;
}

#define CHECK(cond, msg) do { \
    if (!(cond)) { \
        printf("FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__); \
        tests_failed++; \
    } else { \
        if (tvVerbose()) printf("PASS: %s\n", msg); \
        tests_passed++; \
    } \
} while (0)

[[maybe_unused]] static std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}
