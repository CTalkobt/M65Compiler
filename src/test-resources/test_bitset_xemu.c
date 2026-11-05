/* test_bitset_xemu.c — <bitset.h> (#408) against a model of one byte per bit
 *
 * Results at $C0C0 (01 = pass):
 *   [0] set / clear / toggle / test with variable indices: 3000 random ones
 *       on 203 bits against the model
 *   [1] the same with constant indices (the header's expressions)
 *   [2] set_all / clear_all / not (the bits past nbits stay 0), count, any / none
 *   [3] set_range / clear_range: 300 random ranges against the model
 *   [4] find_first_set / find_first_clear from every place
 *   [5] and / or / xor against the model
 *   [6] $AA sentinel
 */
#include <string.h>
#include <bitset.h>

#define OUT ((volatile unsigned char *)0xC0C0)
#define NB 203

static unsigned int seed = 99;
static unsigned int rnd(void) { seed = seed * 25173u + 13849u; return seed >> 3; }

BITSET(bs, NB);
BITSET(bt, NB);
static unsigned char model[NB], model2[NB];

static int same(const unsigned char *b, const unsigned char *m) {
    unsigned int i;
    for (i = 0; i < NB; i++) if (((b[i >> 3] >> (i & 7)) & 1) != m[i]) return 0;
    for (i = NB; i < BS_BYTES(NB) * 8; i++) if ((b[i >> 3] >> (i & 7)) & 1) return 0;   /* past nbits: 0 */
    return 1;
}

static unsigned int mcount(const unsigned char *m) {
    unsigned int i, n = 0;
    for (i = 0; i < NB; i++) n += m[i];
    return n;
}

static int t_variable(void) {
    unsigned int n, i;
    int ok = 1;
    bs_clear_all(bs, NB);
    memset(model, 0, NB);
    for (n = 0; n < 3000 && ok; n++) {
        i = rnd() % NB;
        switch (rnd() & 3) {
        case 0: bs_set(bs, i); model[i] = 1; break;
        case 1: bs_clear(bs, i); model[i] = 0; break;
        case 2: bs_toggle(bs, i); model[i] ^= 1; break;
        default: if (bs_test(bs, i) != model[i]) ok = 0; break;
        }
    }
    return ok && same(bs, model);
}

static int t_constant(void) {
    int ok = 1;
    bs_clear_all(bt, NB);
    bs_set(bt, 0); bs_set(bt, 9); bs_set(bt, 202); bs_set(bt, 15);
    bs_clear(bt, 9); bs_toggle(bt, 100); bs_toggle(bt, 15);
    bs_assign(bt, 7, 1); bs_assign(bt, 0, 0);
    if (!bs_test(bt, 202) || !bs_test(bt, 100) || !bs_test(bt, 7) || bs_test(bt, 9) || bs_test(bt, 15) || bs_test(bt, 0)) ok = 0;
    if (bt[25] != 0x04 || bt[12] != 0x10 || bt[0] != 0x80) ok = 0;
    return ok && bs_count(bt, NB) == 3;
}

static int t_whole(void) {
    int ok = 1;
    unsigned int i;
    bs_set_all(bs, NB);
    memset(model, 1, NB);
    if (!same(bs, model) || bs_count(bs, NB) != NB || !bs_any(bs, NB)) ok = 0;
    bs_clear(bs, 77); model[77] = 0;
    bs_not(bs, NB);
    for (i = 0; i < NB; i++) model[i] ^= 1;
    if (!same(bs, model) || bs_count(bs, NB) != 1) ok = 0;
    bs_clear_all(bs, NB);
    if (bs_any(bs, NB) || !bs_none(bs, NB) || bs_count(bs, NB) != 0) ok = 0;
    bs_set(bs, NB - 1);
    if (!bs_any(bs, NB) || bs_count(bs, NB) != 1) ok = 0;
    return ok;
}

static int t_ranges(void) {
    unsigned int n, from, len, i;
    int ok = 1;
    bs_clear_all(bs, NB);
    memset(model, 0, NB);
    for (n = 0; n < 300 && ok; n++) {
        from = rnd() % NB;
        len = rnd() % (NB - from + 1);
        if (rnd() & 1) { bs_set_range(bs, from, len); for (i = 0; i < len; i++) model[from + i] = 1; }
        else { bs_clear_range(bs, from, len); for (i = 0; i < len; i++) model[from + i] = 0; }
        if (!same(bs, model) || bs_count(bs, NB) != mcount(model)) ok = 0;
    }
    return ok;
}

static int t_find(void) {
    unsigned int from, i;
    int ok = 1;
    signed int want, got;
    bs_clear_all(bs, NB);
    memset(model, 0, NB);
    for (i = 0; i < 12; i++) { unsigned int k = rnd() % NB; bs_set(bs, k); model[k] = 1; }
    bs_set_range(bs, 120, 40);
    for (i = 120; i < 160; i++) model[i] = 1;
    for (from = 0; from <= NB; from++) {
        want = -1;
        for (i = from; i < NB; i++) if (model[i]) { want = (signed int)i; break; }
        got = bs_find_first_set(bs, NB, from);
        if (got != want) ok = 0;
        want = -1;
        for (i = from; i < NB; i++) if (!model[i]) { want = (signed int)i; break; }
        got = bs_find_first_clear(bs, NB, from);
        if (got != want) ok = 0;
    }
    bs_set_all(bs, NB);
    if (bs_find_first_clear(bs, NB, 0) != -1 || bs_find_first_set(bs, NB, 202) != 202) ok = 0;
    return ok;
}

static int t_logic(void) {
    unsigned int i, op;
    int ok = 1;
    for (op = 0; op < 3; op++) {
        bs_clear_all(bs, NB);
        bs_clear_all(bt, NB);
        for (i = 0; i < NB; i++) {
            model[i] = rnd() & 1; model2[i] = rnd() & 1;
            if (model[i]) bs_set(bs, i);
            if (model2[i]) bs_set(bt, i);
        }
        if (op == 0) { bs_and(bs, bt, NB); for (i = 0; i < NB; i++) model[i] &= model2[i]; }
        if (op == 1) { bs_or(bs, bt, NB); for (i = 0; i < NB; i++) model[i] |= model2[i]; }
        if (op == 2) { bs_xor(bs, bt, NB); for (i = 0; i < NB; i++) model[i] ^= model2[i]; }
        if (!same(bs, model)) ok = 0;
    }
    return ok;
}

int main(void) {
    OUT[0] = t_variable();
    OUT[1] = t_constant();
    OUT[2] = t_whole();
    OUT[3] = t_ranges();
    OUT[4] = t_find();
    OUT[5] = t_logic();
    OUT[6] = 0xAA;
    return 0;
}
