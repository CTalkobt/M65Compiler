; crt0_zp.s — Default C Runtime Startup for cc45 (ZP calling convention)
;
; Entry point for all C programs compiled with -fzpcall.
; Same as crt0.s — main takes no parameters via ZP.

.global __init
.global __exit
.global __sp_base
.weak _init_features
.extern _main

.segment "init"

__sp_base = $0101

__init:
    ; Save caller's stack pointer for clean return
    tsx
    stx __saved_spl + 1
    tsy
    sty __saved_sph + 1

    ; Save ZP $08-$FF to BSS buffer (preserve KERNAL/BASIC state)
    ldx #248
    ldy #0
    move $08, __zp_save_buf

    jsr _init_features
    jsr _main

    ; Fall through to __exit
__exit:
    ; Restore ZP $08-$FF from BSS buffer
    ldx #248
    ldy #0
    move __zp_save_buf, $08

    ; Restore caller's stack pointer and return.
    ; Clear Z register — the MEGA65 kernal/BASIC may use Z as part of
    ; the return address banking; a stale Z causes PC corruption on RTS.
    ldz #$00
__saved_spl:
    ldx #$FF
    txs
__saved_sph:
    ldy #$01
    tys
    rts

; Default init_features — does nothing. Override with a strong definition.
_init_features:
    rts

.segment "bss"
__zp_save_buf:
    .res 248
