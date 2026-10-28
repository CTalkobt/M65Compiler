/* test_far_malloc_xemu.c — <far_ptr.h>'s chip RAM heap (banks 4 and 5)
 *
 * The allocator's behaviour pinned down: first fit over a bank's records (in
 * address order, at most 32 a bank), a split leaving the rest free after
 * the block, free merging neighbours, far_reserve's ranges carved out.
 * Results at $C0C0 (01 = pass):
 *   [0] the first blocks: bank 4 from $0100, one after another; bank 5 on
 *       its own; far_size, far_alloc_bank, far_is_valid
 *   [1] free and merge: a freed block's room taken again (first fit); all
 *       freed: one block of $FF00 again, then nothing more
 *   [2] the 32-record limit: a split refused, an exact fit still given
 *   [3] what is refused: size 0, banks 0-3 and 6-7, a 64 KB block;
 *       far_free of null, twice, of an address never given; far_size 0
 *   [4] bank_available (reserved and allocated counted as used),
 *       bank_reset, bank_reset_all
 *   [5] far_calloc zeroes, far_realloc moves and keeps the data, the same
 *       size and bank keeps the pointer, from null allocates
 *   [6] far_malloc_current: the fixed bank, first fit (bank 4, then 5),
 *       round robin
 *   [7] 1500 random far_malloc / far_free in both banks against a model
 *       (the algorithm written out here): the same offsets, sizes and free
 *       bytes, the same refusals
 *   [8] far_reserve: before the first block (skipped), after it (the free
 *       room carved), bank_reset keeps it, ranges refused (outside banks
 *       4-5, last < first, a ninth)
 *   [9] 01
 *  [10] $AA sentinel
 */
#include <far_ptr.h>
#include <far_ops.h>

#define OUT ((volatile unsigned char *)0xC0C0)
#define NULLP(p) far_is_null(p)

static unsigned int off(far_ptr_t p) { return far_offset(p); }

static unsigned char t_first(void) {
    far_ptr_t a, b, c, d;
    bank_reset_all();
    a = far_malloc(100, BANK4);
    b = far_malloc(200, BANK4);
    c = far_malloc(1, BANK4);
    d = far_malloc(50, BANK5);
    if (far_bank(a) != BANK4 || off(a) != 0x0100 || off(b) != 0x0164 || off(c) != 0x022C) return 0;
    if (far_bank(d) != BANK5 || off(d) != 0x0100) return 0;
    if (far_size(a) != 100 || far_size(b) != 200 || far_size(c) != 1 || far_size(d) != 50) return 0;
    if (far_size_long(b) != 200UL || far_alloc_bank(d) != BANK5 || !far_is_valid(c)) return 0;
    return 1;
}

static unsigned char t_free(void) {
    far_ptr_t a, b, c, e, f;
    bank_reset_all();
    a = far_malloc(1000, BANK4);
    b = far_malloc(2000, BANK4);
    c = far_malloc(3000, BANK4);
    far_free(b);
    if (far_size(b) != 0 || far_is_valid(b)) return 0;
    e = far_malloc(1500, BANK4);        /* in b's room */
    if (off(e) != off(b)) return 0;
    f = far_malloc(500, BANK4);         /* the rest of it */
    if (off(f) != off(b) + 1500) return 0;
    far_free(a);
    far_free(e);
    far_free(f);
    far_free(c);                        /* all merged */
    a = far_malloc(0xFF00, BANK4);
    if (NULLP(a) || off(a) != 0x0100) return 0;
    if (!NULLP(far_malloc(1, BANK4))) return 0;
    far_free(a);
    a = far_malloc(0xFF00, BANK4);
    return !NULLP(a) && off(a) == 0x0100;
}

static unsigned char t_records(void) {
    far_ptr_t p[40], q;
    unsigned char i;
    bank_reset_all();
    /* 31 blocks of 16 and the rest: 32 records */
    for (i = 0; i < 31; i++) {
        p[i] = far_malloc(16, BANK4);
        if (NULLP(p[i]) || off(p[i]) != 0x0100 + 16u * i) return 0;
    }
    if (!NULLP(far_malloc(16, BANK4))) return 0;       /* a split: no record left */
    q = far_malloc(0xFF00 - 31u * 16u, BANK4);           /* the exact rest */
    if (NULLP(q) || off(q) != 0x0100 + 31u * 16u) return 0;
    far_free(p[3]);
    far_free(p[4]);                                      /* merged: 31 records */
    q = far_malloc(16, BANK4);                           /* a split again */
    return !NULLP(q) && off(q) == off(p[3]);
}

