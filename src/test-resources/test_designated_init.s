    .o45
    .org $2000
    .weak __sp_base
    __sp_base = $0101
    .weak __static_chain
    .weak __zp_scratch
    .weak __zp_scratch2
    .weak __zp_scratch3
    .weak __zp_scratch4
    .weak cc45.zeroPageStart
    __static_chain = $06
    __zp_scratch = $08
    __zp_scratch2 = $0A
    __zp_scratch3 = $0C
    __zp_scratch4 = $0E
    cc45.zeroPageStart = $08

    .global _origin
    .global _lookup
    .global _main

    .segment "data"
    .byte 0
_origin:
; .debug_var: @global _origin offset=0 size=4 type=int32 scope=global
    .dword 0
    .dword 0
_lookup:
; .debug_var: @global _lookup offset=0 size=2 type=int16 scope=global
    .word 10
    .word 30
    .word 50
    .res 4

    .segment "code"

; function _main
; SAC inline storage: 25 bytes
    _main__local_0: .long 0
    _main__local_3: .word 0
    _main__local_6: .word 0
    _main__local_9: .word 0
    _main__local_12: .word 0
    _main__local_18: .word 0
    _main__local_26: .word 0
    _main__local_28: .word 0
    _main__local_35: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_designated_init.c", 19
    .local @_l_arr = 9
    .local @_l_c = 2
    .local @_l_mix = 17
    .local @_l_p = 5
    .local @_l_result = 0
; .debug_var: __main @_l_arr offset=9 size=2 type=int16 scope=local
; .debug_var: __main @_l_c offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_mix offset=17 size=2 type=int16 scope=local
; .debug_var: __main @_l_p offset=5 size=4 type=int32 scope=local
; .debug_var: __main @_l_result offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_designated_init.c", 21
    leax.local 5
    sta $20
    stx $21
    lda #20
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    ldy #0
    sta ($20),y
    txa
    iny
    sta ($20),y
    lda #10
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    add.16 .AX, #2
    sta $24
    stx $25
    lda $22
    ldx $23
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_designated_init.c", 24
    leax.local 2
    sta $20
    stx $21
    lda #255
    ldx #0
    sta $22
    stx $23
    lda $22
    ldy #0
    sta ($20),y
    lda #128
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    add.16 .AX, #1
    sta $24
    stx $25
    lda $22
    ldy #0
    sta ($24),y
    .loc "test_designated_init.c", 27
    leax.local 9
    sta $20
    stx $21
    lda #100
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    pha
    phx
    struct_elem.16 __zp_scratch, $20, #0
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    lda #44
    ldx #1
    sta $22
    stx $23
    lda $22
    ldx $23
    pha
    phx
    struct_elem.16 __zp_scratch, $20, #2
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    .loc "test_designated_init.c", 30
    leax.local 17
    sta $20
    stx $21
    lda #1
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    pha
    phx
    struct_elem.16 __zp_scratch, $20, #0
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    lda #2
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    pha
    phx
    struct_elem.16 __zp_scratch, $20, #2
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    lda #99
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    pha
    phx
    struct_elem.16 __zp_scratch, $20, #4
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    .loc "test_designated_init.c", 33
    leax.local 5
    sta $20
    stx $21
    ldy #0
    lda ($20),y
    pha
    iny
    lda ($20),y
    tax
    pla
    sta $22
    stx $23
    leax.local 5
    sta $20
    stx $21
    lda $20
    ldx $21
    add.16 .AX, #2
    sta $24
    stx $25
    ldy #0
    lda ($24),y
    pha
    iny
    lda ($24),y
    tax
    pla
    sta $20
    stx $21
    lda $22
    clc
    adc $20
    sta $24
    lda $23
    adc $20+1
    sta $25
    lda $24
    ldx $25
    sta _main__local_26
    stx _main__local_26+1
    .loc "test_designated_init.c", 34
    leax.local 2
    sta $20
    stx $21
    ldy #0
    lda ($20),y
    ldx #0
    sta $22
    lda $22
    ldx #0
    ldx #0
    sta $20
    stx $21
    lda _main__local_26
    ldx _main__local_26+1
    add.16 .AX, $20
    sta $22
    stx $23
    sta _main__local_26
    stx _main__local_26+1
    .loc "test_designated_init.c", 35
    leax.local 9
    sta $20
    stx $21
    lda #1
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $26
    stx $27
    lda _main__local_26
    ldx _main__local_26+1
    add.16 .AX, $26
    sta $20
    stx $21
    sta _main__local_26
    stx _main__local_26+1
    .loc "test_designated_init.c", 36
    leax.local 9
    sta $20
    stx $21
    lda #3
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $26
    stx $27
    lda _main__local_26
    ldx _main__local_26+1
    add.16 .AX, $26
    sta $20
    stx $21
    sta _main__local_26
    stx _main__local_26+1
    .loc "test_designated_init.c", 37
    leax.local 17
    sta $20
    stx $21
    lda #3
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $26
    stx $27
    lda _main__local_26
    ldx _main__local_26+1
    add.16 .AX, $26
    sta $20
    stx $21
    sta _main__local_26
    stx _main__local_26+1
    .loc "test_designated_init.c", 38
    lda #2
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_lookup
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $24
    stx $25
    lda _main__local_26
    ldx _main__local_26+1
    add.16 .AX, $24
    sta $20
    stx $21
    sta _main__local_26
    stx _main__local_26+1
    .loc "test_designated_init.c", 40
    lda _main__local_26
    ldx _main__local_26+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 25
    endproc


__zp_save_buf:
