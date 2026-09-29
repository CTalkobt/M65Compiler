// Test Phase 110.8: Zero-Page Pressure Relief
// Tests identification and relief of zero-page memory pressure

// =============================================================================
// HOT VARIABLES (MUST STAY IN ZP FOR PERFORMANCE)
// =============================================================================

// Loop counter - should stay in zero-page
int __zp loop_counter_hot(int iterations) {
    int result = 0;
    int i;

    // Hot variable: accessed in loop
    for (i = 0; i < iterations; i++) {
        result += i;
    }

    return result;
}

// Frame pointer - must stay in zero-page (system critical)
int test_frame_pointer(int x, int y) {
    int a = x;
    int b = y;
    int c = a + b;
    return c;
}

// Accumulator temp - short-lived, high frequency
int test_accumulator_hot(void) {
    int sum = 0;
    int i;

    for (i = 0; i < 256; i++) {
        sum += i;
    }

    return sum;
}

// =============================================================================
// COLD VARIABLES (CANDIDATES FOR MIGRATION)
// =============================================================================

// Seldom-accessed variable (good candidate for migration)
int __zp rarely_used_var = 42;

int test_cold_variable(void) {
    // Accessed only once in entire function
    int result = rarely_used_var * 2;

    // Rest of function doesn't touch it
    int a = 10;
    int b = 20;
    int c = 30;

    return result + a + b + c;
}

// Configuration variable (read-only, low frequency)
int __zp config_flags = 0x55;

int test_config_access(void) {
    // Check config once, then do work
    if (config_flags & 0x01) {
        return 100;
    }
    return 200;
}

// Error counter (incremented occasionally)
int __zp error_count = 0;

int test_error_counter(void) {
    // Infrequent updates
    error_count++;

    // Lots of other work
    int result = 0;
    int i;
    for (i = 0; i < 100; i++) {
        result += i;
    }

    return result;
}

// =============================================================================
// TEMPORARY VARIABLES (SHORT-LIVED, REMOVABLE)
// =============================================================================

// Temp variable only needed for computation
int test_temp_variable(int x) {
    int __zp temp = x * 2;  // Temporary
    return temp + 10;
}

// Multiple temps in sequence
int test_multiple_temps(int a, int b, int c) {
    int __zp temp1 = a + b;  // Temp 1
    int __zp temp2 = temp1 * c;  // Temp 2 (depends on temp1)
    return temp2;
}

// =============================================================================
// ZERO-PAGE PRESSURE SCENARIOS
// =============================================================================

// Simulated high ZP usage (all these variables compete for space)
// Each occupies 1-2 bytes of zero-page
int test_zp_pressure_high(int x) {
    int __zp var1 = x;
    int __zp var2 = x + 1;
    int __zp var3 = x + 2;
    int __zp var4 = x + 3;
    int __zp var5 = x + 4;
    int __zp var6 = x + 5;
    int __zp var7 = x + 6;
    int __zp var8 = x + 7;
    int __zp var9 = x + 8;
    int __zp var10 = x + 9;

    // All 10 variables in same scope = 20 bytes of zero-page pressure!
    return var1 + var2 + var3 + var4 + var5 +
           var6 + var7 + var8 + var9 + var10;
}

// Candidate for relief: migrate cold vars to main memory
int test_zp_pressure_mixed(int x) {
    int __zp hot_loop_counter;
    int __zp cold_config_var = 0x42;  // Accessed once
    int __zp hot_accumulator = 0;
    int __zp cold_error_flag = 0;     // Accessed once
    int __zp hot_index;

    // Hot variables used in loop
    for (hot_loop_counter = 0; hot_loop_counter < 10; hot_loop_counter++) {
        hot_accumulator += hot_loop_counter;
        hot_index = hot_loop_counter * 2;
    }

    // Cold variables used rarely
    if (cold_config_var & 0x40) {
        cold_error_flag = 1;
    }

    return hot_accumulator + cold_error_flag;
}

// =============================================================================
// RELIEF SCENARIO: TRADE-OFFS
// =============================================================================

