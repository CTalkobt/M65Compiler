; exit.s — Terminate program (ZP calling convention)
;
; _Noreturn void exit(int status);
;   Jumps to __exit with status in .AX.

.global _exit
.extern __exit

.segment "code"

proc _exit
    .zp_uses $03, $04
    .zp_clobbers $03, $04
    .zp_release $03, $04
    .reg_clobbers A, X

    lda $03
    ldx $04
    jmp __exit
    endproc
