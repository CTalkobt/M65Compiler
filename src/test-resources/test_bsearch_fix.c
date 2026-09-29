// Test bsearch with the fix
#include <stdlib.h>

int cmp_count = 0;
int found_result = -1;

int compare_ints(const void *a, const void *b) {
    cmp_count++;
    int va = *(int *)a;
    int vb = *(int *)b;
    if (va < vb) return -1;
    if (va > vb) return 1;
    return 0;
}

int main() {
    int arr[] = {10, 20, 30, 40, 50};
    int key = 30;

    // Test bsearch with function pointer
    int *result = (int *)bsearch(&key, arr, 5, sizeof(int), compare_ints);

    // Check if we found the key and comparator was called
    if (result != NULL && *result == 30 && cmp_count > 0) {
        return 0;  // Success
    }
    return 1;  // Failure
}
