/* test_float_ptr_xemu.c — floats read and written through a pointer by calls
 * (__f32ldp / __f32stp, #369)
 *
 * A float read through a pointer into __float_a (the first operand of the
 * next float operation) is jsr __f32ldp with the pointer in A/X; a float
 * result written through one from __float_a is jsr __f32stp. A file uses
 * each when two or more of its kind have the pointer just computed (array
 * elements at a variable index), then for every one of that kind.
 * Each check writes its number at $C0C0 + i; $AA after the last.
 *
 * Expected at $C0C0: 01 02 03 04 05 06 07 AA
 */

#define RESULT(i) (*(volatile unsigned char *)(0xC0C0 + (i)))

float arr[8] = { 1.0, 2.5, 4.0, 8.0, -3.0, 0.5, 10.0, 6.0 };
float out[8];
float *fp, *gp;
float c = 0.25, d = 2.0;
volatile int ix, jx;
signed int iv;

int main(void) {
    int i;
    float r;
    for (i = 0; i < 8; i++) RESULT(i) = 0;
    ix = 2; jx = 5;
    /* 1, 2: loads through a just-computed pointer, the first operand */
    r = arr[ix] + c;                        /* 4.25 */
    if (r == 4.25) RESULT(0) = 0x01;
    r = arr[jx] * d;                        /* 1.0 */
    if (r == 1.0) RESULT(1) = 0x02;
    /* 3, 4: results stored through a just-computed pointer */
    out[ix] = arr[ix + 1] - c;              /* 7.75 */
    out[jx] = arr[jx - 1] * d;              /* -6.0 */
    if (out[2] == 7.75 && out[5] == -6.0) RESULT(2) = 0x03;
    iv = -9;
    out[ix + 1] = iv;                       /* -9.0 (an int converted) */
    out[jx + 1] = iv * 2;                   /* -18.0 */
    if (out[3] == -9.0 && out[6] == -18.0) RESULT(3) = 0x04;
    /* 5: through pointer variables (the pointer loaded first) */
    fp = &arr[6]; gp = &out[7];
    *gp = *fp + d;                          /* 12.0 */
    if (out[7] == 12.0) RESULT(4) = 0x05;
    /* 6: in a loop, each element through a pointer */
    for (i = 0; i < 8; i++) out[i] = arr[i] * d + c;
    if (out[0] == 2.25 && out[4] == -5.75 && out[7] == 12.25) RESULT(5) = 0x06;
    /* 7: the neighbours unchanged (5 bytes written, not more) */
    out[1] = 0.0; out[2] = 0.0; out[3] = 0.0;
    ix = 2;
    out[ix] = arr[0] + d;                   /* 3.0 */
    if (out[1] == 0.0 && out[2] == 3.0 && out[3] == 0.0) RESULT(6) = 0x07;
    RESULT(7) = 0xAA;
    return 0;
}
