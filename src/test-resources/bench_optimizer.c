// bench_optimizer.c — Benchmark suite measuring optimizer pass impact
// Compile with: cc45 -c bench_optimizer.c -o bench_optimizer.o45
// Compare sizes: cc45 bench_optimizer.c -o bench_opt.s
//                cc45 -O0 bench_optimizer.c -o bench_noopt.s
// Run in emulator to verify correctness.

volatile char *result = (char *)0x4000;

// --- Benchmark 1: LICM (#130) ---
// Loop-invariant r*40 should be hoisted out of inner loop
void bench_licm() {
    char screen[200];
    int r, c;
    for (r = 0; r < 5; r++) {
        for (c = 0; c < 40; c++) {
            screen[r * 40 + c] = (char)(r + c);
        }
    }
    result[0] = screen[0];   // 0
    result[1] = screen[41];  // 2 (r=1, c=1)
    result[2] = screen[199]; // 43 (r=4, c=39) → 0x2B
}

// --- Benchmark 2: Strength Reduction (#131) ---
// MUL by power-of-2 → SHL, DIV_U → SHR, MOD_U → AND
volatile int sr_input = 100;
void bench_strength_reduction() {
    int x = sr_input;
    result[3] = x * 4;    // 400 → 0x90 (low byte)
    result[4] = x / 8;    // 12
    result[5] = x % 16;   // 4
}

// --- Benchmark 3: Type Narrowing (#137) ---
// I16 operations truncated to I8 should use I8 arithmetic
volatile int tn_a = 7, tn_b = 3;
void bench_type_narrowing() {
    int x = tn_a;
    int y = tn_b;
    result[6] = (char)(x + y);   // 10
    result[7] = (char)(x * y);   // 21
    result[8] = (char)(x - y);   // 4
}

// --- Benchmark 4: Algebraic Simplification (#132) ---
// These should all simplify to trivial operations
volatile int alg_x = 42;
void bench_algebraic() {
    int x = alg_x;
    result[9]  = (char)(x + 0);   // 42
    result[10] = (char)(x - x);   // 0
    result[11] = (char)(x | 0);   // 42
}

// --- Benchmark 5: Branch Folding (#134) ---
// Constant conditions should be eliminated
void bench_branch_fold() {
    if (1) {
        result[12] = 0x42;
    } else {
        result[12] = 0xFF; // dead code
    }
    if (0) {
        result[13] = 0xFF; // dead code
    } else {
        result[13] = 0x43;
    }
}

// --- Benchmark 6: Array loop (combined test) ---
// Tests LICM + strength reduction + type narrowing together
int arr[10] = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
void bench_array_loop() {
    int sum = 0;
    int i;
    for (i = 0; i < 10; i++) {
        sum += arr[i];
    }
    result[14] = (char)sum;  // 550 → 0x26 (low byte)
    result[15] = 0xAA;       // marker
}

void main() {
    bench_licm();
    bench_strength_reduction();
    bench_type_narrowing();
    bench_algebraic();
    bench_branch_fold();
    bench_array_loop();
}
