// Test arithmetic expression reentrancy
// Exercises nested multiply/add that use the MEGA65 hardware math registers
// Results written to $4000+ for mmemu validation

volatile char *result = (char *)0x4000;

int a = 3;
int b = 5;
int c = 2;
int d = 4;

void main() {
    // Test 1: simple multiply — a * c = 3 * 2 = 6
    result[0] = a * c;

    // Test 2: simple add — a + b = 3 + 5 = 8
    result[1] = a + b;

    // Test 3: multiply then add — a * b + c = 15 + 2 = 17 = $11
    result[2] = a * b + c;

    // Test 4: nested multiplies — a * b + c * d = 15 + 8 = 23 = $17
    // This is the reentrant case: the + evaluates left (a*b) into $D770,
    // then right (c*d) clobbers $D770 before + can read it
    result[3] = a * b + c * d;

    // Test 5: marker
    result[4] = 0xAA;
}
