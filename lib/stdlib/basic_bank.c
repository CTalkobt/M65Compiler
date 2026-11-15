/* basic_bank.c — basic45 --compile --bank=N (#580): the arrays and the string
 * variables of a compiled BASIC 65 program in bank N (1, 4 or 5), not on the
 * program's heap in bank 0
 *
 * The compiled program defines the bank and its range (__bas_fbank,
 * __bas_fstart, __bas_fend: the first byte, past the last). A value there is
 * named by its 16-bit offset in the bank; 0 is none (a string variable: the
 * empty string; an array variable: no array yet). The memory is a heap of
 * blocks, each a size word (the block's bytes, with the word) and its data;
 * a free block's data starts with the next free block's offset (the list in
 * address order, neighbours merged).
 *
 * An array's block: the number of dimensions (1 byte), each dimension's size
 * (its DIM + 1, 2 bytes), then the elements (floats 5 bytes, integers 2,
 * strings a 2-byte offset), the first subscript varying fastest: as in bank 0
 * (basic_aref.s45). A string's block: its length byte and characters.
 *
 * Arguments in the run time's variables (basic_rt.s45), as basic_str.c's.
 */
#include <string.h>

extern unsigned char _bas_fbank;
extern unsigned int _bas_fstart, _bas_fend;
extern unsigned char *_bas_sa, *_bas_sr;
extern int _bas_n, _bas_m;
extern unsigned int _bas_sub[8];
extern void _bas_error(void);

typedef unsigned char __far *fbp;
typedef unsigned int __far *fwp;

/* an element between the bank and the program (basic_bankx.s45) */
unsigned char _bas_ecell[5];

static unsigned int freep;      /* the first free block (0: none) */
static unsigned char ready;

static void raise(int n) {
    _bas_n = n;
    _bas_error();
}

static fbp at(unsigned int off) {
    return (fbp)(((unsigned long)_bas_fbank << 16) | off);
}

static unsigned int rdw(unsigned int off) {
    return *(fwp)at(off);
}

static void wrw(unsigned int off, unsigned int v) {
    *(fwp)at(off) = v;
}

/* n bytes: their offset (0: no room) */
static unsigned int falloc(unsigned int n) {
    unsigned int need, prev = 0, cur, size, next;
    if (!ready) {
        ready = 1;
        freep = _bas_fstart;
        wrw(freep, _bas_fend - _bas_fstart);
        wrw(freep + 2, 0);
    }
    if (n > 0xFFF0) return 0;
    need = n + 2;
    if (need < 4) need = 4;
    for (cur = freep; cur; prev = cur, cur = next) {
        size = rdw(cur);
        next = rdw(cur + 2);
        if (size < need) continue;
        if (size - need >= 4) {
            /* (the block's end taken: the list stays as it is) */
            size -= need;
            wrw(cur, size);
            cur += size;
            wrw(cur, need);
        } else if (prev) wrw(prev + 2, next);
        else freep = next;
        return cur + 2;
    }
    return 0;
}

static void ffree(unsigned int p) {
    unsigned int b, prev = 0, cur = freep, size;
    if (!p) return;
    b = p - 2;
    while (cur && cur < b) {
        prev = cur;
        cur = rdw(cur + 2);
    }
    size = rdw(b);
    if (cur && b + size == cur) {
        size += rdw(cur);
        cur = rdw(cur + 2);
    }
    if (prev && prev + rdw(prev) == b) {
        wrw(prev, rdw(prev) + size);
        wrw(prev + 2, cur);
        return;
    }
    wrw(b, size);
    wrw(b + 2, cur);
    if (prev) wrw(prev + 2, b);
    else freep = b;
}

/* The element of the array of the variable __bas_sa: __bas_n subscripts (in
 * __bas_sub), __bas_m the element size | $80 (a DIM statement's) | $40
 * (strings). __bas_n := the element's offset; DIM: the array made (an array
 * used before a DIM gets 11 elements in each dimension). ?REDIMENSIONED
 * ARRAY, ?BAD SUBSCRIPT, ?OUT OF MEMORY as the ROM's. */
void _bas_areff(void) {
    unsigned int *var = (unsigned int *)_bas_sa;
    unsigned char ns = (unsigned char)_bas_n, es = (unsigned char)_bas_m & 0x3F, k;
    unsigned char dim = (unsigned char)_bas_m & 0x80;
    unsigned int a = *var, off, d, hdr = 1 + 2 * ns;
    if (a && dim) raise(19);
    if (!a) {
        unsigned long bytes = es;
        unsigned int i, n;
        fbp p;
        for (k = 0; k < ns; k++) {
            d = dim ? _bas_sub[k] + 1 : 11;
            if (!d) raise(16);
            bytes *= d;
            if (bytes > 0xFF00UL) raise(16);
        }
        n = (unsigned int)bytes;
        a = falloc(n + hdr);
        if (!a) raise(16);
        p = at(a);
        p[0] = ns;
        for (k = 0; k < ns; k++) wrw(a + 1 + 2 * k, dim ? _bas_sub[k] + 1 : 11);
        /* (numbers 0, strings the empty one: all 0) */
        p += hdr;
        for (i = 0; i < n; i++) p[i] = 0;
        *var = a;
        if (dim) return;
    }
    if (*at(a) != ns) raise(18);
    off = 0;
    for (k = ns; k-- > 0;) {
        d = rdw(a + 1 + 2 * k);
        if (_bas_sub[k] >= d) raise(18);
        off = off * d + _bas_sub[k];
    }
    _bas_n = (int)(a + hdr + off * es);
}

/* the buffer __bas_sr := the string at __bas_n (0: the empty one) */
void _bas_sfetch(void) {
    unsigned char *r = _bas_sr;
    unsigned char k;
    fbp p;
    if (!_bas_n) {
        r[0] = 0;
        return;
    }
    p = at((unsigned int)_bas_n);
    k = p[0];
    r[0] = k;
    while (k) {
        r[k] = p[k];
        k--;
    }
}

/* __bas_n := a copy in the bank of the string __bas_sa (the empty one: 0);
 * the string at __bas_n (a variable's old value) freed */
void _bas_sstore(void) {
    unsigned char *s = _bas_sa;
    unsigned int old = (unsigned int)_bas_n, a = 0;
    unsigned char k = s[0];
    /* (freed first: its room is the new one's, s being in bank 0) */
    ffree(old);
    if (k) {
        fbp p;
        a = falloc(k + 1);
        if (!a) {
            _bas_n = 0;
            raise(16);
        }
        p = at(a);
        p[0] = k;
        while (k) {
            p[k] = s[k];
            k--;
        }
    }
    _bas_n = (int)a;
}

/* CLR: the string at __bas_n freed */
void _bas_sfree(void) {
    ffree((unsigned int)_bas_n);
}

/* CLR's arrays: the array of the variable __bas_sa freed (a string array's
 * strings too: __bas_m 1) and undimensioned */
void _bas_afreef(void) {
    unsigned int *var = (unsigned int *)_bas_sa;
    unsigned int a = *var;
    if (!a) return;
    if (_bas_m) {
        unsigned char ns = *at(a), k;
        unsigned int e = a + 1 + 2 * ns, i;
        unsigned long count = 1;
        for (k = 0; k < ns; k++) count *= rdw(a + 1 + 2 * k);
        for (i = 0; i < (unsigned int)count; i++) ffree(rdw(e + 2 * i));
    }
    ffree(a);
    *var = 0;
}
