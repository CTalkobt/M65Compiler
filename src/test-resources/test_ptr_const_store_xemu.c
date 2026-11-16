/* test_ptr_const_store_xemu.c — #581: a store through a pointer goes to a
 * constant address only when the pointer still holds it. cc45 took a pointer
 * for the constant it was once given (or a same-named pointer of an earlier
 * function was given, or the amount it was stepped by): *p ^= K stored K at
 * that address, and a pointer stepped, assigned in a branch or through its
 * address was still written at the old one.
 *
 * Results at $C0C0: 1 = pass; [10] $AA.
 */
unsigned char band[400];
unsigned char buf[16];
unsigned char * volatile where;

/* an earlier function steps a pointer named d by 80 ($50) */
void fill(unsigned char col) {
    unsigned char r, *d = band + col;
    for (r = 0; r < 4; r++) { d[0] = r; d += 80; }
}
/* [0] [1] [2]: a parameter of that name */
void inv(unsigned char *d) { *d ^= 0xFF; }
void low(unsigned char *d) { *d ^= 0x0F; }
void set(unsigned char *d) { *d |= 0xF0; }

/* [3] a pointer stepped */
void stepped(void) { unsigned char *p = buf; p = (unsigned char *)where; p++; *p = 0x33; }
/* [4] assigned in a branch not taken */
void branch(unsigned char c) {
    unsigned char *p = buf + 4;
    if (c) p = (unsigned char *)0xA0D0;
    *p = 0x44;
}
/* [5] assigned in a loop: the second store goes to buf */
void looped(unsigned char n) {
    unsigned char *p = (unsigned char *)0xA0D1;
    while (n--) { *p = 0x55; p = buf + 5; }
}
/* [6] a compound assignment through a pointer at a constant address */
void compound(void) { unsigned char *p = (unsigned char *)0xA0D2; *p ^= 0x0F; *p |= 0x80; }
/* [7] changed through its address */
void aliased(void) {
    unsigned char *p = (unsigned char *)0xA0D3, **q = &p;
    *q = buf + 7;
    *p = 0x77;
}
/* [8] stepped by a compound assignment */
void added(void) { unsigned char *p = (unsigned char *)0xA0D4; p += 2; *p = 0x88; }
/* [9] the constant address still used: a const pointer, and one never changed */
void direct(void) {
    unsigned char *const p = (unsigned char *)0xA0D8;
    unsigned char *q = (unsigned char *)0xA0D9;
    *p = 0x90; *p |= 0x09; *q = 0x9A;
}

int main(void) {
    unsigned char *r = (unsigned char *)0xC0C0;
    unsigned char *z = (unsigned char *)0x0050;
    unsigned char z50 = *z;
    fill(3);
    buf[0] = 0x5A; buf[1] = 0x5A; buf[2] = 0x0A;
    inv(buf); low(buf + 1); set(buf + 2);
    r[0] = buf[0] == 0xA5 && *z == z50;
    r[1] = buf[1] == 0x55;
    r[2] = buf[2] == 0xFA;
    where = buf + 2; stepped();
    r[3] = buf[3] == 0x33;
    *(unsigned char *)0xA0D0 = 0; branch(0);
    r[4] = buf[4] == 0x44 && *(unsigned char *)0xA0D0 == 0;
    looped(2);
    r[5] = buf[5] == 0x55 && *(unsigned char *)0xA0D1 == 0x55;
    *(unsigned char *)0xA0D2 = 0x35; compound();
    r[6] = *(unsigned char *)0xA0D2 == 0xBA;
    *(unsigned char *)0xA0D3 = 0; aliased();
    r[7] = buf[7] == 0x77 && *(unsigned char *)0xA0D3 == 0;
    *(unsigned char *)0xA0D4 = 0; added();
    r[8] = *(unsigned char *)0xA0D6 == 0x88 && *(unsigned char *)0xA0D4 == 0;
    direct();
    r[9] = *(unsigned char *)0xA0D8 == 0x99 && *(unsigned char *)0xA0D9 == 0x9A;
    r[10] = 0xAA;
    return 0;
}
