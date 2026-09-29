// Test sum reduction idiom — pattern that detector expects:
// Loop body contains ONLY: accum += arr[i];
// No separate accumulator initialization (compiler will still need to init it first)

volatile char *result = (char *)0x4000;

// Direct pattern match: single statement in loop body
int test_single_stmt(int *arr, int n) {
    int sum = 0;
    for (int i = 0; i < n; i++)
        sum += arr[i];  // Single statement (no braces)
    return sum;
}

void main() {
    // Test 1: sum of [1, 2, 3]
    int arr1[3] = {1, 2, 3};
    int s1 = test_single_stmt(arr1, 3);
    result[0] = (s1 == 6) ? 0 : 1;

    // Marker
    result[1] = 0xAA;
}
