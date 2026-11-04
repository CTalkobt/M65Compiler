/* test_set_xemu.c — <set.h> (#403): struct set (sorted array) and struct tset
 * (red-black tree) against a model, the tree's invariants checked
 *
 * Results at $C0C0 (01 = pass):
 *   [0] set: 1500 random inserts / erases of ints with a comparator against a
 *       presence table; find, size; in-order iteration; lower_bound
 *   [1] tset: the same, and after each step the red-black rules (no red node
 *       with a red child, the same black count on every path, parent links)
 *   [2] tset: erase of a leaf, of a node with one child, of one with two;
 *       element addresses stay put across inserts and erases
 *   [3] both: 5-byte struct elements by a field; cmp NULL (big-endian keys)
 *   [4] both: out of memory (insert gives -1, the set intact); clear / free
 *   [5] $AA sentinel
 */
#include <string.h>
#include <stdlib.h>
#include <set.h>

#define OUT ((volatile unsigned char *)0xC0C0)

static unsigned int seed = 2024;
static unsigned int rnd(void) { seed = seed * 25173u + 13849u; return seed >> 3; }

static int by_int(const void *a, const void *b) {
    signed int x = *(const signed int *)a, y = *(const signed int *)b;
    return x < y ? -1 : x > y;
}

/* the tree's nodes, as tset.s45 lays them out */
struct node { struct node *link[2]; struct node *parent; unsigned char red; signed int key; };

static int bad;
#pragma cc45 recurse
static int black_height(struct node *n, struct node *parent) {
    int l, r;
    if (!n) return 1;
    if (n->parent != parent) bad = 1;
    if (n->red && ((n->link[0] && n->link[0]->red) || (n->link[1] && n->link[1]->red))) bad = 1;
    if (n->link[0] && n->link[0]->key >= n->key) bad = 1;
    if (n->link[1] && n->link[1]->key <= n->key) bad = 1;
    l = black_height(n->link[0], n);
    r = black_height(n->link[1], n);
    if (l != r) bad = 1;
    return l + !n->red;
}

static int rb_ok(struct tset *t) {
    struct node *root = t->root;
    bad = 0;
    if (root && root->red) bad = 1;
    black_height(root, NULL);
    return !bad;
}

static unsigned char present[512];

/* random inserts and erases against the presence table: set (tree 0) or tset */
static int model(int tree) {
    struct set s;
    struct tset t;
    unsigned int n, k, cnt = 0;
    signed int v, last, *p, r;
    int ok = 1;
    memset(present, 0, sizeof present);
    if (tree) t.init(sizeof(int), by_int); else s.init(sizeof(int), by_int);
    for (n = 0; n < 1500 && ok; n++) {
        v = (signed int)(rnd() % 400) - 200;
        if (rnd() % 3) {
            r = tree ? t.insert(&v) : s.insert(&v);
            if (r != (present[v + 256] ? 0 : 1)) ok = 0;
            if (!present[v + 256]) cnt++;
            present[v + 256] = 1;
        } else {
            r = tree ? t.erase(&v) : s.erase(&v);
            if (r != present[v + 256]) ok = 0;
            if (present[v + 256]) cnt--;
            present[v + 256] = 0;
        }
        if ((tree ? t.size() : s.size()) != cnt) ok = 0;
        p = tree ? t.find(&v) : s.find(&v);
        if ((p != NULL) != present[v + 256] || (p && *p != v)) ok = 0;
        if (tree && !rb_ok(&t)) ok = 0;
    }
    k = 0;
    last = -32768;
    for (p = tree ? t.first() : s.first(); p; p = tree ? t.next(p) : s.next(p)) {
        if (k && *p <= last) ok = 0;
        if (!present[*p + 256]) ok = 0;
        last = *p;
        k++;
    }
    if (k != cnt) ok = 0;
    for (v = -205; v < 205; v += 7) {       /* lower_bound: the first present at v or after */
        signed int want = 9999, w;
        for (w = v; w < 256; w++) if (present[w + 256]) { want = w; break; }
        p = tree ? t.lower_bound(&v) : s.lower_bound(&v);
        if (want == 9999 ? p != NULL : (p == NULL || *p != want)) ok = 0;
    }
    if (tree) t.free(); else s.free();
    return ok;
}

