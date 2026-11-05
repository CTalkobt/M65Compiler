/* test_hashmap_xemu.c — <hashmap.h> (#407): struct hashmap / struct hashset
 * against a model
 *
 * Results at $C0C0 (01 = pass):
 *   [0] hashmap, built-in hash and cmp: 2000 random puts / erases / gets of
 *       int keys against a table; size; iteration sees every element once
 *       with its value; the table grew
 *   [1] the same with the program's hash (4 values: long probes) and cmp
 *   [2] C string keys (ksize 0): equal text in another buffer finds them;
 *       replace, erase; hash_str / hash_bytes; strcmp as cmp
 *   [3] hashset of 5-byte keys: insert / find / erase, rehash(n), clear,
 *       free and use again
 *   [4] out of memory: put gives -1, the table intact
 *   [5] $AA sentinel
 */
#include <string.h>
#include <stdlib.h>
#include <hashmap.h>

#define OUT ((volatile unsigned char *)0xC0C0)

static unsigned int seed = 4242;
static unsigned int rnd(void) { seed = seed * 25173u + 13849u; return seed >> 3; }

static unsigned char present[400];
static unsigned int value[400];
static unsigned char seen[400];

static unsigned int calls;
static unsigned int weak_hash(const void *k) { calls++; return *(const unsigned int *)k & 3; }
static int int_cmp(const void *a, const void *b) { return *(const unsigned int *)a != *(const unsigned int *)b; }

static int model(int own) {
    struct hashmap m;
    unsigned int n, key, val, cnt = 0, k = 0, *e, *vp;
    signed int r;
    int ok = 1;
    memset(present, 0, sizeof present);
    calls = 0;
    if (own) m.init(sizeof(int), sizeof(int), 0, weak_hash, int_cmp);
    else m.init(sizeof(int), sizeof(int), 0, NULL, NULL);
    for (n = 0; n < (own ? 600u : 2000u) && ok; n++) {
        key = rnd() % (own ? 60u : 400u);
        if (rnd() % 3) {
            val = rnd();
            r = m.put(&key, &val);
            if (r != (present[key] ? 0 : 1)) ok = 0;
            if (!present[key]) cnt++;
            present[key] = 1;
            value[key] = val;
        } else {
            r = m.erase(&key);
            if (r != present[key]) ok = 0;
            if (present[key]) cnt--;
            present[key] = 0;
        }
        if (m.size() != cnt) ok = 0;
        key = rnd() % (own ? 60u : 400u);
        vp = m.get(&key);
        if ((vp != NULL) != present[key] || (vp && *vp != value[key])) ok = 0;
        if (m.slots && (m.used >= m.cap || m.used < m.len)) ok = 0;
    }
    memset(seen, 0, sizeof seen);
    for (e = m.first(); e; e = m.next(e)) {
        if (*e >= 400 || !present[*e] || seen[*e] || e[1] != value[*e]) ok = 0;
        else seen[*e] = 1;
        k++;
    }
    if (k != cnt) ok = 0;
    if (m.cap < 16 || (m.cap & (m.cap - 1))) ok = 0;
    if (own && calls == 0) ok = 0;
    m.free();
    if (m.size() != 0 || m.first() != NULL) ok = 0;
    return ok;
}