// Before relief: 8 bytes ZP pressure
// After relief: 4 bytes ZP (cold vars moved), 4 bytes main memory
int test_relief_benefit(void) {
    int __zp loop_cnt;      // Hot - stays ZP (1 byte)
    int __zp sum_acc;       // Hot - stays ZP (2 bytes)
    int __zp status;        // Cold - candidate for migration (1 byte)
    int __zp debug_flag;    // Cold - candidate for migration (1 byte)

    sum_acc = 0;

    for (loop_cnt = 0; loop_cnt < 100; loop_cnt++) {
        sum_acc += loop_cnt;
    }

    // Cold accesses
    status = sum_acc > 1000 ? 1 : 0;
    debug_flag = 0;

    return sum_acc + status + debug_flag;
}

// =============================================================================
// CONTEXT: WHEN RELIEF HELPS
// =============================================================================

// Scenario 1: Function with many local variables
int test_relief_context_locals(int x, int y, int z) {
    int __zp a = x;
    int __zp b = y;
    int __zp c = z;
    int __zp d = x + y;
    int __zp e = y + z;
    int __zp f = x + z;
    int __zp g = a + b + c;  // Only this one is really needed

    return g;  // g used once, others computed but not used
}

// Scenario 2: Multiple functions competing for ZP
int helper1_uses_zp(void) {
    int __zp temp1 = 42;
    int __zp temp2 = 84;
    return temp1 + temp2;
}

int helper2_uses_zp(void) {
    int __zp temp3 = 100;
    int __zp temp4 = 200;
    return temp3 + temp4;
}

int test_relief_context_functions(void) {
    return helper1_uses_zp() + helper2_uses_zp();
}

// Scenario 3: Register allocation pressure
// Compiler needs many registers for computation
int test_relief_context_computation(int x) {
    int __zp r1 = x;
    int __zp r2 = r1 * 2;
    int __zp r3 = r2 + r1;
    int __zp r4 = r3 * 3;
    int __zp r5 = r4 - r2;
    int __zp r6 = r5 / r1;
    int __zp r7 = r6 + r4;

    // Intermediate values: good candidates for relief
    return r7;
}

// =============================================================================
// RELIEF IMPACT MEASUREMENT
// =============================================================================

// Measure: access frequency vs. ZP cost
int test_access_frequency_high(void) {
    int __zp var = 0;
    int i;

    // High frequency: 1000 accesses
    for (i = 0; i < 1000; i++) {
        var += i;
    }

    // Don't migrate - frequency × cost_increase = loss
    return var;
}

int test_access_frequency_low(void) {
    int __zp var = 0;

    // Single access
    var = 42;

    // Good candidate for relief - only accessed once
    return var;
}

int test_access_frequency_medium(void) {
    int __zp var = 0;
    int i;

    // Medium frequency: 10 accesses
    for (i = 0; i < 10; i++) {
        var += i;
    }

    // Borderline - depends on other ZP pressure
    return var;
}

// =============================================================================
// MAIN TEST DRIVER
// =============================================================================

int main(void) {
    int total = 0;

    // Hot variables (must stay)
    total += loop_counter_hot(10);
    total += test_frame_pointer(3, 4);
    total += test_accumulator_hot();

    // Cold variables (candidates for migration)
    total += test_cold_variable();
    total += test_config_access();
    total += test_error_counter();

    // Temporary variables
    total += test_temp_variable(5);
    total += test_multiple_temps(2, 3, 4);

    // Zero-page pressure scenarios
    total += test_zp_pressure_high(1);
    total += test_zp_pressure_mixed(10);

    // Relief benefit measurements
    total += test_relief_benefit();

    // Relief context scenarios
    total += test_relief_context_locals(1, 2, 3);
    total += test_relief_context_functions();
    total += test_relief_context_computation(5);

    // Access frequency measurements
    total += test_access_frequency_high();
    total += test_access_frequency_low();
    total += test_access_frequency_medium();

    return total;  // Valid result
}
