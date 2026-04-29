; putchar.s — Write a character to screen via KERNAL CHROUT
;
; int putchar(int c);
;   Writes the low byte of c to the screen using KERNAL $FFD2.
;   Returns c.

.global _putchar

.segment "code"

proc _putchar, W#_p_c
    .var _fp = 0
    lda.sp _p_c
    jsr $FFD2
    ldax _p_c, s
    rtn #0
    endproc
