/* test_harness.c — Test Framework Implementation */

#include "test_harness.h"
#include <time.h>

test_stats_t test_stats = {0};
test_context_t current_test = {0};

void test_harness_init(void) {
    test_stats.total_tests = 0;
    test_stats.passed = 0;
    test_stats.failed = 0;
    test_stats.skipped = 0;
    test_stats.total_time_ms = 0;

    printf("=== TEST HARNESS INITIALIZED ===\n\n");
}

void test_harness_record_result(void) {
    test_stats.total_tests++;

    if (current_test.passed) {
        test_stats.passed++;
        printf("[PASS] %s::%s (%d assertions)\n",
               current_test.suite_name, current_test.test_name,
               current_test.assertions);
    } else {
        test_stats.failed++;
        printf("[FAIL] %s::%s (%d failures / %d assertions)\n",
               current_test.suite_name, current_test.test_name,
               current_test.failures, current_test.assertions);
    }
}

void test_harness_run_test(void (*test_fn)(void), const char *name) {
    if (!test_fn) return;

    current_test.test_name = name;
    current_test.passed = 1;
    current_test.assertions = 0;
    current_test.failures = 0;

    test_fn();
    test_harness_record_result();
}

test_stats_t test_harness_get_stats(void) {
    return test_stats;
}

void test_harness_print_summary(void) {
    printf("\n=== TEST SUMMARY ===\n");
    printf("Total:   %d\n", test_stats.total_tests);
    printf("Passed:  %d\n", test_stats.passed);
    printf("Failed:  %d\n", test_stats.failed);
    printf("Skipped: %d\n", test_stats.skipped);

    float pass_rate = test_stats.total_tests > 0 ?
        (float)test_stats.passed / test_stats.total_tests * 100.0f : 0.0f;

    printf("Pass Rate: %.1f%%\n", pass_rate);

    if (test_stats.total_time_ms > 0) {
        printf("Total Time: %dms\n", test_stats.total_time_ms);
    }

    printf("\n");

    if (test_stats.failed == 0) {
        printf("✓ ALL TESTS PASSED\n");
    } else {
        printf("✗ %d TEST(S) FAILED\n", test_stats.failed);
    }
}

unsigned int test_get_time_ms(void) {
    /* Simple millisecond timer using clock() */
    return (unsigned int)(clock() / (CLOCKS_PER_SEC / 1000));
}
