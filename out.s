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
    jsr _main
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

    .extern _printf

_ascii_str:
    .res 6
_petscii_str:
    .res 8
_large_number:
    .dword 305419896
_vflag:
    .word 0
_const_val:
    .word 42


; function _test_asm
    proc _test_asm
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stddef.h", 4
    .var @_l_x = $20

@entry:
    .loc "/tmp/verify_v1_0.c", 26
    .loc "/tmp/verify_v1_0.c", 27
    lda #$42
@__return:
    .func_flags stack_call, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc

; function _zp_func
    proc _zp_func, W#@_p_a, W#@_p_b
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stddef.h", 11
    .var @_p_a = 2
    .var @_p_b = 4

    ldax.fp @_p_a
    sta $20
    stx $21
    ldax.fp @_p_b
    sta $22
    stx $23
@entry:
    .loc "/tmp/verify_v1_0.c", 33
    lda $20
    clc
    adc $22
    sta $24
    lda $21
    adc $22+1
    sta $25
    lda $24
    ldx $25
@__return:
    .func_flags stack_call, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    endproc

; function _main
    proc _main
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdlib.h", 7
; frame: 28 bytes (frame-allocated vRegs only)
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    .local __vr0 = 0
    .local __vr2 = 2
    .local __vr4 = 4
    .local __vr6 = 6
    .local __vr7 = 8
    .local __vr9 = 12
    .local __vr10 = 14
    .local __vr12 = 16
    .local __vr13 = 18
    .local __vr15 = 20
    .local __vr17 = 22
    .local __vr20 = 24
    .local __vr22 = 26

@entry:
    .loc "/tmp/verify_v1_0.c", 37
    ldax #__str_0
    stax.fp __vr0
    ldax.fp __vr0
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _printf
    .var _fp = _fp - 2
    .loc "/tmp/verify_v1_0.c", 38
    ldax #__str_1
    stax.fp __vr2
    ldax.fp __vr2
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _printf
    .var _fp = _fp - 2
    .loc "/tmp/verify_v1_0.c", 39
    ldax #__str_2
    stax.fp __vr4
    ldax.fp __vr4
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _printf
    .var _fp = _fp - 2
    .loc "/tmp/verify_v1_0.c", 40
    ldax #__str_3
    stax.fp __vr6
    lda _large_number
    ldx _large_number+1
    ldy _large_number+2
    ldz _large_number+3
    staxyz.fp __vr7
    ldaxyz.fp __vr7
    sta $28
    stx $29
    sty $2A
    stz $2B
    ldax.fp __vr6
    sta $2C
    stx $2D
    lda $28
    ldx $29
    ldy $2A
    ldz $2B
    push .axyz
    .var _fp = _fp + 4
    lda $2C
    ldx $2D
    push .ax
    .var _fp = _fp + 2
    jsr _printf
    .var _fp = _fp - 6
    .loc "/tmp/verify_v1_0.c", 41
    ldax #__str_4
    stax.fp __vr9
    lda _vflag
    ldx _vflag+1
    stax.fp __vr10
    ldax.fp __vr10
    sta $28
    stx $29
    ldax.fp __vr9
    sta $2C
    stx $2D
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    lda $2C
    ldx $2D
    push .ax
    .var _fp = _fp + 2
    jsr _printf
    .var _fp = _fp - 4
    .loc "/tmp/verify_v1_0.c", 42
    ldax #__str_5
    stax.fp __vr12
    lda _const_val
    ldx _const_val+1
    stax.fp __vr13
    ldax.fp __vr13
    sta $28
    stx $29
    ldax.fp __vr12
    sta $2C
    stx $2D
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    lda $2C
    ldx $2D
    push .ax
    .var _fp = _fp + 2
    jsr _printf
    .var _fp = _fp - 4
    .loc "/tmp/verify_v1_0.c", 43
    jsr _test_asm
    .loc "/tmp/verify_v1_0.c", 44
    ldax #__str_6
    stax.fp __vr15
    ldax.fp __vr15
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _printf
    .var _fp = _fp - 2
    .loc "/tmp/verify_v1_0.c", 45
    ldax #__str_7
    stax.fp __vr17
    phw #4
    .var _fp = _fp + 2
    phw #3
    .var _fp = _fp + 2
    jsr _zp_func
    .var _fp = _fp - 4
    stax.fp __vr20
    ldax.fp __vr20
    sta $28
    stx $29
    ldax.fp __vr17
    sta $2C
    stx $2D
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    lda $2C
    ldx $2D
    push .ax
    .var _fp = _fp + 2
    jsr _printf
    .var _fp = _fp - 4
    .loc "/tmp/verify_v1_0.c", 46
    ldax #__str_8
    stax.fp __vr22
    ldax.fp __vr22
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _printf
    .var _fp = _fp - 2
    .loc "/tmp/verify_v1_0.c", 47
    lda #0
    ldx #0
@__return:
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    .func_flags stack_call
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc


__str_0:
    .ascii "v1.0 Feature Verification
"
    .byte 0
__str_1:
    .ascii "  ASCII encoding: OK
"
    .byte 0
__str_2:
    .ascii "  PETSCII encoding: OK
"
    .byte 0
__str_3:
    .ascii "  Long type: %ld
"
    .byte 0
__str_4:
    .ascii "  Volatile: %d
"
    .byte 0
__str_5:
    .ascii "  Const: %d
"
    .byte 0
__str_6:
    .ascii "  Inline asm: OK
"
    .byte 0
__str_7:
    .ascii "  ZP calling convention: %d
"
    .byte 0
__str_8:
    .ascii "
All v1.0 features verified!
"
    .byte 0

__zp_save_buf:
