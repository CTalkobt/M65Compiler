/* sacovl/o1.c — overlay o1 */
#pragma cc45 overlay(o1)

unsigned int r_mid(unsigned char y);

/* its array must survive the call into overlay o2 through r_mid */
unsigned int o1_f(unsigned char x) {
    unsigned char buf[16];
    unsigned char i;
    unsigned int s = 0, m;
    for (i = 0; i < 16; i++) buf[i] = x;
    m = r_mid(7);
    for (i = 0; i < 16; i++) s += buf[i];
    return s + m;
}

/* never active while o2_k is: may share storage with it */
unsigned int o1_h(unsigned char x) {
    unsigned char t[12];
    unsigned char i;
    unsigned int s = 0;
    for (i = 0; i < 12; i++) t[i] = x;
    for (i = 0; i < 12; i++) s += t[i];
    return s;
}
