; exit.s — Normal program termination
;
; _Noreturn void exit(int status);
;   Weak function — can be overridden by user code for pre-exit hooks
;   (atexit handlers, cleanup). Default calls __exit directly.
;
; _Noreturn void _exit(int status);
;   Strong function — always calls __exit. Cannot be overridden.
;   Use _exit() when you want guaranteed immediate termination.

.weak _exit
.global _exit
.global __exit_
.extern __exit
.extern __sp_base

.segment "code"

proc _exit, W#_p_status
    .var _fp = 0
    ldax _p_status, sp
    jmp __exit
    endproc

; _exit (POSIX-style) — strong, non-overridable
proc __exit_, W#_p_status
    .var _fp = 0
    ldax _p_status, sp
    jmp __exit
    endproc
