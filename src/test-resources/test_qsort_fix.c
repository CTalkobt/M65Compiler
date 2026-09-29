// Test qsort with function pointer comparator
#include <stdlib.h>

volatile char *test_result = (char *)0x4000;
int compare_calls = 0;

int compare_ints(const void *a, const void *b) {
    compare_calls++;
    int va = *(int *)a;
    int vb = *(int *)b;
    if (va < vb) return -1;
    if (va > vb) return 1;
    return 0;
}

void main() {
    int arr[] = {50, 10, 40, 20, 30};

    // Sort the array
    qsort(arr, 5, sizeof(int), compare_ints);

    // Check results
    int sorted = 1;
    if (arr[0] != 10) sorted = 0;
    if (arr[1] != 20) sorted = 0;
    if (arr[2] != 30) sorted = 0;
    if (arr[3] != 40) sorted = 0;
    if (arr[4] != 50) sorted = 0;

    // Store results
    test_result[0] = sorted ? 0xAA : 0x01;            // Sorted correctly?
    test_result[1] = (compare_calls > 0) ? 0xBB : 0x02; // Comparator called?
    test_result[2] = 0xFF;                            // Marker
}
