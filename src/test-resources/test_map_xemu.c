/* test_map_xemu.c — <map.h> (#407): struct map (sorted array) and struct tmap
 * (red-black tree) against a model
 *
 * Results at $C0C0 (01 = pass):
 *   [0] map: 1500 random puts / erases of int keys and values against a table;
 *       get, size, put's replace; in-order iteration with the values;
 *       lower_bound
 *   [1] tmap: the same
 *   [2] both: 5-byte struct values; cmp NULL on big-endian keys; a value
 *       replaced in place (tmap: its address kept)
 *   [3] both: out of memory (put gives -1, the map intact); clear / free
 *   [4] $AA sentinel
 */
#include <string.h>
#include <stdlib.h>
#include <map.h>

#define OUT ((volatile unsigned char *)0xC0C0)

static unsigned int seed = 77;
static unsigned int rnd(void) { seed = seed * 25173u + 13849u; return seed >> 3; }

static int by_int(const void *a, const void *b) {
    signed int x = *(const signed int *)a, y = *(const signed int *)b;
    return x < y ? -1 : x > y;
}

static unsigned char present[512];
static unsigned int value[512];

static int model(int tree) {
    struct map m;
    struct tmap t;
    unsigned int n, k = 0, cnt = 0, val, *vp;
    signed int key, last, *e, r;
    int ok = 1;
    memset(present, 0, sizeof present);
    if (tree) t.init(sizeof(int), sizeof(int), by_int); else m.init(sizeof(int), sizeof(int), by_int);
    for (n = 0; n < 1500 && ok; n++) {
        key = (signed int)(rnd() % 300) - 150;
        if (rnd() % 3) {
            val = rnd();
            r = tree ? t.put(&key, &val) : m.put(&key, &val);
            if (r != (present[key + 256] ? 0 : 1)) ok = 0;
            if (!present[key + 256]) cnt++;
            present[key + 256] = 1;
            value[key + 256] = val;
        } else {
            r = tree ? t.erase(&key) : m.erase(&key);
            if (r != present[key + 256]) ok = 0;
            if (present[key + 256]) cnt--;
            present[key + 256] = 0;
        }
        if ((tree ? t.size() : m.size()) != cnt) ok = 0;
        key = (signed int)(rnd() % 300) - 150;
        vp = tree ? t.get(&key) : m.get(&key);
        if ((vp != NULL) != present[key + 256] || (vp && *vp != value[key + 256])) ok = 0;
    }
    last = -32768;
    for (e = tree ? t.first() : m.first(); e; e = tree ? t.next(e) : m.next(e)) {
        if (k && *e <= last) ok = 0;
        if (!present[*e + 256] || e[1] != (signed int)value[*e + 256]) ok = 0;
        last = *e;
        k++;
    }
    if (k != cnt) ok = 0;
    for (key = -155; key < 155; key += 7) {
        signed int want = 9999, w;
        for (w = key; w < 256; w++) if (present[w + 256]) { want = w; break; }
        e = tree ? t.lower_bound(&key) : m.lower_bound(&key);
        if (want == 9999 ? e != NULL : (e == NULL || *e != want)) ok = 0;
    }
    if (tree) t.free(); else m.free();
    return ok;
}

struct rec { unsigned char tag; signed int a; unsigned int b; };

static int t_values(void) {
    struct map m;
    struct tmap t;
    struct rec r, *p, *q;
    unsigned char k[2], *e;
    unsigned int i, last;
    int ok = sizeof(struct rec) == 5;
    m.init(2, sizeof(struct rec), NULL);
    t.init(2, sizeof(struct rec), NULL);
    for (i = 0; i < 60; i++) {
        unsigned int v = rnd();
        k[0] = (unsigned char)(v >> 8); k[1] = (unsigned char)v;
        r.tag = k[1]; r.a = -(signed int)i; r.b = v;
        m.put(k, &r); t.put(k, &r);
    }
    if (m.size() != t.size()) ok = 0;
    last = 0;
    for (e = m.first(); e; e = m.next(e)) {         /* big-endian: by bytes is by value */
        unsigned int v = (e[0] << 8) | e[1];
        p = (struct rec *)(e + 2);
        if (v < last || p->b != v || p->tag != e[1]) ok = 0;
        last = v;
    }
    last = 0;
    for (e = t.first(); e; e = t.next(e)) {
        unsigned int v = (e[0] << 8) | e[1];
        p = (struct rec *)(e + 2);
        if (v < last || p->b != v) ok = 0;
        last = v;
    }
    e = t.first();                                  /* replace: in place */
    q = t.get(e);
    r.tag = 0x5A; r.a = 1234; r.b = 4321;
    if (q != (struct rec *)(e + 2) || t.put(e, &r) != 0 || t.get(e) != q || q->a != 1234 || q->tag != 0x5A) ok = 0;
    e = m.first();
    if (m.put(e, &r) != 0 || ((struct rec *)m.get(e))->b != 4321) ok = 0;
    k[0] = 0xFF; k[1] = 0xFF;
    if (m.get(k) != NULL && ((struct rec *)m.get(k))->b != 0xFFFF) ok = 0;
    m.free(); t.free();
    return ok;
}

static int t_memory(void) {
    struct map m;
    struct tmap t;
    static unsigned char big[500];
    unsigned int n, key;
    signed int r = 1;
    int ok = 1;
    m.init(sizeof key, sizeof big, NULL);
    for (n = 0; n < 200 && r == 1; n++) { key = n; big[0] = (unsigned char)n; r = m.put(&key, big); }
    if (r != -1 || m.size() != n - 1) ok = 0;
    key = 3;
    if (!m.get(&key) || *(unsigned char *)m.get(&key) != 3) ok = 0;
    m.clear();
    if (m.size() != 0 || m.first() != NULL) ok = 0;
    m.free();
    t.init(sizeof key, sizeof big, NULL);
    r = 1;
    for (n = 0; n < 200 && r == 1; n++) { key = n; big[0] = (unsigned char)n; r = t.put(&key, big); }
    if (r != -1 || t.size() != n - 1) ok = 0;
    key = 5;
    if (!t.get(&key) || *(unsigned char *)t.get(&key) != 5) ok = 0;
    t.clear();
    if (t.size() != 0 || t.first() != NULL) ok = 0;
    t.free();
    return ok;
}

int main(void) {
    OUT[0] = model(0);
    OUT[1] = model(1);
    OUT[2] = t_values();
    OUT[3] = t_memory();
    OUT[4] = 0xAA;
    return 0;
}