static unsigned char t_refused(void) {
    far_ptr_t p, n;
    unsigned char b;
    bank_reset_all();
    n = far_make(BANK0, 0);
    if (!NULLP(far_malloc(0, BANK4))) return 0;
    for (b = 0; b < 8; b++)
        if (b != 4 && b != 5 && !NULLP(far_malloc(10, (bank_t)b))) return 0;
    if (!NULLP(far_malloc_long(0x10000UL, BANK4)) || !NULLP(far_malloc(0xFF01, BANK4))) return 0;
    far_free(n);
    p = far_malloc(10, BANK4);
    far_free(p);
    far_free(p);                                    /* twice */
    far_free(far_make(BANK4, 0x1234));              /* never given */
    far_free(far_make(BANK6, 0x0100));
    if (far_size(p) != 0 || far_size(n) != 0 || far_size(far_make(BANK7, 0x100)) != 0) return 0;
    p = far_malloc(10, BANK4);
    return !NULLP(p) && off(p) == 0x0100 && bank_available(BANK4) == 0xFF00 - 10;
}

static unsigned char t_available(void) {
    far_ptr_t a, b;
    bank_reset_all();
    if (bank_available(BANK4) != 0xFF00 || bank_available(BANK5) != 0xFF00) return 0;
    if (bank_available(BANK0) != 0 || bank_available(BANK6) != 0) return 0;
    a = far_malloc(0x1000, BANK4);
    b = far_malloc(0x2000, BANK5);
    if (bank_available(BANK4) != 0xEF00 || bank_available(BANK5) != 0xDF00) return 0;
    bank_reset(BANK4);
    if (bank_available(BANK4) != 0xFF00 || bank_available(BANK5) != 0xDF00) return 0;
    a = far_malloc(0x10, BANK4);
    if (off(a) != 0x0100) return 0;
    bank_reset_all();
    b = far_malloc(0x10, BANK5);
    return off(b) == 0x0100 && bank_available(BANK5) == 0xFEF0;
}

static unsigned char t_calloc_realloc(void) {
    far_ptr_t a, b, c;
    unsigned int i;
    bank_reset_all();
    a = far_malloc(64, BANK4);
    far_memset(a, 0x5A, 64);
    far_free(a);
    a = far_calloc(8, 8, BANK4);
    for (i = 0; i < 64; i++)
        if (far_read_uint8(a, i)) return 0;
    for (i = 0; i < 64; i++) far_write_uint8(a, i, (uint8_t)i);
    b = far_realloc(a, 64, BANK4);              /* the same: kept */
    if (off(b) != off(a)) return 0;
    c = far_realloc(a, 200, BANK5);             /* moved, the data kept, a freed */
    if (far_bank(c) != BANK5 || far_size(c) != 200 || far_size(a) != 0) return 0;
    for (i = 0; i < 64; i++)
        if (far_read_uint8(c, i) != (uint8_t)i) return 0;
    a = far_realloc(far_make(BANK0, 0), 30, BANK4);
    return far_size(a) == 30 && off(a) == 0x0100;
}

static unsigned char t_current(void) {
    far_ptr_t a, b, c;
    bank_reset_all();
    bank_set(BANK5);
    if (bank_get() != BANK5) return 0;
    a = far_malloc_current(10);
    if (far_bank(a) != BANK5) return 0;
    far_policy_first_fit();
    a = far_malloc_current(0xF000);
    b = far_malloc_current(0x1000);     /* bank 4 has not room: bank 5 */
    if (far_bank(a) != BANK4 || far_bank(b) != BANK5) return 0;
    bank_reset_all();
    far_policy_round_robin();
    a = far_malloc_current(10);
    b = far_malloc_current(10);
    c = far_malloc_current(10);
    if (far_bank(a) != BANK4 || far_bank(b) != BANK5 || far_bank(c) != BANK4) return 0;
    c = far_calloc_current(2, 5);
    if (far_bank(c) != BANK5 || far_size(c) != 10) return 0;
    far_policy_fixed_bank(BANK4);
    return far_bank(far_malloc_current(1)) == BANK4;
}

/* ---------------------------------------------------------------------------
 * The model: the algorithm as written out (two banks)
 * ------------------------------------------------------------------------- */
#define MAXR 32
static unsigned int m_off[2][MAXR], m_size[2][MAXR];
static unsigned char m_free[2][MAXR], m_n[2];

static unsigned int m_alloc(unsigned char k, unsigned int size) {
    unsigned char i, j;
    unsigned int at, rest;
    if (!size) return 0;
    if (!m_n[k]) {
        m_off[k][0] = 0x0100;
        m_size[k][0] = 0xFF00;
        m_free[k][0] = 1;
        m_n[k] = 1;
    }
    for (i = 0; i < m_n[k]; i++)
        if (m_free[k][i] == 1 && m_size[k][i] >= size) break;
    if (i == m_n[k]) return 0;
    at = m_off[k][i];
    rest = m_size[k][i] - size;
    if (rest) {
        if (m_n[k] >= MAXR) return 0;
        for (j = m_n[k]; j > i + 1; j--) {
            m_off[k][j] = m_off[k][j - 1];
            m_size[k][j] = m_size[k][j - 1];
            m_free[k][j] = m_free[k][j - 1];
        }
        m_off[k][i + 1] = at + size;
        m_size[k][i + 1] = rest;
        m_free[k][i + 1] = 1;
        m_n[k]++;
    }
    m_size[k][i] = size;
    m_free[k][i] = 0;
    return at;
}

