// cl45 fixture: mega65-wide-arith. long arithmetic on operands widened from
// int or char whose result is narrowed again: 16 bits give the same value.
unsigned int a, b, r;
unsigned char c;
unsigned long big, wide;

void narrowed(void) {
    r = (unsigned int)((long)a + (long)b);                      // EXPECT mega65-wide-arith
    r = (unsigned int)(a * 10L);                                // EXPECT mega65-wide-arith
    c = (unsigned char)((unsigned long)a ^ c);                  // EXPECT mega65-wide-arith
}

void needed(void) {
    wide = (long)a + (long)b;           // the result is a long
    wide = a * 1000L;
    r = (unsigned int)(big + a);        // an operand is a long
    r = (unsigned int)((long)a / 7);    // (a division's low bits need the whole value)
    r = a + b;
}
