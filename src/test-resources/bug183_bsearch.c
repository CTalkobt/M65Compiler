// Bug #183: bsearch returns NULL when linked from library
// This test reproduces the issue with bsearch returning NULL for existing elements

#include <stdlib.h>
#include <stdio.h>

volatile char *test_result = (char *)0x4000;

// Test 1: Direct bsearch call (used when compiling bsearch.c with user code)
int sorted[5] = {10, 20, 30, 40, 50};

int cmp(const void *a, const void *b) {
    int va = *(const int *)a;
    int vb = *(const int *)b;
    return va - vb;
}

void test_bsearch() {
    // Test: Find middle element
    int key = 30;
    int *found = (int *)bsearch((const void *)&key, sorted, 5, sizeof(int), cmp);

    // Validation: should find sorted[2] = 30
    test_result[0] = (found != NULL) ? 0xAA : 0x01;  // Should be found
    test_result[1] = (found == &sorted[2]) ? 0xBB : 0x02;  // Should be at index 2
    if (found) {
        test_result[2] = (*found == 30) ? 0xCC : 0x03;  // Value should be 30
    } else {
        test_result[2] = 0x99;  // Not found marker
    }

    // Test: Find first element
    key = 10;
    found = (int *)bsearch((const void *)&key, sorted, 5, sizeof(int), cmp);
    test_result[3] = (found != NULL && *found == 10) ? 0xAA : 0x04;

    // Test: Find last element
    key = 50;
    found = (int *)bsearch((const void *)&key, sorted, 5, sizeof(int), cmp);
    test_result[4] = (found != NULL && *found == 50) ? 0xAA : 0x05;

    // Test: Find non-existent element (should return NULL)
    key = 35;
    found = (int *)bsearch((const void *)&key, sorted, 5, sizeof(int), cmp);
    test_result[5] = (found == NULL) ? 0xAA : 0x06;

    // Completion marker
    test_result[6] = 0xFF;
}

void main() {
    test_bsearch();
}