static void m_release(unsigned char k, unsigned int at) {
    unsigned char i, j;
    for (i = 0; i < m_n[k]; i++)
        if (m_off[k][i] == at && !m_free[k][i]) break;
    if (i == m_n[k]) return;
    m_free[k][i] = 1;
    i = 0;
    while (i + 1 < m_n[k]) {
        if (m_free[k][i] == 1 && m_free[k][i + 1] == 1 && m_off[k][i] + m_size[k][i] == m_off[k][i + 1]) {
            m_size[k][i] += m_size[k][i + 1];
            for (j = i + 1; j + 1 < m_n[k]; j++) {
                m_off[k][j] = m_off[k][j + 1];
                m_size[k][j] = m_size[k][j + 1];
                m_free[k][j] = m_free[k][j + 1];
            }
            m_n[k]--;
        } else {
            i++;
        }
    }
}

static unsigned int m_avail(unsigned char k) {
    unsigned int used = 0;
    unsigned char i;
    for (i = 0; i < m_n[k]; i++)
        if (m_free[k][i] != 1) used += m_size[k][i];
    return 0xFF00 - used;
}

static unsigned int seed = 4321;
static unsigned int rnd(void) {
    seed = seed * 25173u + 13849u;
    return seed >> 3;
}

static far_ptr_t live[64];
static unsigned char nlive;

static unsigned char t_random(void) {
    unsigned int n, size, want;
    unsigned char k, i;
    far_ptr_t p;
    bank_reset_all();
    m_n[0] = m_n[1] = 0;
    nlive = 0;
    for (n = 0; n < 1500; n++) {
        k = (unsigned char)(rnd() & 1);
        if (nlive < 64 && (nlive == 0 || (rnd() % 100u) < 55u)) {
            switch (rnd() & 3) {
            case 0: size = (rnd() & 15) + 1; break;
            case 1: size = (rnd() & 1023) + 1; break;
            case 2: size = (rnd() & 8191) + 1; break;
            default: size = rnd() % 0xFF00u + 1; break;
            }
            want = m_alloc(k, size);
            p = far_malloc(size, (bank_t)(4 + k));
            if (want == 0) {
                if (!NULLP(p)) return 0;
            } else {
                if (NULLP(p) || far_bank(p) != 4 + k || off(p) != want || far_size(p) != size) return 0;
                live[nlive++] = p;
            }
        } else {
            i = (unsigned char)(rnd() % nlive);
            p = live[i];
            live[i] = live[--nlive];
            m_release((unsigned char)(far_bank(p) - 4), off(p));
            far_free(p);
            if (far_size(p) != 0) return 0;
        }
        if (bank_available(BANK4) != m_avail(0) || bank_available(BANK5) != m_avail(1)) return 0;
    }
    return 1;
}

/* far_reserve (last: bank_reset keeps its ranges) */
static unsigned char t_reserve(void) {
    far_ptr_t a, b, c;
    unsigned char i;
    bank_reset_all();
    if (!far_reserve(0x40200UL, 0x402FFUL)) return 0;   /* before the first block */
    a = far_malloc(0x100, BANK4);                         /* fits before it */
    b = far_malloc(0x10, BANK4);                          /* after it */
    if (off(a) != 0x0100 || off(b) != 0x0300) return 0;
    if (bank_available(BANK4) != 0xFF00 - 0x100 - 0x100 - 0x10) return 0;
    if (!far_reserve(0x40400UL, 0x404FFUL)) return 0;   /* after: the free room carved */
    c = far_malloc(0x200, BANK4);
    if (off(c) != 0x0500) return 0;
    if (!far_reserve(0x4FF00UL, 0x50120UL)) return 0;   /* across both banks */
    a = far_malloc(0x10, BANK5);
    if (off(a) != 0x0121) return 0;
    bank_reset_all();                                      /* the ranges stay */
    a = far_malloc(0x180, BANK4);                          /* $0100 and $0300 too small */
    b = far_malloc(0x100, BANK4);
    if (off(a) != 0x0500 || off(b) != 0x0100) return 0;
    if (far_reserve(0x30000UL, 0x3FFFFUL) || far_reserve(0x60000UL, 0x60010UL)) return 0;
    if (far_reserve(0x40900UL, 0x40800UL)) return 0;   /* last < first */
    for (i = 0; i < 5; i++)
        if (!far_reserve(0x41000UL + 0x100u * i, 0x41000UL + 0x100u * i)) return 0;
    return !far_reserve(0x42000UL, 0x42000UL);              /* a ninth */
}

int main(void) {
    unsigned char i;
    for (i = 0; i < 10; i++) OUT[i] = 0;
    OUT[10] = 0xAA;
    OUT[0] = t_first();
    OUT[1] = t_free();
    OUT[2] = t_records();
    OUT[3] = t_refused();
    OUT[4] = t_available();
    OUT[5] = t_calloc_realloc();
    OUT[6] = t_current();
    OUT[7] = t_random();
    OUT[8] = t_reserve();
    OUT[9] = 1;
    return 0;
}
