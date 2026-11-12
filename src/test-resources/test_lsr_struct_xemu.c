/* test_lsr_struct_xemu.c — #548: a loop over an array of structs by a byte
 * index keeps a pointer stepped by sizeof (no multiply per pass), and reads
 * the pointer itself where no step comes between; the same results as a
 * reference indexing through a volatile index (computed each time).
 *
 * Results at $C0C0: one byte per check, 0 = pass; $AA after the last
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define NCHECK 8
typedef struct { unsigned char where, kind, owner, prof, x, y, equip, tools, flags, ref, job, jt, jg, moves, task, work, cg[4], ca[4]; } unit_t;  /* 24 bytes */
#define MAX_UNITS 64
unit_t units[MAX_UNITS];
typedef struct { unsigned char a; unsigned int b; unsigned char c[2]; } odd_t;   /* 5 bytes */
odd_t odds[255];
static unsigned char res[NCHECK];

unsigned char count_at(unsigned char x, unsigned char y) {
    unsigned char u, n = 0;
    for (u = 0; u < MAX_UNITS; ++u)
        if (units[u].where == 1 && units[u].x == x && units[u].y == y) ++n;
    return n;
}
unsigned char count_ref(unsigned char x, unsigned char y) {
    volatile unsigned char u;
    unsigned char n = 0;
    for (u = 0; u < MAX_UNITS; ++u)
        if (units[u].where == 1 && units[u].x == x && units[u].y == y) ++n;
    return n;
}
/* the index used in the body and after its increment */
unsigned int index_sum(void) {
    unsigned char u;
    unsigned int s = 0;
    for (u = 0; u < MAX_UNITS; ) {
        s += units[u].kind * u;
        ++u;
        if (u < MAX_UNITS) s += units[u].owner;
    }
    return s;
}
unsigned int index_sum_ref(void) {
    volatile unsigned char u;
    unsigned int s = 0;
    for (u = 0; u < MAX_UNITS; ) {
        s += units[u].kind * u;
        ++u;
        if (u < MAX_UNITS) s += units[u].owner;
    }
    return s;
}
/* a break, a write through the element */
unsigned char first_free(void) {
    unsigned char u;
    for (u = 0; u < MAX_UNITS; ++u)
        if (!units[u].where) { units[u].flags |= 0x80; break; }
    return u;
}
/* nested loops */
unsigned int pairs(void) {
    unsigned char i, j;
    unsigned int n = 0;
    for (i = 0; i < 16; ++i)
        for (j = 0; j < 16; ++j)
            if (units[i].kind == units[j].kind) n += units[j].work;
    return n;
}
unsigned int pairs_ref(void) {
    volatile unsigned char i, j;
    unsigned int n = 0;
    for (i = 0; i < 16; ++i)
        for (j = 0; j < 16; ++j)
            if (units[i].kind == units[j].kind) n += units[j].work;
    return n;
}
/* a while loop from a start not known, an odd size, up to 254 */
unsigned int odd_sum(unsigned char from) {
    unsigned char i = from;
    unsigned int s = 0;
    while (i < 255) { s += odds[i].b + odds[i].c[1]; i++; }
    return s;
}
unsigned int odd_sum_ref(unsigned char from) {
    volatile unsigned char i = from;
    unsigned int s = 0;
    while (i < 255) { s += odds[i].b + odds[i].c[1]; i++; }
    return s;
}

int main(void) {
    unsigned int k;
    unsigned char x, y;
    for (k = 0; k < NCHECK; k++) res[k] = 0;
    for (k = 0; k < MAX_UNITS; k++) {
        units[k].where = (unsigned char)(k % 3 != 0);
        units[k].kind = (unsigned char)(k * 7);
        units[k].owner = (unsigned char)(k ^ 0x55);
        units[k].x = (unsigned char)(k % 5);
        units[k].y = (unsigned char)(k % 4);
        units[k].work = (unsigned char)(k + 1);
        units[k].kind %= 5;
    }
    for (k = 0; k < 255; k++) { odds[k].b = k * 257; odds[k].c[1] = (unsigned char)(k + 3); }
    for (x = 0; x < 5; x++)
        for (y = 0; y < 4; y++)
            if (count_at(x, y) != count_ref(x, y)) res[0]++;
    res[1] = index_sum() != index_sum_ref();
    units[0].where = 1; units[1].where = 1; units[2].where = 0;
    res[2] = first_free() != 2 || !(units[2].flags & 0x80);
    units[0].where = 0;
    res[3] = first_free() != 0;
    res[4] = pairs() != pairs_ref();
    res[5] = odd_sum(0) != odd_sum_ref(0) || odd_sum(200) != odd_sum_ref(200) || odd_sum(255) != 0 ||
             odd_sum(254) != odd_sum_ref(254);
    for (k = 0; k < NCHECK; k++) OUT[k] = res[k];
    OUT[NCHECK] = 0xAA;
    return 0;
}
