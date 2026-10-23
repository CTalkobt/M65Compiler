/* test_member_decls_xemu.c — struct members declared several to a line
 * (#333: const unsigned char *a, *b; made b a pointer to pointers: the type
 * specifier had taken a's '*', and b's added one more)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] const unsigned char *a, *b: both index bytes
 *   [1] unsigned char *c, *d, e: two pointers and a char
 *   [2] **pp, *q; a typedef'd pointer type: t1, *t2
 *   [3] * const k1, *k2; and the same at file scope and in a function
 *   [4] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
typedef const unsigned char *bytes_t;
typedef struct {
    const unsigned char *a, *b;            /* the issue's form */
    unsigned char *c, *d, e;               /* pointer, pointer, char */
    const unsigned char **pp, *q;          /* pointer to pointer, pointer */
    bytes_t t1, *t2;                       /* typedef pointer: pointer, pointer to pointer */
    unsigned char * const k1, *k2;         /* const pointer, pointer */
} s_t;
static const unsigned char da[4] = {1, 2, 3, 4}, db[4] = {5, 6, 7, 8};
static unsigned char dc[4] = {9, 10, 11, 12};
static const unsigned char *dp[2] = {da, db};
static bytes_t tb[2] = {db, da};
static const s_t s1 = {da, db, dc, dc + 1, 7, dp, db + 2, da, tb, dc, dc + 2};
static const s_t *sp = &s1;
const unsigned char *ga, *gb;              /* file scope too */
int main(void) {
    volatile unsigned char i = 1;
    const unsigned char *la, *lb;          /* and locals */
    la = da; lb = db; ga = da; gb = db;
    OUT[0] = sp->a[i] == 2 && sp->b[i] == 6 && sizeof(sp->b[0]) == 1;
    OUT[1] = sp->c[i] == 10 && sp->d[i] == 11 && sp->e == 7 && sizeof(sp->e) == 1;
    OUT[2] = sp->pp[i][i] == 6 && sp->q[i] == 8 && sp->t1[i] == 2 && sp->t2[i][i] == 2;
    OUT[3] = sp->k1[i] == 10 && sp->k2[i] == 12 && la[i] == 2 && lb[i] == 6 && ga[i] == 2 && gb[i] == 6;
    OUT[4] = 0xAA;
    return 0;
}
