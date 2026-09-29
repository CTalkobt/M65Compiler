// Test dot product loop idiom detection and transformation
// Computes dot product of two arrays

volatile char *result = (char *)0x4000;

// Dot product pattern: for(i=0; i<n; i++) sum += a[i]*b[i];
int dot_product(int *a, int *b, int n) {
    int sum = 0;
    for (int i = 0; i < n; i++)
        sum += a[i] * b[i];
    return sum;
}

void main() {
    // Test 1: dot([1,2,3], [4,5,6]) = 1*4 + 2*5 + 3*6 = 4 + 10 + 18 = 32
    int a1[3] = {1, 2, 3};
    int b1[3] = {4, 5, 6};
    int dp1 = dot_product(a1, b1, 3);
    result[0] = (dp1 == 32) ? 0 : 1;

    // Test 2: dot([0,0,0], [1,2,3]) = 0
    int a2[3] = {0, 0, 0};
    int b2[3] = {1, 2, 3};
    int dp2 = dot_product(a2, b2, 3);
    result[1] = (dp2 == 0) ? 0 : 1;

    // Test 3: dot([1,1], [2,2]) = 1*2 + 1*2 = 4
    int a3[2] = {1, 1};
    int b3[2] = {2, 2};
    int dp3 = dot_product(a3, b3, 2);
    result[2] = (dp3 == 4) ? 0 : 1;

    // Test 4: Empty arrays (n=0) = 0
    int a4[1] = {999};
    int b4[1] = {999};
    int dp4 = dot_product(a4, b4, 0);
    result[3] = (dp4 == 0) ? 0 : 1;

    // Marker
    result[4] = 0xAA;
}
