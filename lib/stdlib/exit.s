; exit.s — Terminate program
;
; _Noreturn void exit(int status);
;   Jumps to __exit, which terminates the program using the
;   configured exit mode (rts/halt/brk). The status code is
;   passed in .AX (low byte in A, high byte in X).

.global _exit
.extern __exit

.segment "code"

proc _exit, W#_p_status
    .var _fp = 0
    ldax _p_status, s
    jmp __exit
    endproc
