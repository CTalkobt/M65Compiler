/* test_cast_ptrptr_xemu.c — casts to a pointer to a pointer, and pointers to
 * structs never defined (opaque handles)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] *(unsigned char **)(b + 2) loads the whole pointer (it loaded one
 *       byte: a free-list walk lost the high byte); through a variable too
 *   [1] **(T **)e is a T; ((T **)e) + 1 steps over a pointer; (T **)n of an
 *       integer address
 *   [2] a pointer to a struct never defined in the file: members, globals,
 *       locals, parameters, comparisons, passing and returning, casts
 *   [3] $AA sentinel
 */
#include <stddef.h>

#define OUT ((volatile unsigned char *)0xC0C0)

static unsigned char blk[4][6];     /* [0..1] a value, [2..3] the next block */

static unsigned char *next_of(unsigned char *b) { return *(unsigned char **)(b + 2); }

static int t_walk(void) {
    unsigned char *b, *c;
    unsigned char i, n = 0;
    unsigned int sum = 0;
    int ok = 1;
    for (i = 0; i < 4; i++) {
        *(unsigned int *)blk[i] = 0x1111u * (i + 1);
        *(unsigned char **)(blk[i] + 2) = i < 3 ? blk[i + 1] : 0;
    }
    for (b = blk[0]; b && n < 6; b = *(unsigned char **)(b + 2), n++) {
        if ((unsigned int)b != (unsigned int)blk[n]) ok = 0;
        sum += *(unsigned int *)b;
    }
    if (n != 4 || sum != 0xAAAA) ok = 0;
    c = next_of(blk[1]);
    if (c != blk[2]) ok = 0;
    {
        unsigned char **pp = (unsigned char **)(blk[2] + 2);
        if (*pp != blk[3]) ok = 0;
    }
    return ok;
}

static unsigned char bytes[4] = { 0x5A, 0xA5, 0x3C, 0xC3 };
static unsigned char *ptrs[3] = { bytes, bytes + 1, bytes + 3 };
static signed int words[2] = { -5, 1000 };
static signed int *wptrs[2] = { words + 1, words };

static int t_levels(void) {
    void *v = ptrs, *w = wptrs;
    int ok = 1;
    if (**(unsigned char **)v != 0x5A) ok = 0;                  /* a T */
    if (*((unsigned char **)v)[1] != 0xA5) ok = 0;
    if (**(((unsigned char **)v) + 2) != 0xC3) ok = 0;          /* steps 2 bytes */
    if (*(unsigned char **)((unsigned char **)v + 1) != bytes + 1) ok = 0;
    if (**(signed int **)w != 1000 || **((signed int **)w + 1) != -5) ok = 0;
    if ((unsigned int)((unsigned char **)v + 2) - (unsigned int)v != 4) ok = 0;
    if (*(unsigned char **)(unsigned int)&ptrs[2] != bytes + 3) ok = 0;
    if (*(unsigned char * volatile *)0xA000 != (unsigned char *)0x1234) ok = 0;   /* (set by main; RAM: $C000-$CFFF reads ROM) */
    return ok;
}

/* opaque handles: declared, never defined here */
struct opaque;
typedef struct handle handle_t;
struct box { struct opaque *o; struct never *n[3]; handle_t *h; int len; };

static struct opaque *g_o;
static struct never *g_arr[4];
static handle_t *g_h = NULL;

static struct opaque *pass(struct opaque *p) { return p; }
static int same(struct never *a, struct never *b) { return a == b; }

static int t_opaque(void) {
    static unsigned char mem[8];
    struct box b;
    struct opaque *l, *m;
    struct never *q;
    void *v;
    int ok = 1;
    b.o = (struct opaque *)mem;
    b.n[0] = 0; b.n[1] = (struct never *)(mem + 2); b.n[2] = (struct never *)(mem + 4);
    b.h = (handle_t *)(mem + 6);
    b.len = 3;
    l = b.o;
    q = b.n[1];
    v = l;
    m = (struct opaque *)v;
    g_o = pass(l);
    g_arr[2] = q;
    g_h = b.h;
    if (!l || l != m || g_o != (struct opaque *)mem) ok = 0;
    if (same(q, b.n[2]) || !same(q, g_arr[2])) ok = 0;
    if ((unsigned char *)g_h != mem + 6 || (char *)q - (char *)l != 2) ok = 0;
    if (sizeof(struct opaque *) != 2 || sizeof b != 12) ok = 0;
    return ok;
}

int main(void) {
    *(unsigned char **)0xA000 = (unsigned char *)0x1234;
    OUT[0] = t_walk();
    OUT[1] = t_levels();
    OUT[2] = t_opaque();
    OUT[3] = 0xAA;
    return 0;
}
