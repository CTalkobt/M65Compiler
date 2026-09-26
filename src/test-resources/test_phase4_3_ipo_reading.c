// Phase 4.3: IPO Hints Reading & Aggregation Test
// Tests that linker can read and aggregate IPO hints from .o45 files

// Simple leaf function
int compute(int x) {
    return x * 2;
}

// Caller function
int process(int a, int b) {
    return compute(a) + compute(b);
}

// Main entry
int main(void) {
    return process(5, 10);
}
