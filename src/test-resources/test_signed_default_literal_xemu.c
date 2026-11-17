// #612: with signed_default an unsuffixed literal is a signed int, so a ?:
// of a signed operand and a literal (or of two literals) is signed and is
// sign-extended when widened. In the default (unsigned) mode nothing changes:
// c ? 0 : -1 is unsigned there.
// Results at $C0C0: 01 per check, $AA sentinel.

volatile int m1 = -1;
volatile signed char c1 = -1;
volatile int z = 0;
volatile unsigned big = 40000;

#pragma cc45 signed_default

static unsigned char sd_tern_lits(void)   { long a = z ? 0 : -1; return a == -1L; }
static unsigned char sd_tern_int(void)    { int s = m1; long a = z ? 0 : s; return a == -1L; }
static unsigned char sd_tern_char(void)   { signed char c = c1; long a = z ? 0 : c; return a == -1L; }
static unsigned char sd_tern_cmp(void)    { int s = m1; return (z ? 0 : s) < 0; }
static unsigned char sd_tern_neg(void)    { long a = -(z ? 1 : 2); return a == -2L; }
static unsigned char sd_tern_sub(void)    { long a = (z ? 2 : 1) - 2; return a == -1L; }
static unsigned char sd_tern_folded(void) { int s = m1; long a = z ? (1 - 1) : s; return a == -1L; }
// what must stay as it was
static unsigned char sd_unsigned(void)    { unsigned u = big; return u > 5 && u + 1 == 40001U && (z ? 0 : u) > 32767; }
static unsigned char sd_arith(void)       { int s = m1; long a = s + 0; long b = s >> 1; return a == -1L && b == -1L && s < 1; }
static unsigned char sd_u_suffix(void)    { int s = m1; long a = z ? 0U : s; return a == 65535L; }
static unsigned char sd_long_lit(void)    { long l = z ? 100000 : -1; return l == -1L; }

#pragma cc45 unsigned_default

// the default: the literal is unsigned, and so is the result
static unsigned char ud_tern(void)        { long a = z ? (unsigned)0 : -1; return a == 65535L; }
static unsigned char ud_signed(void)      { signed int s = m1; long a = z ? (signed)0 : s; return a == -1L; }

int main(void) {
    unsigned char *o = (unsigned char *)0xC0C0;
    o[0] = sd_tern_lits();
    o[1] = sd_tern_int();
    o[2] = sd_tern_char();
    o[3] = sd_tern_cmp();
    o[4] = sd_tern_neg();
    o[5] = sd_tern_sub();
    o[6] = sd_tern_folded();
    o[7] = sd_unsigned();
    o[8] = sd_arith();
    o[9] = sd_u_suffix();
    o[10] = sd_long_lit();
    o[11] = ud_tern();
    o[12] = ud_signed();
    o[13] = 0xAA;
    return 0;
}
