// #614: the float runtime maps the BASIC ROM at $4000-$7FFF itself for each
// ROM call and puts the map it found back. Checked where that matters: in a
// __far function, which runs in a mapped overlay in the same window (the
// code after the float call must still be there), with floats before and
// after in resident code, and from a second __far function calling the first.
// Results at $C0C0: 01 per check, $AA sentinel.
#include <math.h>
#include <stdio.h>
#include <string.h>

volatile float two = 2.0f, nine = 9.0f, half = 0.5f;

static unsigned char near_(float v, float want) {
    float d = v - want;
    if (d < 0) d = -d;
    return d < 0.001f;
}

// in the mapped window: ROM calls (sqrt, sin, the arithmetic without
// -ffast-float) between its own instructions
__far float far_calc(float x) {
    float r = sqrtf(x);          // 3
    r = r * two + half;          // 6.5
    r += sinf(0.0f);             // 6.5
    return r / half;             // 13
}

__far unsigned char far_outer(void) {
    char buf[16];
    float v = far_calc(nine);
    unsigned char ok = near_(v, 13.0f);
    sprintf(buf, "%.2f", v);     // FOUT, from the window
    return ok && strcmp(buf, "13.00") == 0;
}

int main(void) {
    unsigned char *o = (unsigned char *)0xC0C0;
    float a = two * nine;                    // resident, before
    o[0] = near_(a, 18.0f);
    o[1] = near_(far_calc(nine), 13.0f);     // the far function returns here
    o[2] = far_outer();                      // far calling far
    o[3] = near_(sqrtf(nine) + expf(0.0f), 4.0f);   // resident, after
    o[4] = near_(far_calc(two * two), 9.0f); // and again
    o[5] = 0xAA;
    return 0;
}
