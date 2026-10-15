/* test_cmpvalue_xemu.c — comparisons used as values (0 or 1) are built from
 * the flags without branches (C for < and >=, Z for == and !=); each
 * against the same comparison deciding a branch
 *
 * Results at $C0C0 (01 = pass):
 *   [0] signed int  ==, !=, <, >=, <=, > (variables and constants)
 *   [1] unsigned int
 *   [2] signed and unsigned char
 *   [3] signed and unsigned long
 *   [4] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

static volatile unsigned char res[6];
static const signed int sv[] = { 0, 1, -1, 2, -2, 127, -128, 255, 256, -256, 32767, -32768, 1000 };
static const signed long lv[] = { 0, 1, -1, 65535L, 65536L, -65536L, 2147483647L, -2147483647L - 1, 100000L };

#define BR(c) ((c) ? 1 : 0)
static unsigned char branchy(unsigned char op, unsigned char r) {
    unsigned char b;
    if (r) b = 1; else b = 0;
    (void)op;
    return b;
}

int main(void) {
    unsigned char i, j, ok;
    ok = 1;
    for (i = 0; i < 13; i++)
        for (j = 0; j < 13; j++) {
            signed int a = sv[i], b = sv[j];
            res[0] = a == b; res[1] = a != b; res[2] = a < b;
            res[3] = a >= b; res[4] = a <= b; res[5] = a > b;
            if (res[0] != branchy(0, BR(a == b)) || res[1] != branchy(1, BR(a != b)) ||
                res[2] != branchy(2, BR(a < b)) || res[3] != branchy(3, BR(a >= b)) ||
                res[4] != branchy(4, BR(a <= b)) || res[5] != branchy(5, BR(a > b))) ok = 0;
            res[0] = a == 0; res[1] = a != 0; res[2] = a < 0; res[3] = a >= 0; res[4] = a < 100; res[5] = a >= -5;
            if (res[0] != BR(a == 0) || res[1] != BR(a != 0) || res[2] != BR(a < 0) || res[3] != BR(a >= 0) ||
                res[4] != BR(a < 100) || res[5] != BR(a >= -5)) ok = 0;
        }
    OUT[0] = ok;
    ok = 1;
    for (i = 0; i < 13; i++)
        for (j = 0; j < 13; j++) {
            unsigned int a = (unsigned int)sv[i], b = (unsigned int)sv[j];
            res[0] = a == b; res[1] = a != b; res[2] = a < b; res[3] = a >= b; res[4] = a <= b; res[5] = a > b;
            if (res[0] != BR(a == b) || res[1] != BR(a != b) || res[2] != BR(a < b) || res[3] != BR(a >= b) ||
                res[4] != BR(a <= b) || res[5] != BR(a > b)) ok = 0;
            res[2] = a < 300; res[3] = a >= 300;
            if (res[2] != BR(a < 300) || res[3] != BR(a >= 300)) ok = 0;
        }
    OUT[1] = ok;
    ok = 1;
    for (i = 0; i < 13; i++)
        for (j = 0; j < 13; j++) {
            signed char a = (signed char)sv[i], b = (signed char)sv[j];
            unsigned char c = (unsigned char)sv[i], d = (unsigned char)sv[j];
            res[0] = a == b; res[1] = a != b; res[2] = a < b; res[3] = a >= b;
            if (res[0] != BR(a == b) || res[1] != BR(a != b) || res[2] != BR(a < b) || res[3] != BR(a >= b)) ok = 0;
            res[0] = c == d; res[1] = c != d; res[2] = c < d; res[3] = c >= d; res[4] = c == 7; res[5] = c < 7;
            if (res[0] != BR(c == d) || res[1] != BR(c != d) || res[2] != BR(c < d) || res[3] != BR(c >= d) ||
                res[4] != BR(c == 7) || res[5] != BR(c < 7)) ok = 0;
        }
    OUT[2] = ok;
    ok = 1;
    for (i = 0; i < 9; i++)
        for (j = 0; j < 9; j++) {
            signed long a = lv[i], b = lv[j];
            unsigned long c = (unsigned long)lv[i], d = (unsigned long)lv[j];
            res[0] = a == b; res[1] = a != b; res[2] = a < b; res[3] = a >= b;
            if (res[0] != BR(a == b) || res[1] != BR(a != b) || res[2] != BR(a < b) || res[3] != BR(a >= b)) ok = 0;
            res[2] = c < d; res[3] = c >= d; res[4] = c <= d; res[5] = c > d;
            if (res[2] != BR(c < d) || res[3] != BR(c >= d) || res[4] != BR(c <= d) || res[5] != BR(c > d)) ok = 0;
        }
    OUT[3] = ok;
    OUT[4] = 0xAA;
    return 0;
}