static int t_strings(void) {
    static char names[40][8], probe[8];
    struct hashmap m, s;
    unsigned int i, v, *vp;
    char **e;
    int ok = 1;
    unsigned char b[3] = { 1, 2, 3 };
    if (hash_str("") != 5381 || hash_str("ab") != hash_bytes("ab", 2)) ok = 0;
    if (hash_bytes(b, 3) == hash_bytes(b, 2)) ok = 0;
    m.init(0, sizeof(int), 8, NULL, NULL);
    for (i = 0; i < 40; i++) {
        names[i][0] = 'k';
        names[i][1] = (char)('a' + i % 26);
        names[i][2] = (char)('0' + i / 26);
        names[i][3] = 0;
        v = i * 3;
        if (m.put(names[i], &v) != 1) ok = 0;
    }
    for (i = 0; i < 40; i++) {
        strcpy(probe, names[i]);                    /* another buffer, the same text */
        vp = m.get(probe);
        if (!vp || *vp != i * 3) ok = 0;
        e = m.find(probe);
        if (!e || *e != names[i]) ok = 0;           /* the pointer kept */
    }
    strcpy(probe, "kz9");
    if (m.get(probe) != NULL) ok = 0;
    v = 999;
    strcpy(probe, names[7]);
    if (m.put(probe, &v) != 0 || *(unsigned int *)m.get(names[7]) != 999) ok = 0;
    if (m.erase(probe) != 1 || m.get(names[7]) != NULL || m.erase(probe) != 0 || m.size() != 39) ok = 0;
    m.free();
    s.init(0, 0, 0, NULL, (hash_cmp_t)strcmp);      /* a set of strings, strcmp */
    for (i = 0; i < 40; i++) s.put(names[i], NULL);
    strcpy(probe, names[33]);
    if (s.find(probe) == NULL || s.size() != 40) ok = 0;
    s.free();
    return ok;
}

struct key5 { unsigned char a; unsigned int b, c; };

static int t_set(void) {
    struct hashset h;
    struct key5 k;
    unsigned int i, n = 0;
    void *e;
    int ok = sizeof(struct key5) == 5;
    h.init(sizeof k, 0, NULL, NULL);
    for (i = 0; i < 100; i++) {
        k.a = (unsigned char)i; k.b = i * 7; k.c = ~i;
        if (h.insert(&k) != 1) ok = 0;
    }
    k.a = 5; k.b = 35; k.c = ~5u;
    if (h.insert(&k) != 0 || h.size() != 100) ok = 0;
    for (i = 0; i < 100; i += 2) {
        k.a = (unsigned char)i; k.b = i * 7; k.c = ~i;
        if (h.erase(&k) != 1) ok = 0;
    }
    for (i = 0; i < 100; i++) {
        k.a = (unsigned char)i; k.b = i * 7; k.c = ~i;
        e = h.find(&k);
        if ((e != NULL) != (i & 1) || (e && memcmp(e, &k, 5))) ok = 0;
    }
    k.b = 1;
    if (h.find(&k) != NULL) ok = 0;
    if (h.rehash(1024) != 0 || h.cap != 1024 || h.used != 50 || h.size() != 50) ok = 0;
    for (e = h.first(); e; e = h.next(e)) n++;
    if (n != 50) ok = 0;
    k.a = 9; k.b = 63; k.c = ~9u;
    if (h.find(&k) == NULL) ok = 0;
    if (h.rehash(0) != 0 || h.cap != 128) ok = 0;        /* as small as 50 allows */
    h.clear();
    if (h.size() != 0 || h.first() != NULL || h.find(&k) != NULL) ok = 0;
    if (h.insert(&k) != 1 || h.find(&k) == NULL) ok = 0;
    h.free();
    if (h.insert(&k) != 1 || h.find(&k) == NULL || h.size() != 1) ok = 0;
    h.free();
    return ok;
}

static int t_memory(void) {
    struct hashmap m;
    static unsigned char big[700];
    unsigned int n, key;
    signed int r = 1;
    int ok = 1;
    m.init(sizeof key, sizeof big, 0, NULL, NULL);
    for (n = 0; n < 200 && r == 1; n++) { key = n; big[0] = (unsigned char)n; r = m.put(&key, big); }
    if (r != -1 || m.size() != n - 1 || n < 4) ok = 0;
    for (key = 0; key < n - 1; key++) {
        unsigned char *v = m.get(&key);
        if (!v || *v != (unsigned char)key) ok = 0;
    }
    m.free();
    return ok;
}

int main(void) {
    OUT[0] = model(0);
    OUT[1] = model(1);
    OUT[2] = t_strings();
    OUT[3] = t_set();
    OUT[4] = t_memory();
    OUT[5] = 0xAA;
    return 0;
}
