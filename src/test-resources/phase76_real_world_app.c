/**
 * Real-World Application: Data Processing Engine
 * Phase 76: Real-World Application Performance Testing
 * 
 * This program demonstrates realistic code patterns:
 * - Array processing (buffer, lookup tables)
 * - Recursive algorithms (tree traversal, quicksort)
 * - Function dispatch (callback patterns)
 * - Memory management (dynamic allocation)
 * - String processing (pattern matching)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * Data Structures
 * ============================================================================ */

typedef struct {
    int id;
    int value;
    int flags;
} DataRecord;

typedef struct {
    int key;
    int (*handler)(int);
} DispatchEntry;

/* ============================================================================
 * Lookup Tables (Array Optimization)
 * ============================================================================ */

static int sine_table[256] = {
    0,   3,   6,   9,  12,  15,  18,  21,  24,  27,  30,  33,  36,  39,  42,  45,
   48,  51,  54,  57,  60,  63,  66,  69,  72,  75,  78,  81,  84,  87,  90,  93,
   96,  99, 102, 105, 108, 111, 114, 117, 120, 123, 126, 127, 126, 123, 120, 117,
  114, 111, 108, 105, 102,  99,  96,  93,  90,  87,  84,  81,  78,  75,  72,  69,
   66,  63,  60,  57,  54,  51,  48,  45,  42,  39,  36,  33,  30,  27,  24,  21,
   18,  15,  12,   9,   6,   3,   0,  -3,  -6,  -9, -12, -15, -18, -21, -24, -27,
  -30, -33, -36, -39, -42, -45, -48, -51, -54, -57, -60, -63, -66, -69, -72, -75,
  -78, -81, -84, -87, -90, -93, -96, -99,-102,-105,-108,-111,-114,-117,-120,-123,
 -126,-127,-126,-123,-120,-117,-114,-111,-108,-105,-102, -99, -96, -93, -90, -87,
  -84, -81, -78, -75, -72, -69, -66, -63, -60, -57, -54, -51, -48, -45, -42, -39,
  -36, -33, -30, -27, -24, -21, -18, -15, -12,  -9,  -6,  -3,   0,   3,   6,   9,
   12,  15,  18,  21,  24,  27,  30,  33,  36,  39,  42,  45,  48,  51,  54,  57,
   60,  63,  66,  69,  72,  75,  78,  81,  84,  87,  90,  93,  96,  99, 102, 105,
  108, 111, 114, 117, 120, 123, 126, 127, 126, 123, 120, 117, 114, 111, 108, 105,
  102,  99,  96,  93,  90,  87,  84,  81,  78,  75,  72,  69,  66,  63,  60,  57,
   54,  51,  48,  45,  42,  39,  36,  33,  30,  27,  24,  21,  18,  15,  12,   9
};

/* ============================================================================
 * Handler Functions (Dispatcher Pattern)
 * ============================================================================ */

static int handler_add(int val) {
    return val + 10;
}

static int handler_mul(int val) {
    return val * 2;
}

static int handler_sub(int val) {
    return val - 5;
}

static int handler_div(int val) {
    return val > 0 ? val / 2 : 0;
}

/* ============================================================================
 * Recursive Algorithms (Must opt out of SAC - recursive functions need stack)
 * ============================================================================ */

#pragma cc45 recurse
static int binary_search(int *arr, int low, int high, int target) {
    if (low > high) return -1;

    int mid = (low + high) / 2;
    if (arr[mid] == target) return mid;
    if (arr[mid] < target) return binary_search(arr, mid + 1, high, target);
    return binary_search(arr, low, mid - 1, target);
}

#pragma cc45 no_static_alloc
static int partition(int *arr, int low, int high) {
    int pivot = arr[high];
    int i = low - 1;

    for (int j = low; j < high; j++) {
        if (arr[j] < pivot) {
            i++;
            int tmp = arr[i];
            arr[i] = arr[j];
            arr[j] = tmp;
        }
    }

    int tmp = arr[i + 1];
    arr[i + 1] = arr[high];
    arr[high] = tmp;
    return i + 1;
}

#pragma cc45 no_static_alloc
static void quicksort(int *arr, int low, int high) {
    if (low < high) {
        int pi = partition(arr, low, high);
        quicksort(arr, low, pi - 1);
        quicksort(arr, pi + 1, high);
    }
}

/* ============================================================================
 * Data Processing Functions (SAC Candidates)
 * ============================================================================ */

/* Process array with dispatcher */
static int dispatch_process(int val, int op) {
    switch (op) {
        case 0: return handler_add(val);
        case 1: return handler_mul(val);
        case 2: return handler_sub(val);
        case 3: return handler_div(val);
        default: return val;
    }
}

/* Pattern matching */
static int match_pattern(const char *str, const char *pat) {
    int i = 0, j = 0;
    while (str[i] && pat[j]) {
        if (str[i] == pat[j]) {
            j++;
        }
        i++;
    }
    return pat[j] == '\0';
}

/* Array reduction */
static int sum_array(int *arr, int size) {
    int sum = 0;
    for (int i = 0; i < size; i++) {
        sum += arr[i];
    }
    return sum;
}

/* ============================================================================
 * Main Processing Workflow
 * ============================================================================ */

int main() {
    printf("Phase 76: Real-World Application Testing\n");
    printf("=========================================\n\n");
    
    /* Test 1: Lookup Table Access */
    printf("Test 1: Lookup Table (sine)\n");
    int sine_vals[4];
    for (int i = 0; i < 4; i++) {
        sine_vals[i] = sine_table[i * 64];
        printf("  sine[%d] = %d\n", i * 64, sine_vals[i]);
    }
    
    /* Test 2: Recursive Binary Search */
    printf("\nTest 2: Binary Search\n");
    int sorted_arr[10] = {1, 3, 5, 7, 9, 11, 13, 15, 17, 19};
    int pos = binary_search(sorted_arr, 0, 9, 13);
    printf("  Found 13 at position: %d\n", pos);
    
    /* Test 3: Quicksort */
    printf("\nTest 3: Quicksort\n");
    int data[8] = {64, 34, 25, 12, 22, 11, 90, 88};
    printf("  Before: ");
    for (int i = 0; i < 8; i++) printf("%d ", data[i]);
    quicksort(data, 0, 7);
    printf("\n  After:  ");
    for (int i = 0; i < 8; i++) printf("%d ", data[i]);
    printf("\n");
    
    /* Test 4: Dispatcher Pattern */
    printf("\nTest 4: Dispatcher Pattern\n");
    int result = dispatch_process(20, 0);
    printf("  dispatch(20, ADD) = %d\n", result);
    result = dispatch_process(20, 1);
    printf("  dispatch(20, MUL) = %d\n", result);
    
    /* Test 5: Pattern Matching */
    printf("\nTest 5: Pattern Matching\n");
    int match = match_pattern("HELLO", "HLO");
    printf("  Pattern HLO in HELLO: %s\n", match ? "YES" : "NO");
    
    /* Test 6: Array Reduction */
    printf("\nTest 6: Array Reduction\n");
    int vals[5] = {10, 20, 30, 40, 50};
    int total = sum_array(vals, 5);
    printf("  Sum of array: %d\n", total);
    
    printf("\n✓ All tests completed\n");
    return 0;
}
