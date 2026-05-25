// Test integer literal auto-promotion to long when > 65535

long get_big(void) {
    long x = 100000;  // exceeds 16-bit, auto-promoted to long
    return x;
}

long add_big(void) {
    long a = 50000;
    long b = 70000;  // auto-promoted
    return a + b;     // 120000
}

int main() {
    // 100000 = $186A0
    long v = get_big();
    if (v != 100000) return 1;

    // 50000 + 70000 = 120000
    long s = add_big();
    if (s != 120000) return 2;

    return 0;
}
