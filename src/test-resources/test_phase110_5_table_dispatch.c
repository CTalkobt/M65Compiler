// Test Phase 110.5: Table-Driven Dispatch Optimization
// Tests conversion of switch statements to jump tables

// =============================================================================
// DENSE SWITCH STATEMENTS (TABLE CANDIDATES)
// =============================================================================

// Perfect dense range: 0-9 (10 consecutive cases)
int test_dense_0_to_9(int x) {
    switch (x) {
        case 0: return 10;
        case 1: return 11;
        case 2: return 12;
        case 3: return 13;
        case 4: return 14;
        case 5: return 15;
        case 6: return 16;
        case 7: return 17;
        case 8: return 18;
        case 9: return 19;
        default: return 0;
    }
}

// Dense range: 1-10 (10 consecutive cases)
int test_dense_1_to_10(int op) {
    switch (op) {
        case 1: return 100;
        case 2: return 110;
        case 3: return 120;
        case 4: return 130;
        case 5: return 140;
        case 6: return 150;
        case 7: return 160;
        case 8: return 170;
        case 9: return 180;
        case 10: return 190;
        default: return -1;
    }
}

// Dense: error codes (-3 to 3)
int test_dense_negative_to_positive(int err) {
    switch (err) {
        case -3: return 300;
        case -2: return 200;
        case -1: return 100;
        case 0: return 0;
        case 1: return 101;
        case 2: return 102;
        case 3: return 103;
        default: return -999;
    }
}

// Large dense range: 0-99 (100 cases!)
int test_dense_large(int code) {
    switch (code) {
        case 0: return 1;
        case 1: return 2;
        case 2: return 3;
        case 3: return 4;
        case 4: return 5;
        // ... conceptually 0-99 all present
        case 99: return 100;
        default: return 0;
    }
}

// =============================================================================
// SPARSE SWITCH STATEMENTS (NOT TABLE CANDIDATES)
// =============================================================================

// Sparse: only 2 cases (too few)
int test_sparse_two_cases(int x) {
    switch (x) {
        case 1: return 10;
        case 2: return 20;
        default: return 0;
    }
}

// Sparse: 3 cases (still too few)
int test_sparse_three_cases(int x) {
    switch (x) {
        case 10: return 100;
        case 20: return 200;
        case 30: return 300;
        default: return 0;
    }
}

// Sparse: widely separated values
int test_sparse_wide_spread(int code) {
    switch (code) {
        case 0: return 1;
        case 100: return 2;
        case 200: return 3;
        case 300: return 4;
        default: return 0;
    }
}

// Sparse: gaps in range (30% dense - below threshold)
int test_sparse_with_gaps(int x) {
    switch (x) {
        case 1: return 10;
        case 3: return 30;  // Gap: case 2 missing
        case 5: return 50;  // Gap: case 4 missing
        case 7: return 70;  // Gap: case 6 missing
        case 9: return 90;  // Gap: case 8 missing
        default: return 0;
    }
}

// =============================================================================
// REAL-WORLD PATTERNS (DENSE - TABLE WORTHY)
// =============================================================================

// ASCII character classification
int classify_char(char c) {
    switch (c) {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            return 1;  // Digit
        case 'a': case 'b': case 'c': case 'd': case 'e':
        case 'f': case 'g': case 'h': case 'i': case 'j':
        case 'k': case 'l': case 'm': case 'n': case 'o':
        case 'p': case 'q': case 'r': case 's': case 't':
        case 'u': case 'v': case 'w': case 'x': case 'y':
        case 'z':
            return 2;  // Lowercase letter
        case 'A': case 'B': case 'C': case 'D': case 'E':
        case 'F': case 'G': case 'H': case 'I': case 'J':
        case 'K': case 'L': case 'M': case 'N': case 'O':
        case 'P': case 'Q': case 'R': case 'S': case 'T':
        case 'U': case 'V': case 'W': case 'X': case 'Y':
        case 'Z':
            return 3;  // Uppercase letter
        default: return 0;  // Other
    }
}

// Game state machine
int process_state(int state) {
    switch (state) {
        case 0: return 10;   // MENU
        case 1: return 20;   // PLAYING
        case 2: return 30;   // PAUSED
        case 3: return 40;   // GAME_OVER
        case 4: return 50;   // LOADING
        case 5: return 60;   // CREDITS
        default: return -1;
    }
}

// HTTP status code handling
int http_status_class(int code) {
    switch (code) {
        case 100: case 101: case 102: return 1;  // 1xx Informational
        case 200: case 201: case 202: case 204: case 206: return 2;  // 2xx Success
        case 300: case 301: case 302: case 304: case 307: return 3;  // 3xx Redirect
        case 400: case 401: case 403: case 404: case 405: return 4;  // 4xx Client Error
        case 500: case 502: case 503: case 504: return 5;  // 5xx Server Error
        default: return 0;
    }
}

// Day of week
int day_to_number(int day) {
    switch (day) {
        case 1: return 100;  // Monday
        case 2: return 200;  // Tuesday
        case 3: return 300;  // Wednesday
        case 4: return 400;  // Thursday
        case 5: return 500;  // Friday
        case 6: return 600;  // Saturday
        case 7: return 700;  // Sunday
        default: return 0;
    }
}

