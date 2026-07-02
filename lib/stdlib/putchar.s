; putchar.s — Write a character to screen via KERNAL CHROUT
;
; int putchar(int c);
;   Writes the low byte of c to the screen using KERNAL $FFD2.
;   Returns c.
;
; The kernal clobbers A, X, Y, Z and flags. We save/restore Z
; because the compiler uses it as a temp register (TAZ/TZA pattern).

.global _putchar
.extern __sp_base

.segment "code"

proc _putchar, W#_p_c
    .var _fp = 0
    lda.sp _p_c
    phz
    jsr $FFD2
    plz
    ldax _p_c, sp
    rtn #0
    endproc
