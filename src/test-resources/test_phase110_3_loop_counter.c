// Test Phase 110.3: Register-Resident Loop Optimization
// Tests optimization of 8-bit loop counters allocated to X register

// =============================================================================
// BASIC 8-BIT LOOPS (X-REGISTER CANDIDATES)
// =============================================================================

// Simple ascending loop: unsigned char
unsigned char test_loop_char_ascending(void) {
    unsigned char sum = 0;
    for (unsigned char i = 0; i < 10; i++) {
        sum += i;
    }
    return sum;  // sum should be 45
}

// Simple descending loop: unsigned char
unsigned char test_loop_char_descending(void) {
    unsigned char sum = 0;
    for (unsigned char i = 10; i > 0; i--) {
        sum += i;
    }
    return sum;  // sum should be 55
}

// Full 8-bit range: 0-255
unsigned char test_loop_char_full_range(void) {
    unsigned char count = 0;
    for (unsigned char i = 0; i < 255; i++) {
        count++;
    }
    return count;  // count should be 255
}

// Loop with small bound
unsigned char test_loop_char_small(void) {
    unsigned char result = 0;
    for (unsigned char i = 0; i < 5; i++) {
        result = i;
    }
    return result;  // result should be 4
}

// =============================================================================
// LOOP VARIANTS (8-bit, OPTIMIZABLE)
// =============================================================================

// <= comparison instead of <
unsigned char test_loop_char_lte(void) {
    unsigned char sum = 0;
    for (unsigned char i = 0; i <= 10; i++) {
        sum += i;
    }
    return sum;  // sum should be 55
}

// Loop with explicit initial value
unsigned char test_loop_char_init(void) {
    unsigned char result = 0;
    for (unsigned char i = 5; i < 10; i++) {
        result += i;
    }
    return result;  // sum of 5,6,7,8,9 = 35
}

// Nested loops with 8-bit counters
unsigned char test_loop_char_nested(void) {
    unsigned char sum = 0;
    for (unsigned char i = 0; i < 5; i++) {
        for (unsigned char j = 0; j < 4; j++) {
            sum++;
        }
    }
    return sum;  // 5 * 4 = 20
}

// =============================================================================
// LOOP PATTERNS (VARIOUS BOUNDS)
// =============================================================================

// Loop with power-of-2 bound (common optimization case)
unsigned char test_loop_power_of_2(void) {
    unsigned char sum = 0;
    for (unsigned char i = 0; i < 16; i++) {
        sum += i;
    }
    return sum;  // 0+1+2+...+15 = 120
}

// Loop with power-of-2 bound (256)
unsigned char test_loop_256(void) {
    unsigned char count = 0;
    for (unsigned char i = 0; i < 256; i++) {
        count++;
    }
    return count;  // count should wrap to 0 or be 255 depending on loop
}

