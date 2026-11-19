// #659: a bit-field updated in place (f.mode++, --f.mode, f.mode += 2, through
// a pointer; a local and a global) and then read gives the new value. The
// update writes the field's storage unit in memory (BFINS, with no store
// after it for ++ / --), which CSE took for an instruction that writes
// nothing: the read after it reused the value read before.
// Results at $C0C0: 01 per check, $AA sentinel.
struct flags { unsigned char active : 1; unsigned char mode : 3; unsigned char prio : 4; };
struct wide { unsigned count : 10; unsigned chan : 6; };

struct flags g;
struct wide gw;
volatile unsigned char vz = 0;

static unsigned char t_local(void) {
    struct flags f;
    f.active = 1; f.mode = 5; f.prio = 9;
    unsigned char a = f.mode;
    f.mode++;
    unsigned char b = f.mode;
    f.mode--;
    unsigned char c = f.mode;
    ++f.mode;
    unsigned char d = f.mode;
    --f.mode;
    --f.mode;
    unsigned char e = f.mode;
    return a == 5 && b == 6 && c == 5 && d == 6 && e == 4 && f.prio == 9 && f.active == 1;
}

static unsigned char t_global(void) {
    g.active = 0; g.mode = 2; g.prio = 15;
    unsigned char a = g.mode;
    g.mode++;
    unsigned char b = g.mode;
    g.mode--;
    g.mode--;
    unsigned char c = g.mode;
    return a == 2 && b == 3 && c == 1 && g.prio == 15 && g.active == 0;
}

static unsigned char t_postfix_value(void) {
    struct flags f;
    f.mode = 3;
    unsigned char a = f.mode++;      // the value before
    unsigned char b = f.mode;
    unsigned char c = f.mode--;
    unsigned char d = f.mode;
    return a == 3 && b == 4 && c == 4 && d == 3;
}

static unsigned char t_compound(void) {
    struct flags f;
    f.mode = 1; f.prio = 2;
    unsigned char a = f.mode;
    f.mode += 3;
    unsigned char b = f.mode;
    f.prio |= 8;
    f.mode -= 2;
    unsigned char c = f.mode;
    g.mode = 1;
    unsigned char d = g.mode;
    g.mode <<= 2;
    return a == 1 && b == 4 && c == 2 && f.prio == 10 && d == 1 && g.mode == 4;
}

static unsigned char t_pointer(void) {
    struct flags f;
    struct flags *p = vz ? &g : &f;
    f.mode = 6;
    unsigned char a = f.mode;
    p->mode++;
    unsigned char b = f.mode;
    p->mode--;
    p->mode--;
    unsigned char c = p->mode;
    return a == 6 && b == 7 && c == 5 && f.mode == 5;
}

static unsigned char t_wide(void) {
    struct wide w;
    w.count = 500; w.chan = 30;
    unsigned a = w.count;
    w.count++;
    unsigned b = w.count;
    gw.count = 1023; gw.chan = 1;
    gw.count--;
    unsigned c = gw.count;
    return a == 500 && b == 501 && w.chan == 30 && c == 1022 && gw.chan == 1;
}

// the field changed in one branch, read after the if
static unsigned char t_join(void) {
    struct flags f;
    f.mode = 1;
    unsigned char a = f.mode;
    if (vz == 0) f.mode++;
    unsigned char b = f.mode;
    if (vz == 0) g.mode--;
    return a == 1 && b == 2;
}

int main(void) {
    unsigned char *o = (unsigned char *)0xC0C0;
    o[0] = t_local();
    o[1] = t_global();
    o[2] = t_postfix_value();
    o[3] = t_compound();
    o[4] = t_pointer();
    o[5] = t_wide();
    o[6] = t_join();
    o[7] = 0xAA;
    return 0;
}
