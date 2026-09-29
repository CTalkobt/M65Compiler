// Test: Cast Fold Type Preservation with Uncalled Functions
// This tests the issue where standalone functions returning casted constants
// don't generate proper multi-byte return values with -O1+

// Function that returns a casted constant (uncalled in this module)
long get_long_42(void) {
    return (long)42;
}

// Another variant
long get_long_negative(void) {
    return (long)-1;
}

// Function that returns a larger value
long get_long_large(void) {
    return (long)0x12345678;
}

// Short that returns casted constant (for comparison)
short get_short_42(void) {
    return (short)42;
}

// Int that returns casted constant
int get_int_42(void) {
    return (int)42;
}

// This function IS called, for comparison
int called_function(void) {
    return 42;
}

int main(void) {
    // Only call the int function - long functions are uncalled
    int result = called_function();
    return result;
}