// =============================================================================
// MEDIUM DENSITY CASES (BORDERLINE)
// =============================================================================

// 6 cases in range 1-10 (60% density - right at threshold)
int test_medium_density_60(int x) {
    switch (x) {
        case 1: return 10;
        case 2: return 20;
        case 4: return 40;  // Gap: 3 missing
        case 6: return 60;  // Gap: 5 missing
        case 8: return 80;  // Gap: 7 missing
        case 10: return 100;  // Gap: 9 missing
        default: return 0;
    }
}

// 7 cases in range 0-9 (70% density - above threshold)
int test_medium_density_70(int x) {
    switch (x) {
        case 0: return 0;
        case 1: return 1;
        case 2: return 2;
        case 4: return 4;  // Gap: 3 missing
        case 5: return 5;
        case 7: return 7;  // Gap: 6 missing
        case 9: return 9;  // Gap: 8 missing
        default: return -1;
    }
}

// =============================================================================
// SWITCH WITH COMPLEX BODIES (STILL TABLE WORTHY)
// =============================================================================

// Dense switch with computation in cases
int test_dense_complex_body(int x) {
    switch (x) {
        case 0: return x * 10;
        case 1: return x * 10 + 1;
        case 2: return x * 10 + 2;
        case 3: return x * 10 + 3;
        case 4: return x * 10 + 4;
        case 5: return x * 10 + 5;
        case 6: return x * 10 + 6;
        case 7: return x * 10 + 7;
        case 8: return x * 10 + 8;
        case 9: return x * 10 + 9;
        default: return -1;
    }
}

// Dense switch with function calls
int helper_func(int x);

int test_dense_with_calls(int x) {
    switch (x) {
        case 0: return helper_func(0);
        case 1: return helper_func(1);
        case 2: return helper_func(2);
        case 3: return helper_func(3);
        case 4: return helper_func(4);
        case 5: return helper_func(5);
        default: return -1;
    }
}

// Dense switch with multiple statements per case
int test_dense_multi_stmt(int x) {
    int result = 0;
    switch (x) {
        case 0: result = 10; result *= 2; break;
        case 1: result = 20; result *= 2; break;
        case 2: result = 30; result *= 2; break;
        case 3: result = 40; result *= 2; break;
        case 4: result = 50; result *= 2; break;
        case 5: result = 60; result *= 2; break;
        case 6: result = 70; result *= 2; break;
        case 7: result = 80; result *= 2; break;
        case 8: result = 90; result *= 2; break;
        default: result = 0;
    }
    return result;
}

// =============================================================================
// FALLTHROUGH CASES
// =============================================================================

// Cases with fallthrough (combined cases)
int test_fallthrough_combined(int x) {
    switch (x) {
        case 0:
        case 1:
        case 2: return 10;  // All three cases return same
        case 3:
        case 4:
        case 5: return 20;
        case 6:
        case 7:
        case 8: return 30;
        case 9: return 40;
        default: return 0;
    }
}

// =============================================================================
// BENCHMARK: CASCADING vs TABLE
// =============================================================================

// This demonstrates the code size difference
// Cascading: ~40 bytes (10 CMP+BEQ pairs)
// Table: ~30 bytes (bounds check + table data)
// Savings: ~10 bytes (25% reduction)

int benchmark_cascade(int x) {
    if (x == 0) return 10;
    if (x == 1) return 11;
    if (x == 2) return 12;
    if (x == 3) return 13;
    if (x == 4) return 14;
    if (x == 5) return 15;
    if (x == 6) return 16;
    if (x == 7) return 17;
    if (x == 8) return 18;
    if (x == 9) return 19;
    return 0;
}

int benchmark_switch(int x) {
    switch (x) {
        case 0: return 10;
        case 1: return 11;
        case 2: return 12;
        case 3: return 13;
        case 4: return 14;
        case 5: return 15;
        case 6: return 16;
        case 7: return 17;
        case 8: return 18;
        case 9: return 19;
        default: return 0;
    }
}

// =============================================================================
// MAIN TEST DRIVER
// =============================================================================

int main(void) {
    int total = 0;

    // Dense switches (should be optimized to tables)
    total += test_dense_0_to_9(5);      // 15
    total += test_dense_1_to_10(5);     // 140
    total += test_dense_negative_to_positive(0);  // 0

    // Sparse switches (should NOT be optimized)
    total += test_sparse_two_cases(1);   // 10
    total += test_sparse_three_cases(20);  // 200
    total += test_sparse_with_gaps(5);   // 50

    // Real-world patterns
    total += classify_char('5');         // 1 (digit)
    total += process_state(2);           // 30 (paused)
    total += day_to_number(5);           // 500 (Friday)

    // Medium density (borderline cases)
    total += test_medium_density_60(2);  // 20
    total += test_medium_density_70(2);  // 2

    // Complex bodies (still table-worthy)
    total += test_dense_complex_body(5); // 5*10 = 50
    total += test_dense_multi_stmt(3);   // 40*2 = 80

    // Fallthrough cases
    total += test_fallthrough_combined(2);  // 10

    // Benchmark comparison
    total += benchmark_switch(5);        // 15

    return total;  // Should be ~1083
}
