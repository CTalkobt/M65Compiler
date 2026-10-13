/* test_vector_xemu.c — GCC vector extension (vector_size), compound
 * literal initializers
 *
 * Results at $C0C0 (01 = pass):
 *   [0] + - * / % & | ^ on vectors (unsigned char x4, short x8)
 *   [1] scalar operands (v / 2, 3 * v), unary -, ~, compound assignment
 *   [2] comparisons give -1/0 masks; v[i] read and write
 *   [3] vectors as parameters and return values, through pointers, in
 *       unions; casts between vector types of the same size
 *   [4] __builtin_shuffle with one and two vectors
 *   [5] mode(QI/HI/SI) integers; vector of long
 *   [6] arrays of structs/vectors initialized with (T){...}
 *   [7] $AA sentinel
 * (plain short/long are unsigned in cc45: the signed vectors say so)
 */
#define OUT ((volatile unsigned char *)0xC0C0)

typedef unsigned char v4qi __attribute__((vector_size(4)));
typedef signed short v8hi __attribute__((vector_size(16)));
typedef unsigned short v8uhi __attribute__((vector_size(16)));
typedef signed long v2si __attribute__((vector_size(8)));
typedef int __attribute__((mode(SI))) si_t;
typedef int __attribute__((mode(QI))) qi_t;

static v4qi addq(v4qi a, v4qi b) { return a + b; }
static void mul8(v8hi *r, v8hi *a, v8hi *b) { *r = *a * *b; }

struct P { signed short a[2]; };
struct P parr[] = { ((struct P){ { 1, 2 } }), ((struct P){ { 3, 4 } }) };
v8uhi uarr[] = { ((v8uhi){ 10, 20, 30, 40, 50, 60, 70, 80 }), ((v8uhi){ 1, 2, 3, 4, 5, 6, 7, 8 }) };

int main(void) {
    int ok, i;
    {
        v4qi a = { 10, 20, 30, 40 }, b = { 1, 2, 3, 4 }, c;
        v8hi x = { 100, -200, 300, -400, 5, 6, 7, 8 }, y = { 3, 4, 5, 6, 7, 8, 9, 10 }, z;
        c = addq(a, b);
        ok = c[0] == 11 && c[3] == 44;
        c = a - b; ok = ok && c[1] == 18;
        c = a / b; ok = ok && c[2] == 10;
        c = a % b; ok = ok && c[3] == 0 && (a % (v4qi){ 3, 3, 3, 3 })[0] == 1;
        mul8(&z, &x, &y); ok = ok && z[1] == -800 && z[7] == 80;
        z = x & y; ok = ok && z[0] == (100 & 3);
        z = x | y; ok = ok && z[2] == (300 | 5);
        z = x ^ y; ok = ok && z[3] == (-400 ^ 6);
        OUT[0] = ok;
    }
    {
        v8hi x = { 100, -200, 300, -400, 5, 6, 7, 8 }, z;
        z = x / 2; ok = z[1] == -100 && z[4] == 2;
        z = 3 * x; ok = ok && z[3] == -1200;
        z = -x; ok = ok && z[0] == -100;
        z = ~x; ok = ok && z[5] == ~6;
        z += x; ok = ok && z[0] == -1;
        x <<= 1; ok = ok && x[2] == 600;
        OUT[1] = ok;
    }
    {
        v8hi x = { 1, 5, 3, 7, 0, 0, 0, 0 }, y = { 2, 5, 1, 9, 0, 1, 0, 0 }, m;
        m = x < y; ok = m[0] == -1 && m[1] == 0 && m[2] == 0 && m[3] == -1 && m[5] == -1;
        m = x == y; ok = ok && m[1] == -1 && m[0] == 0;
        x[6] = 42; ok = ok && x[6] == 42;
        i = 3; ok = ok && y[i] == 9;
        OUT[2] = ok;
    }
    {
        union { v8hi v; short s[8]; } u;
        v8hi x = { 1, 2, 3, 4, 5, 6, 7, 8 };
        v8uhi ux;
        u.v = x;
        ux = (v8uhi)x;
        ok = u.s[7] == 8 && ux[0] == 1 && sizeof(v8hi) == 16 && sizeof(v4qi) == 4;
        OUT[3] = ok;
    }
    {
        v8hi a = { 10, 11, 12, 13, 14, 15, 16, 17 }, b = { 20, 21, 22, 23, 24, 25, 26, 27 }, r;
        v8hi m1 = { 7, 6, 5, 4, 3, 2, 1, 0 }, m2 = { 0, 8, 1, 9, 15, 2, 10, 3 };
        r = __builtin_shuffle(a, m1); ok = r[0] == 17 && r[7] == 10;
        r = __builtin_shuffle(a, b, m2); ok = ok && r[1] == 20 && r[3] == 21 && r[4] == 27 && r[5] == 12;
        OUT[4] = ok;
    }
    {
        v2si l = { 100000L, -5L }, k;
        k = l * 3;
        OUT[5] = sizeof(si_t) == 4 && sizeof(qi_t) == 1 && k[0] == 300000L && k[1] == -15L;
    }
    OUT[6] = parr[0].a[1] == 2 && parr[1].a[0] == 3 && uarr[0][7] == 80 && uarr[1][2] == 3;
    OUT[7] = 0xAA;
    return 0;
}
