/* test_pcode_xemu.c — research/pcode: the interpreter (pcode_vm.s45, version
 * 2; version 1 pcode_vm_v1.s45) running hand-written p-code
 * (pcode_progs.s45) in xemu.
 * $C0C0: [0] sum of 1..5 = 15, version 1's instructions, on version 1
 *        [1] the same program on version 2
 *        [2] bytes: dst = src + 7 with a byte wrap (version 2)
 *        [3] byte compare and branch, both ways (version 2)
 *        [4] sum with the combined instructions, the test at the top
 *        [5] sum with the combined instructions, rotated
 *        [6] ADV and INC (byte, word with a carry, a byte wrapping alone)
 *        [7] LDE of a byte
 *        [8] $AA */
extern unsigned pcode_run(const unsigned char *code, void *frame);
extern unsigned pcode_run1(const unsigned char *code, void *frame);
extern const unsigned char pc_sum[], pc_bytes[], pc_cmp[], pc_sum2[], pc_sum3[], pc_adv[], pc_lde[];

#define R ((volatile unsigned char *)0xC0C0)

static unsigned arr[5] = {1, 2, 3, 4, 5};
static unsigned frame[8];
static unsigned char src[4] = {10, 20, 30, 250}, dst[4];
static unsigned char bf[16];

static void sumFrame(void) { frame[0] = (unsigned)arr; frame[1] = 5; frame[2] = frame[3] = 99; }

int main(void) {
    sumFrame();
    R[0] = pcode_run1(pc_sum, frame) == 15 && frame[2] == 15 && frame[3] == 5;
    sumFrame();
    R[1] = pcode_run(pc_sum, frame) == 15 && frame[2] == 15 && frame[3] == 5;

    *(unsigned *)&bf[0] = (unsigned)src;
    *(unsigned *)&bf[2] = (unsigned)dst;
    *(unsigned *)&bf[4] = 4;
    bf[14] = 7;
    R[2] = pcode_run(pc_bytes, bf) == 1 && dst[0] == 17 && dst[1] == 27 && dst[2] == 37 && dst[3] == 1;
    R[3] = pcode_run(pc_cmp, bf) == 200;

    sumFrame();
    R[4] = pcode_run(pc_sum2, frame) == 15 && frame[3] == 5;
    sumFrame();
    R[5] = pcode_run(pc_sum3, frame) == 15 && frame[3] == 5;

    bf[5] = 0x77;
    R[6] = pcode_run(pc_adv, bf) == 0x0100 && bf[0] == 6 && bf[4] == 0 && bf[5] == 0x77;

    *(unsigned *)&bf[0] = (unsigned)src;
    R[7] = pcode_run(pc_lde, bf) == 30;
    R[8] = 0xAA;
    return 0;
}
