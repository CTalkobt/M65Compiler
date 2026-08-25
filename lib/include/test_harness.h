/* test_harness.h — Universal Test Framework
 *
 * Minimal, efficient test harness for unit and integration testing.
 * Zero external dependencies, works with compiler/assembler test builds.
 */

#ifndef TEST_HARNESS_H
#define TEST_HARNESS_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* Test statistics */
typedef struct {
    int total_tests;
    int passed;
    int failed;
    int skipped;
    unsigned int total_time_ms;
} test_stats_t;

/* Per-test context */
typedef struct {
    const char *test_name;
    const char *suite_name;
    int passed;
    int assertions;
    int failures;
    unsigned int start_time;
} test_context_t;

/* Global test state */
extern test_stats_t test_stats;
extern test_context_t current_test;

/* ===== Test Registration ===== */

#define TEST_SUITE(name) \
    void test_suite_##name(void); \
    void register_suite_##name(void) { test_suite_##name(); }

#define TEST(suite_name, test_name) \
    void test_##suite_name##_##test_name(void); \
    void run_test_##suite_name##_##test_name(void) { \
        current_test.test_name = #test_name; \
        current_test.suite_name = #suite_name; \
        current_test.passed = 1; \
        current_test.assertions = 0; \
        current_test.failures = 0; \
        test_##suite_name##_##test_name(); \
        test_harness_record_result(); \
    } \
    void test_##suite_name##_##test_name(void)

/* ===== Assertions ===== */

#define ASSERT_TRUE(condition) \
    do { \
        current_test.assertions++; \
        if (!(condition)) { \
            current_test.passed = 0; \
            current_test.failures++; \
            printf("  FAIL: %s:%d - Expected true\n", __FILE__, __LINE__); \
        } \
    } while (0)

#define ASSERT_FALSE(condition) \
    do { \
        current_test.assertions++; \
        if ((condition)) { \
            current_test.passed = 0; \
            current_test.failures++; \
            printf("  FAIL: %s:%d - Expected false\n", __FILE__, __LINE__); \
        } \
    } while (0)

#define ASSERT_EQ(a, b) \
    do { \
        current_test.assertions++; \
        if ((a) != (b)) { \
            current_test.passed = 0; \
            current_test.failures++; \
            printf("  FAIL: %s:%d - Expected %d, got %d\n", \
                   __FILE__, __LINE__, (int)(b), (int)(a)); \
        } \
    } while (0)

#define ASSERT_NEQ(a, b) \
    do { \
        current_test.assertions++; \
        if ((a) == (b)) { \
            current_test.passed = 0; \
            current_test.failures++; \
            printf("  FAIL: %s:%d - Expected not equal, got %d\n", \
                   __FILE__, __LINE__, (int)(a)); \
        } \
    } while (0)

#define ASSERT_GT(a, b) \
    do { \
        current_test.assertions++; \
        if (!((a) > (b))) { \
            current_test.passed = 0; \
            current_test.failures++; \
            printf("  FAIL: %s:%d - Expected %d > %d\n", \
                   __FILE__, __LINE__, (int)(a), (int)(b)); \
        } \
    } while (0)

#define ASSERT_LT(a, b) \
    do { \
        current_test.assertions++; \
        if (!((a) < (b))) { \
            current_test.passed = 0; \
            current_test.failures++; \
            printf("  FAIL: %s:%d - Expected %d < %d\n", \
                   __FILE__, __LINE__, (int)(a), (int)(b)); \
        } \
    } while (0)

#define ASSERT_STR_EQ(a, b) \
    do { \
        current_test.assertions++; \
        if (strcmp((a), (b)) != 0) { \
            current_test.passed = 0; \
            current_test.failures++; \
            printf("  FAIL: %s:%d - String mismatch: '%s' vs '%s'\n", \
                   __FILE__, __LINE__, (a), (b)); \
        } \
    } while (0)

#define ASSERT_NOT_NULL(ptr) \
    do { \
        current_test.assertions++; \
        if ((ptr) == NULL) { \
            current_test.passed = 0; \
            current_test.failures++; \
            printf("  FAIL: %s:%d - Expected non-NULL\n", __FILE__, __LINE__); \
        } \
    } while (0)

#define ASSERT_NULL(ptr) \
    do { \
        current_test.assertions++; \
        if ((ptr) != NULL) { \
            current_test.passed = 0; \
            current_test.failures++; \
            printf("  FAIL: %s:%d - Expected NULL\n", __FILE__, __LINE__); \
        } \
    } while (0)

/* ===== Test Harness API ===== */

void test_harness_init(void);
void test_harness_record_result(void);
void test_harness_run_test(void (*test_fn)(void), const char *name);
test_stats_t test_harness_get_stats(void);
void test_harness_print_summary(void);

/* Get current time in milliseconds (for benchmarking) */
unsigned int test_get_time_ms(void);

#endif
