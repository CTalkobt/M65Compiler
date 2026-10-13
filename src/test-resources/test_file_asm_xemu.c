/* test_file_asm_xemu.c — asm("...") at file scope
 *
 * File-scope asm text goes to the assembler at module level (it used to be
 * dropped): data, equates and whole functions defined in assembly, used
 * from C.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] a data table defined in file-scope asm, read from C
 *   [1] a function written in file-scope asm, called from C
 *   [2] asm code that updates a C global
 *   [3] an equate (its address seen from C)
 *   [4] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

int counter = 40;

asm("    .segment \"data\"\n"
    "    .global _tbl\n"
    "_tbl: .byte 7, 9, 11\n");

asm(".global _asm_seven\n"
    "_asm_seven:\n"
    "    lda #7\n"
    "    ldx #0\n"
    "    rts\n");

asm(".global _bump\n"
    "_bump:\n"
    "    inc _counter\n"
    "    inc _counter\n"
    "    rts\n");

asm(".global _magic\n_magic = $1234");

extern unsigned char tbl[3];
extern char magic[];
int asm_seven(void);
void bump(void);

int main(void) {
    OUT[4] = 0xAA;
    OUT[0] = tbl[0] == 7 && tbl[1] == 9 && tbl[2] == 11;
    OUT[1] = asm_seven() == 7;
    bump();
    OUT[2] = counter == 42;
    OUT[3] = (unsigned)magic == 0x1234;
    return 0;
}
