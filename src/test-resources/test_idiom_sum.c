// Test sum reduction idiom detection and transformation
// Computes sum of array and verifies result

volatile char *result = (char *)0x4000;

int sum_reduce_int(int *arr, int n) {
    int sum = 0;
    for (int i = 0; i < n; i++) {
        sum += arr[i];
    }
    return sum;
}

void main() {
    // Test 1: sum of [1, 2, 3, 4, 5] = 15
    int arr1[5] = {1, 2, 3, 4, 5};
    int s1 = sum_reduce_int(arr1, 5);
    result[0] = (s1 == 15) ? 0 : 1;

    // Test 2: sum of [10, 20, -5] = 25
    int arr2[3] = {10, 20, -5};
    int s2 = sum_reduce_int(arr2, 3);
    result[1] = (s2 == 25) ? 0 : 1;

    // Test 3: empty array, n=0
    int arr3[1] = {999};
    int s3 = sum_reduce_int(arr3, 0);
    result[2] = (s3 == 0) ? 0 : 1;

    // Test 4: single element
    int arr4[1] = {42};
    int s4 = sum_reduce_int(arr4, 1);
    result[3] = (s4 == 42) ? 0 : 1;

    // Marker
    result[4] = 0xAA;
}
