// cl45 fixture: mega65-implicit-narrowing. A wider integer stored in a
// narrower one with no cast.
unsigned int n;
unsigned long big;
unsigned char c, d;
unsigned char buf[8];
struct rec { unsigned char lo; unsigned int word; } r;
unsigned int get(void);

void narrowed(void) {
    unsigned char local = n;            // EXPECT mega65-implicit-narrowing
    c = n;                              // EXPECT mega65-implicit-narrowing
    c = n + 1;                          // EXPECT mega65-implicit-narrowing
    n = big;                            // EXPECT mega65-implicit-narrowing
    buf[1] = n >> 8;                    // EXPECT mega65-implicit-narrowing
    r.lo = get();                       // EXPECT mega65-implicit-narrowing
    n = 0x12345;                        // EXPECT mega65-implicit-narrowing
    c = c ? n : d;                      // EXPECT mega65-implicit-narrowing
    d = local;
}

void fine(void) {
    unsigned char local = (unsigned char)n;
    c = (unsigned char)n;
    c = d + 1;
    c = n & 0xFF;
    c = d ? c : 7;
    c = n < 10;
    c = !n;
    n = c;
    big = n;
    n = (unsigned int)big;
    r.word = get();
    buf[1] = (unsigned char)(n >> 8);
    d = local;
}
