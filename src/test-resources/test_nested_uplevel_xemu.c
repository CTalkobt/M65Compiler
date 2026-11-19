// #660: a nested function reaching its parent's variables (ADDR_UPLEVEL)
// named the parent's variable as a vreg of the nested function: a vreg of
// the same number in the nested function (its static link, a pointer) got
// the parent variable's type. A char there: the static link allocated one
// byte, its high byte lost, the parent's variable read from page 0; or a
// pointer the nested function computed stored without its high byte.
// The parent's first locals are chars, so their vreg numbers are those of
// the nested functions' static link and first temporaries.
// Results at $C0C0: 01 per check, $AA sentinel.
static struct { unsigned char a, b, ok; } cs;
volatile unsigned char vz = 0;

static unsigned char t_read(void) {
    unsigned char pos = 3, more = 4;
    unsigned char got = 0;
    void h(void) { if (cs.ok) got = pos + more; }
    cs.ok = 1;
    h();
    return got == 7;
}

static unsigned char t_write(void) {
    unsigned char pos = 1, n = 0;
    void bump(unsigned char k) { if (cs.ok) { pos += k; n++; } }
    cs.ok = 1;
    bump(2); bump(3);
    return pos == 6 && n == 2;
}

static unsigned char t_chain(void) {
    unsigned char depth = 5;
    unsigned char inner(void) {
        unsigned char extra = 1;
        unsigned char deepest(void) { return depth + extra + cs.b; }
        return deepest();
    }
    cs.b = 10;
    return inner() == 16;
}

static unsigned char t_member(void) {
    unsigned char pos = 0;
    unsigned char fail_at = 0;
    void fail(unsigned char why) { if (cs.ok) { fail_at = pos + why; } cs.ok = 0; }
    cs.ok = 1;
    pos = 8;
    fail(1);
    fail(2);
    return fail_at == 9 && cs.ok == 0;
}

int main(void) {
    unsigned char *o = (unsigned char *)0xC0C0;
    o[0] = t_read();
    o[1] = t_write();
    o[2] = t_chain();
    o[3] = t_member();
    o[4] = 0xAA;
    return 0;
}
