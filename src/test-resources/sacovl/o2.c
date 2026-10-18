/* sacovl/o2.c — overlay o2 */
#pragma cc45 overlay(o2)

/* runs while o1_f is active (o1_f -> r_mid -> o2_g): its locals must not
 * overlap o1_f's */
unsigned int o2_g(unsigned char y) {
    unsigned char w[16];
    unsigned char i;
    unsigned int s = 5;
    for (i = 0; i < 16; i++) w[i] = y;
    for (i = 0; i < 16; i++) s += w[i];
    return s;
}

/* never active while o1_h is */
unsigned int o2_k(unsigned char y) {
    unsigned char t[12];
    unsigned char i;
    unsigned int s = 1;
    for (i = 0; i < 12; i++) t[i] = y;
    for (i = 0; i < 12; i++) s += t[i];
    return s;
}
