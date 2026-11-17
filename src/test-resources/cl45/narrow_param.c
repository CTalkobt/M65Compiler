// cl45 fixture: mega65-narrow-param. A static function's int parameter that
// every call gives a constant of 0-255.
extern unsigned char grid[40];
extern int n;

static void fill(int from, int count, int value) {              // EXPECT mega65-narrow-param // EXPECT mega65-narrow-param
    int i;
    for (i = 0; i < count; i++) grid[from + i] = (unsigned char)value;
}

static int scale(int v, unsigned char by) {
    return v * by;
}

int main(void) {
    fill(0, n, 7);
    fill(10, n + 1, 9);
    return scale(n, 3) + scale(1000, 2);
}
