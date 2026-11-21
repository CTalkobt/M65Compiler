/* #676: (unsigned long)x << k, x an int (or a byte) loaded through a pointer
 * index: the byte-move shift (#550) staged the value at the destination's
 * low byte and copied it upward lowest first, losing the high byte.
 * Each result at $C0C0: 01 = pass; $AA after the last. */
#define R ((volatile unsigned char *)0xC0C0)

static const unsigned int tab[4] = { 0x1234, 0x0100, 0xFEDC, 0x0300 };
static const unsigned char btab[4] = { 0x12, 0x81, 0xFF, 0x7F };

/* the issue's shape: the index a pointer load */
unsigned long f64(const unsigned char *s) { return (unsigned long)tab[s[3]] * 64; }
unsigned long f6(const unsigned char *s) { return (unsigned long)tab[s[1]] << 6; }
unsigned long f1(const unsigned char *s) { return (unsigned long)tab[s[2]] << 1; }
unsigned long f3(const unsigned char *s) { return (unsigned long)tab[s[2]] << 3; }
unsigned long f8(const unsigned char *s) { return (unsigned long)tab[s[2]] << 8; }
unsigned long f12(const unsigned char *s) { return (unsigned long)tab[s[2]] << 12; }
unsigned long f15(const unsigned char *s) { return (unsigned long)tab[s[2]] << 15; }
unsigned long b6(const unsigned char *s) { return (unsigned long)btab[s[0]] << 6; }
unsigned long b13(const unsigned char *s) { return (unsigned long)btab[s[2]] << 13; }
/* the index a parameter (staged above: already right) */
unsigned long p6(unsigned char i) { return (unsigned long)tab[i] << 6; }
/* the workaround's form, the result added to a base */
unsigned long base = 0x40000UL;
unsigned long img(const unsigned char *s) { return base + (unsigned long)tab[s[3]] * 64; }

int main(void) {
    static const unsigned char idx[4] = { 2, 1, 2, 0 };
    unsigned char n = 0;
    R[n++] = f64(idx) == 0x048D00UL;             /* tab[0] = $1234 */
    R[n++] = f6(idx) == 0x4000UL;                /* tab[1] = $0100 */
    R[n++] = f1(idx) == 0x1FDB8UL;               /* tab[2] = $FEDC */
    R[n++] = f3(idx) == 0x7F6E0UL;
    R[n++] = f8(idx) == 0xFEDC00UL;
    R[n++] = f12(idx) == 0xFEDC000UL;
    R[n++] = f15(idx) == 0x7F6E0000UL;
    R[n++] = b6(idx) == 0x03FC0UL;              /* btab[2] = $FF */
    R[n++] = b13(idx) == 0x1FE000UL;
    R[n++] = p6(0) == 0x048D00UL && p6(2) == 0x3FB700UL;
    R[n++] = img(idx) == 0x088D00UL;
    R[n] = 0xAA;
    return 0;
}
