; abort.s — Abnormal program termination
;
; _Noreturn void abort(void);
;   Weak function — can be overridden by user code for pre-abort hooks.
;   Default implementation calls __abort (BRK).

.weak _abort
.global _abort
.extern __abort

.segment "code"

_abort:
    jmp __abort
