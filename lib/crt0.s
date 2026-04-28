; crt0.s — Default C Runtime Startup for cc45
;
; This is the entry point for all C programs. The compiler's startup stub
; jumps to __init, which:
;   1. Calls _init_features (weak — overridable for platform setup)
;   2. Tail-calls _main (so main's return value in .AX propagates to caller)
;
; To override the init hook, provide a strong _init_features in another
; object file (e.g., crt0_mega65.s) and link it before or after crt0.
;
; Usage (relocatable):
;   ln45 -prg crt0.o45 main.o45 -o program.prg
;
; Usage (direct compile):
;   The compiler automatically emits "jsr __init" and includes an inline crt0.

.global __init
.weak _init_features
.extern _main

.segment "code"

__init:
    jsr _init_features
    jmp _main           ; tail-call: main's RTS returns to our caller with .AX

; Default init_features — does nothing. Override with a strong definition
; to set up hardware (16-bit stack, I/O mapping, DMA, etc.)
_init_features:
    rts