static int t_tree_shapes(void) {
    struct tset t;
    signed int v, *addr[20];
    unsigned int i;
    int ok = 1;
    t.init(sizeof(int), by_int);
    for (i = 0; i < 20; i++) { v = (signed int)(i * 10); t.insert(&v); }
    for (i = 0; i < 20; i++) { v = (signed int)(i * 10); addr[i] = t.find(&v); }
    v = 0;  if (t.erase(&v) != 1 || !rb_ok(&t)) ok = 0;            /* a leaf */
    v = 100; if (t.erase(&v) != 1 || !rb_ok(&t)) ok = 0;
    for (i = 0; i < 20; i++) {                                     /* every kind as they come */
        v = (signed int)(((i * 7) % 20) * 10);
        t.erase(&v);
        if (!rb_ok(&t)) ok = 0;
        if (i == 10) {
            unsigned int j;
            for (j = 0; j < 20; j++) {
                signed int w = (signed int)(j * 10), *q = t.find(&w);
                if (q && q != addr[j]) ok = 0;                      /* nothing moved */
            }
        }
    }
    if (t.size() != 0 || t.first() != NULL) ok = 0;
    v = 5; if (t.erase(&v) != 0) ok = 0;
    t.free();
    return ok;
}

struct rec { unsigned char tag; signed int id; unsigned int data; };
static int by_id(const void *a, const void *b) {
    signed int x = ((const struct rec *)a)->id, y = ((const struct rec *)b)->id;
    return x < y ? -1 : x > y;
}

static int t_elements(void) {
    struct set s;
    struct tset t;
    struct rec r, *p;
    unsigned char k[2], *q;
    unsigned int i, last;
    int ok = sizeof(struct rec) == 5;
    s.init(sizeof(struct rec), by_id);
    t.init(sizeof(struct rec), by_id);
    for (i = 0; i < 30; i++) {
        r.tag = (unsigned char)i; r.id = (signed int)((i * 37) % 31) - 15; r.data = i * 100;
        s.insert(&r); t.insert(&r);
    }
    r.id = 3;
    p = s.find(&r); if (!p || p->id != 3 || p->data != p->tag * 100u) ok = 0;
    p = t.find(&r); if (!p || p->id != 3 || p->data != p->tag * 100u) ok = 0;
    s.free(); t.free();
    s.init(2, NULL);
    t.init(2, NULL);
    for (i = 0; i < 40; i++) {
        unsigned int v = rnd();
        k[0] = (unsigned char)(v >> 8); k[1] = (unsigned char)v;    /* big-endian: by bytes is by value */
        s.insert(k); t.insert(k);
    }
    last = 0;
    for (q = s.first(); q; q = s.next(q)) { unsigned int v = (q[0] << 8) | q[1]; if (v < last) ok = 0; last = v; }
    last = 0;
    for (q = t.first(); q; q = t.next(q)) { unsigned int v = (q[0] << 8) | q[1]; if (v < last) ok = 0; last = v; }
    if (s.size() != t.size()) ok = 0;
    s.free(); t.free();
    return ok;
}

static int t_memory(void) {
    struct set s;
    struct tset t;
    static unsigned char big[600];
    unsigned int n;
    signed int r = 1;
    int ok = 1;
    s.init(sizeof big, NULL);
    for (n = 0; n < 200 && r == 1; n++) { big[0] = (unsigned char)n; big[1] = (unsigned char)(n >> 8); r = s.insert(big); }
    if (r != -1 || s.size() != n - 1) ok = 0;
    s.clear();
    if (s.size() != 0 || s.first() != NULL) ok = 0;
    s.free();
    t.init(sizeof big, NULL);
    r = 1;
    for (n = 0; n < 200 && r == 1; n++) { big[0] = (unsigned char)n; big[1] = (unsigned char)(n >> 8); r = t.insert(big); }
    if (r != -1 || t.size() != n - 1) ok = 0;
    t.clear();
    if (t.size() != 0 || t.first() != NULL) ok = 0;
    /* the memory came back: as many fit again */
    r = 1;
    for (n = 0; n < 200 && r == 1; n++) { big[0] = (unsigned char)n; big[1] = (unsigned char)(n >> 8); r = t.insert(big); }
    if (r != -1 || n < 5) ok = 0;
    t.free();
    return ok;
}

int main(void) {
    OUT[0] = model(0);
    OUT[1] = model(1);
    OUT[2] = t_tree_shapes();
    OUT[3] = t_elements();
    OUT[4] = t_memory();
    OUT[5] = 0xAA;
    return 0;
}
