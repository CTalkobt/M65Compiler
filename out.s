* = $2000
    __sp_base = $0101
; Save ZP $08-$FF to BSS buffer
    ldx #0
@__zp_save_loop:
    lda $08,x
    sta __zp_save_buf,x
    inx
    cpx #248
    bne @__zp_save_loop
    sta $02
    stx $03
; Restore ZP $08-$FF from BSS buffer
    ldx #0
@__zp_restore_loop:
    lda __zp_save_buf,x
    sta $08,x
    inx
    cpx #248
    bne @__zp_restore_loop
    lda $02
    ldx $03
__halt:
    jmp __halt
    __zp_scratch = $08
    __zp_scratch2 = $0A
    __zp_scratch3 = $0C
    __zp_scratch4 = $0E


; function _s1
    proc _s1
    .var _fp = 0
    .loc "/tmp/test_multi_enc.c", 2

@entry:
    ldax #__str_0
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    .func_flags stack_call, leaf
    .reg_clobbers A, X
    .flag_clobbers N, Z
    endproc

; function _s2
    proc _s2
    .var _fp = 0
    .loc "/tmp/test_multi_enc.c", 4

@entry:
    ldax #__str_1
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    .func_flags stack_call, leaf
    .reg_clobbers A, X
    .flag_clobbers N, Z
    endproc

; function _s3
    proc _s3
    .var _fp = 0
    .loc "/tmp/test_multi_enc.c", 6

@entry:
    ldax #__str_2
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    .func_flags stack_call, leaf
    .reg_clobbers A, X
    .flag_clobbers N, Z
    endproc


__str_0:
    .text "ABC"
    .byte 0
__str_1:
    .text "DEF"
    .byte 0
__str_2:
    .text "GHI"
    .byte 0

__zp_save_buf:
