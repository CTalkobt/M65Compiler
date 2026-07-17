; exit.s — Normal program termination (ZP calling convention)
;
; _Noreturn void exit(int status);
;   Weak function — can be overridden for pre-exit hooks.
;
; _Noreturn void _exit(int status);
;   Strong function — always calls __exit directly.

.weak _exit
.global _exit
.global __exit_
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

proc __exit_
    .zp_uses $03, $04
    .zp_clobbers $03, $04
    .zp_release $03, $04
    .reg_clobbers A, X

    lda $03
    ldx $04
    jmp __exit
    endproc
