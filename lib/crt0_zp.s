; crt0_zp.s — Default C Runtime Startup for cc45 (ZP calling convention)
;
; Entry point for all C programs compiled with -fzpcall.
; Same as crt0.s — main takes no parameters via ZP.

.global __init
.global __sp_base
.weak _init_features
.extern _main

.segment "code"

__sp_base = $0101

__init:
    jsr _init_features
    jmp _main           ; tail-call: main's RTS returns to our caller with .AX

; Default init_features — does nothing. Override with a strong definition.
_init_features:
    rts
