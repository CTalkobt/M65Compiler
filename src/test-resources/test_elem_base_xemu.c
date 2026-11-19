// #658: an element of an array that is a member of a global struct or union
// (g.s.arr[i]) is the element of the global at the member's offset
// (sta _g+100,y for a small one), not of a pointer &g + 100 computed first
// on every pass of a loop. Extern and file-local globals, byte and 16-bit
// elements, arrays of structs, an array past 256 bytes, a nested member.
// Results at $C0C0: 01 per check, $AA sentinel.
union area { unsigned char m[256]; struct { unsigned char pad[100]; unsigned char rows[50]; unsigned w[20]; } r; };
extern union area area;
unsigned char eb_byte(unsigned i);

struct item { unsigned char a; unsigned b; };
struct shelf { unsigned char tag; struct item items[10]; unsigned char after; };
static struct shelf sh;

struct big { unsigned char head[3]; unsigned char data[300]; unsigned char tail; };
static struct big bg;

struct inner { unsigned char x[8]; };
struct outer { unsigned char p; struct inner in[4]; };
static struct outer ou;

__zp struct { unsigned char lo; unsigned char v[6]; } zs;

volatile unsigned char vz = 0;

static void clear_rows(void) { unsigned char r; for (r = 0; r < 50; r++) area.r.rows[r] = 0; }
static void fill_rows(unsigned char v) { unsigned char r; for (r = 0; r < 50; r++) area.r.rows[r] = v + r; }

static unsigned char t_fill(void) {
    unsigned i;
    for (i = 0; i < 256; i++) area.m[i] = 0xEE;
    clear_rows();
    for (i = 100; i < 150; i++) if (eb_byte(i) != 0) return 0;
    // what is around it is untouched
    return eb_byte(99) == 0xEE && eb_byte(150) == 0xEE;
}

static unsigned char t_read(void) {
    unsigned char r, s = 0;
    fill_rows(10);
    for (r = 0; r < 50; r++) s += area.r.rows[r];
    // 10 * 50 + 0 + 1 + ... + 49 = 500 + 1225 = 1725 = $6BD: $BD in a byte
    return s == 0xBD && eb_byte(100) == 10 && eb_byte(149) == 59;
}

static unsigned char t_words(void) {
    unsigned char i;
    unsigned s = 0;
    for (i = 0; i < 20; i++) area.r.w[i] = 1000u + i;
    for (i = 0; i < 20; i++) s += area.r.w[i];
    return s == 20000u + 190 && eb_byte(150) == (unsigned char)1000 && eb_byte(151) == 1000 >> 8;
}

static unsigned char t_structs(void) {
    unsigned char i;
    unsigned s = 0;
    sh.tag = 7; sh.after = 9;
    for (i = 0; i < 10; i++) { sh.items[i].a = i; sh.items[i].b = 100u * i; }
    for (i = 0; i < 10; i++) s += sh.items[i].a + sh.items[i].b;
    return s == 45 + 4500 && sh.tag == 7 && sh.after == 9;
}

static unsigned char t_big(void) {
    unsigned i;
    unsigned char s = 0;
    bg.head[2] = 1; bg.tail = 2;
    for (i = 0; i < 300; i++) bg.data[i] = (unsigned char)i;
    for (i = 250; i < 300; i++) s ^= bg.data[i];
    unsigned char want = 0;
    for (i = 250; i < 300; i++) want ^= (unsigned char)i;
    return s == want && bg.data[299] == (unsigned char)299 && bg.head[2] == 1 && bg.tail == 2;
}

static unsigned char t_nested(void) {
    unsigned char i, j;
    unsigned s = 0;
    ou.p = 5;
    for (i = 0; i < 4; i++) for (j = 0; j < 8; j++) ou.in[i].x[j] = i * 8 + j;
    for (j = 0; j < 8; j++) s += ou.in[2].x[j];
    unsigned char k = vz + 3;
    return s == 16 * 8 + 28 && ou.in[k].x[7] == 31 && ou.p == 5;
}

static unsigned char t_zp(void) {
    unsigned char i, s = 0;
    zs.lo = 1;
    for (i = 0; i < 6; i++) zs.v[i] = i + 1;
    for (i = 0; i < 6; i++) s += zs.v[i];
    return s == 21 && zs.lo == 1;
}

// a variable index read from memory each time, and one that goes down
static unsigned char t_index(void) {
    unsigned char r;
    fill_rows(0);
    for (r = 50; r-- > 0;) area.r.rows[r] = 49 - r;
    unsigned char i = vz + 49;
    return area.r.rows[i] == 0 && area.r.rows[vz] == 49;
}

int main(void) {
    unsigned char *o = (unsigned char *)0xC0C0;
    o[0] = t_fill();
    o[1] = t_read();
    o[2] = t_words();
    o[3] = t_structs();
    o[4] = t_big();
    o[5] = t_nested();
    o[6] = t_zp();
    o[7] = t_index();
    o[8] = 0xAA;
    return 0;
}
