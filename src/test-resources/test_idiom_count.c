// Test count loop idiom detection and transformation
// Counts occurrences of a value in a byte array

volatile char *result = (char *)0x4000;

// Count pattern: for(i=0; i<n; i++) if(arr[i]==target) count++;
int count_bytes(unsigned char *arr, int n, unsigned char target) {
    int count = 0;
    for (int i = 0; i < n; i++)
        if (arr[i] == target) count++;
    return count;
}

void main() {
    // Test 1: Count 'L' in "HELLO" (should be 2)
    unsigned char arr1[5] = {'H', 'E', 'L', 'L', 'O'};
    int c1 = count_bytes(arr1, 5, 'L');
    result[0] = (c1 == 2) ? 0 : 1;

    // Test 2: Count 'X' in "HELLO" (should be 0)
    int c2 = count_bytes(arr1, 5, 'X');
    result[1] = (c2 == 0) ? 0 : 1;

    // Test 3: Count 'H' in "HELLO" (should be 1)
    int c3 = count_bytes(arr1, 5, 'H');
    result[2] = (c3 == 1) ? 0 : 1;

    // Test 4: Empty array (n=0, should be 0)
    unsigned char arr2[1] = {0};
    int c4 = count_bytes(arr2, 0, 'A');
    result[3] = (c4 == 0) ? 0 : 1;

    // Marker
    result[4] = 0xAA;
}
