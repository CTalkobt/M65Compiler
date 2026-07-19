; exit.s — Normal program termination with atexit handler support
;
; _Noreturn void exit(int status);
;   Calls registered atexit() handlers in reverse order, then terminates.
;
; _Noreturn void _exit(int status);
;   Immediate termination without calling atexit handlers.

.global _exit
.global __exit_
.extern __exit
.extern ___atexit_run
.extern __sp_base

.segment "code"

; exit() — call atexit handlers, then terminate
proc _exit, W#_p_status
    .var _fp = 0
    jsr ___atexit_run
    ldax _p_status, sp
    jmp __exit
    endproc

; _exit() — immediate termination (no atexit)
proc __exit_, W#_p_status
    .var _fp = 0
    ldax _p_status, sp
    jmp __exit
    endproc
