// Test: Explicit cast expressions
// Validates parsing and code generation for C-style casts.

int main() {
    // Cast int to char (narrowing)
    int big = 0x1234;
    char low = (char)big;
    if (low != 0x34) return 1;

    // Cast char to int (widening)
    char c = 200;
    int wide = (int)c;
    if (wide != 200) return 2;

    // Cast literal directly
    int x = (char)0x1FF;  // 0xFF = 255
    if (x != 255) return 3;

    // Cast in expression context
    char a = 100;
    char b = 150;
    int sum = (int)a + (int)b;
    if (sum != 250) return 4;

    // Cast result used in comparison
    int val = 0x0102;
    if ((char)val != 2) return 5;

    // Nested cast (cast of cast)
    int n = 0xABCD;
    char n_lo = (char)(int)n;
    if (n_lo != 0xCD) return 6;

    // Cast does not affect original variable
    int orig = 0x5678;
    char tmp = (char)orig;
    if (orig != 0x5678) return 7;
    if (tmp != 0x78) return 8;

    // Cast pointer to int
    char arr = 42;
    char *p = &arr;
    int addr = (int)p;
    if (addr == 0) return 9;  // address should be nonzero

    // Cast in arithmetic with mixed types
    char c1 = 10;
    int r = (int)c1 * 30;
    if (r != 300) return 10;

    return 0;
}
