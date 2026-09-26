// Test linear search idiom detection and transformation
// Finds first occurrence of a value in a byte array

volatile char *result = (char *)0x4000;

// Linear search: for(i=0; i<n; i++) if(arr[i]==target) { found=1; break; }
int search_byte(unsigned char *arr, int n, unsigned char target) {
    int found = -1;
    for (int i = 0; i < n; i++)
        if (arr[i] == target) { found = i; break; }
    return found;
}

void main() {
    // Test 1: Find 'X' in "HELLO" (not found, should be -1)
    unsigned char arr1[5] = {'H', 'E', 'L', 'L', 'O'};
    int idx1 = search_byte(arr1, 5, 'X');
    result[0] = (idx1 == -1) ? 0 : 1;

    // Test 2: Find 'L' in "HELLO" (found at index 2)
    int idx2 = search_byte(arr1, 5, 'L');
    result[1] = (idx2 == 2) ? 0 : 1;

    // Test 3: Find 'H' in "HELLO" (found at index 0)
    int idx3 = search_byte(arr1, 5, 'H');
    result[2] = (idx3 == 0) ? 0 : 1;

    // Test 4: Find 'O' in "HELLO" (found at index 4)
    int idx4 = search_byte(arr1, 5, 'O');
    result[3] = (idx4 == 4) ? 0 : 1;

    // Marker
    result[4] = 0xAA;
}
