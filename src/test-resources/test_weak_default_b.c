/* test_weak_default_b.c — the strong definitions test_weak_default_xemu.c's
 * weak defaults give way to */
unsigned int weak_default_sink;
void weak_default_touch(unsigned int v);

unsigned char w_small(void) { return 2; }

/* (other names, a local and a call: its own static storage) */
unsigned int w_params(unsigned int x, unsigned char y) {
    unsigned int t = x - y;
    weak_default_touch(t);
    return t;
}

unsigned char w_const(unsigned char k) { return k * 3; }

void weak_default_touch(unsigned int v) { weak_default_sink = v; }
