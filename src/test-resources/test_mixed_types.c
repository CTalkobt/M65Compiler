// Test: Mixed char and int types in expressions and assignments
// Validates correct code generation for type widening and narrowing.

int main() {
    char c = 200;
    int i = 1000;

    // Char widened to int in expression
    int r1 = c + i;
    if (r1 != 1200) return 1;

    // Int narrowed to char in assignment
    char c2 = i;  // truncated to low byte: 1000 & 0xFF = 232
    if (c2 != 232) return 2;

    // Char-only arithmetic
    char a = 100;
    char b = 50;
    char sum = a + b;
    if (sum != 150) return 3;

    // Char overflow wraps
    char big = 250;
    char small = 10;
    char wrapped = big + small; // 260 & 0xFF = 4
    if (wrapped != 4) return 4;

    // Int array with char index
    int arr[4];
    char idx = 2;
    arr[idx] = 42;
    if (arr[2] != 42) return 5;

    return 0;
}
