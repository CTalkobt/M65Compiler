// Test: register keyword — ZP allocation for local variables
// Returns 0 on success, nonzero on failure.

int add(int a, int b) {
    return a + b;
}

int main() {
    // Test 1: register int with initializer
    register int i = 10;
    if (i != 10) return 1;

    // Test 2: register char with initializer
    register char c = 42;
    if (c != 42) return 2;

    // Test 3: register int assignment
    i = 99;
    if (i != 99) return 3;

    // Test 4: register char assignment
    c = 7;
    if (c != 7) return 4;

    // Test 5: register int in loop (inc optimization)
    register int sum = 0;
    for (i = 0; i < 5; i = i + 1) {
        sum = sum + i;
    }
    if (sum != 10) return 5;  // 0+1+2+3+4 = 10

    // Test 6: register var interacts with stack vars
    int stack_a = 100;
    register int reg_b = 200;
    int stack_c = 300;
    if (stack_a + reg_b + stack_c != 600) return 6;

    // Test 7: register var passed to function
    register int x = 15;
    register int y = 25;
    int result = add(x, y);
    if (result != 40) return 7;

    // Test 8: register var receives function return value
    register int z = add(3, 4);
    if (z != 7) return 8;

    // Test 9: register int literal store (16-bit)
    register int big = 1000;
    if (big != 1000) return 9;

    // Test 10: register char literal store
    register char ch = 255;
    if (ch != 255) return 10;

    // Test 11: register int increment by 1 (inw optimization)
    register int counter = 0;
    counter = counter + 1;
    counter = counter + 1;
    counter = counter + 1;
    if (counter != 3) return 11;

    // Test 12: register char decrement
    register char dc = 10;
    dc = dc - 1;
    dc = dc - 1;
    if (dc != 8) return 12;

    // Test 13: multiple register vars in nested scope
    register int outer = 1;
    {
        register int inner = 2;
        outer = outer + inner;
    }
    if (outer != 3) return 13;

    // Test 14: register in for-loop init
    int fsum = 0;
    for (register int fi = 1; fi <= 5; fi = fi + 1) {
        fsum = fsum + fi;
    }
    if (fsum != 15) return 14;  // 1+2+3+4+5 = 15

    // Test 15: mixed register and non-register in expressions
    int sa = 10;
    register int rb = 20;
    int sc = 30;
    register int rd = 40;
    int total = sa + rb + sc + rd;
    if (total != 100) return 15;

    // Test 16: register var self-assignment (should be no-op)
    register int self = 42;
    self = self;
    if (self != 42) return 16;

    return 0;  // All tests passed
}