// Array indexed by 8-bit loop counter
unsigned char test_loop_array_index(void) {
    unsigned char arr[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    unsigned char sum = 0;
    for (unsigned char i = 0; i < 10; i++) {
        sum += arr[i];
    }
    return sum;  // 0+1+2+...+9 = 45
}

// Loop with multiplication in body
unsigned char test_loop_multiply(void) {
    unsigned char result = 0;
    for (unsigned char i = 1; i < 10; i++) {
        result += i * i;
    }
    return result;  // 1+4+9+16+25+36+49+64+81 = 285 (mod 256)
}

// =============================================================================
// 16-BIT LOOPS (NOT X-REGISTER CANDIDATES)
// =============================================================================

// unsigned int loop: too large for X register
unsigned int test_loop_int_large(void) {
    unsigned int sum = 0;
    for (unsigned int i = 0; i < 1000; i++) {
        sum += i;
    }
    return sum;  // sum of 0..999
}

// signed int loop: not 8-bit
int test_loop_int_signed(void) {
    int sum = 0;
    for (int i = -10; i < 10; i++) {
        sum += i;
    }
    return sum;  // should be 0 (symmetric range)
}

// =============================================================================
// LOOP CONSTRAINTS (NON-OPTIMIZABLE CASES)
// =============================================================================

// Loop counter modified in body: should NOT optimize
unsigned char test_loop_modified(void) {
    unsigned char sum = 0;
    for (unsigned char i = 0; i < 10; i++) {
        i += 2;  // Counter modified!
        sum += i;
    }
    return sum;
}

// Nested loop using same variable: should NOT optimize
unsigned char test_loop_nested_same_var(void) {
    unsigned char sum = 0;
    for (unsigned char i = 0; i < 5; i++) {
        for (unsigned char i = 0; i < 3; i++) {  // Reuses i!
            sum++;
        }
    }
    return sum;
}

// Function call in loop body (might clobber X): be cautious
unsigned char external_func(void);

unsigned char test_loop_with_call(void) {
    unsigned char result = 0;
    for (unsigned char i = 0; i < 5; i++) {
        result += external_func();  // Function call
    }
    return result;
}

// =============================================================================
// REAL-WORLD PATTERNS
// =============================================================================

// String processing
unsigned char count_chars(const char* str, unsigned char max_len) {
    unsigned char count = 0;
    for (unsigned char i = 0; i < max_len && str[i]; i++) {
        count++;
    }
    return count;
}

// Bitmap operations (8-bit flag testing)
unsigned char count_set_bits(unsigned char bitmap) {
    unsigned char count = 0;
    for (unsigned char i = 0; i < 8; i++) {
        if (bitmap & (1 << i)) {
            count++;
        }
    }
    return count;
}

// Buffer operations
void fill_buffer(unsigned char* buffer, unsigned char size, unsigned char value) {
    for (unsigned char i = 0; i < size; i++) {
        buffer[i] = value;
    }
}

// Lookup table iteration
unsigned char lookup_index(unsigned char key, const unsigned char* table, unsigned char table_size) {
    for (unsigned char i = 0; i < table_size; i++) {
        if (table[i] == key) {
            return i;
        }
    }
    return 255;  // Not found
}

// Game loop pattern (pixel iteration)
unsigned char process_pixels(unsigned char width, unsigned char height) {
    unsigned char total = 0;
    for (unsigned char y = 0; y < height; y++) {
        for (unsigned char x = 0; x < width; x++) {
            total++;
        }
    }
    return total;  // width * height
}

// =============================================================================
// LOOP METRICS
// =============================================================================

// Very tight loop (best case for optimization)
unsigned char test_tight_loop(void) {
    unsigned char i;
    unsigned char sum = 0;
    for (i = 0; i < 8; i++) {
        sum++;
    }
    return sum;
}

// Loop with complex body (savings might be less obvious)
unsigned char test_complex_loop(void) {
    unsigned char sum = 0;
    for (unsigned char i = 0; i < 4; i++) {
        sum += (i * i) + (i << 1) + 1;
    }
    return sum;
}

// =============================================================================
// MAIN TEST DRIVER
// =============================================================================

int main(void) {
    unsigned char total = 0;

    // Basic 8-bit loops
    total += test_loop_char_ascending();    // 45
    total += test_loop_char_descending();   // 55
    total += test_loop_char_small();        // 4
    total += test_loop_char_lte();          // 55
    total += test_loop_char_init();         // 35

    // Nested loops
    total += test_loop_char_nested();       // 20

    // Various bounds
    total += test_loop_power_of_2();        // 120
    total += test_loop_array_index();       // 45

    // Real-world patterns
    total += count_set_bits(0x0F);          // 4
    unsigned char arr[3] = {10, 20, 30};
    total += lookup_index(20, arr, 3);      // 1
    total += process_pixels(5, 4);          // 20

    // Tight loop
    total += test_tight_loop();             // 8

    return (int)total;  // Should be ~411 (or mod 256)
}
