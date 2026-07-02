; crt0_zp.s — Default C Runtime Startup for cc45 (ZP calling convention)
;
; Entry point for all C programs compiled with -fzpcall.
; Same as crt0.s — main takes no parameters via ZP.

.global __init
.global __exit
.global __sp_base
.global __zp_save_buf
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

    ; Enable MEGA65 I/O — GS knock must precede hardware register access
    lda #$47            ; 'G'
    sta $D02F
    lda #$53            ; 'S'
    sta $D02F

    ; Save ZP $08-$FF to BSS buffer
    ldx #0
@__zp_save_loop:
    lda $08,x
    sta __zp_save_buf,x
    inx
    cpx #248
    bne @__zp_save_loop

    jsr _init_features
    jsr _main

    ; Fall through to __exit
__exit:
    ; Restore ZP $08-$FF from BSS buffer
    ldx #0
@__zp_restore_loop:
    lda __zp_save_buf,x
    sta $08,x
    inx
    cpx #248
    bne @__zp_restore_loop

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
