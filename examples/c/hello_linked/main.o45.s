    .o45
    .extern __sp_base
    .weak __static_chain
    .weak __zp_scratch
    .weak __zp_scratch2
    .weak __zp_scratch3
    .weak __zp_scratch4
    __static_chain = $06
    __zp_scratch = $08
    __zp_scratch2 = $0A
    __zp_scratch3 = $0C
    __zp_scratch4 = $0E

    .extern _itoa
    .extern _printf
    .extern _puts
    .extern _strlen

    .global _main

    .segment "code"

; function _main
    proc _main
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler.dev_v1.0.5/bin/../lib/include/stdio.h", 5
; frame: 26 bytes (frame-allocated vRegs only)
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
    tsx
    txa
    clc
    adc #1
    sta $FD
    lda #$01
    adc #0
    sta $FE
    .local __vr0 = 0
    .local __vr1 = 8
    .local __vr3 = 10
    .local __vr4 = 12
    .local __vr5 = 14
    .local __vr7 = 16
    .local __vr9 = 18
    .local __vr10 = 20
    .local __vr11 = 22
    .local __vr13 = 24
    .local @_l_buf = 0

@entry:
    .loc "main.c", 10
    ldax #__str_0
    stax.fp __vr1
    ldax.fp __vr1
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _puts
    phx
    pha
    tsx
    txa
    clc
    adc #1
    sta $FD
    lda #$01
    adc #0
    sta $FE
    pla
    plx
    plz
    plz
    .var _fp = _fp - 2
    .loc "main.c", 12
    lda #42
    ldx #0
    stax.fp __vr3
    leax.fp 0
    stax.fp __vr4
    lda #10
    ldx #0
    stax.fp __vr5
    ldax.fp __vr5
    sta $28
    stx $29
    ldax.fp __vr4
    sta $2A
    stx $2B
    ldax.fp __vr3
    sta $2C
    stx $2D
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    lda $2A
    ldx $2B
    push .ax
    .var _fp = _fp + 2
    lda $2C
    ldx $2D
    push .ax
    .var _fp = _fp + 2
    jsr _itoa
    phx
    pha
    tsx
    txa
    clc
    adc #1
    sta $FD
    lda #$01
    adc #0
    sta $FE
    pla
    plx
    plz
    plz
    plz
    plz
    plz
    plz
    .var _fp = _fp - 6
    .loc "main.c", 13
    leax.fp 0
    stax.fp __vr7
    ldax.fp __vr7
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _puts
    phx
    pha
    tsx
    txa
    clc
    adc #1
    sta $FD
    lda #$01
    adc #0
    sta $FE
    pla
    plx
    plz
    plz
    .var _fp = _fp - 2
    .loc "main.c", 15
    ldax #__str_1
    stax.fp __vr9
    ldax #__str_2
    stax.fp __vr10
    ldax.fp __vr10
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _strlen
    phx
    pha
    tsx
    txa
    clc
    adc #1
    sta $FD
    lda #$01
    adc #0
    sta $FE
    pla
    plx
    plz
    plz
    .var _fp = _fp - 2
    stax.fp __vr11
    ldax.fp __vr11
    sta $28
    stx $29
    ldax.fp __vr9
    sta $2A
    stx $2B
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    lda $2A
    ldx $2B
    push .ax
    .var _fp = _fp + 2
    jsr _printf
    phx
    pha
    tsx
    txa
    clc
    adc #1
    sta $FD
    lda #$01
    adc #0
    sta $FE
    pla
    plx
    plz
    plz
    plz
    plz
    .var _fp = _fp - 4
    .loc "main.c", 17
    ldax #__str_3
    stax.fp __vr13
    ldax.fp __vr13
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _puts
    phx
    pha
    tsx
    txa
    clc
    adc #1
    sta $FD
    lda #$01
    adc #0
    sta $FE
    pla
    plx
    plz
    plz
    .var _fp = _fp - 2
    .loc "main.c", 19
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
    .func_flags stack_call
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc


    .segment "data"
__str_0:
    .text "linked hello!"
    .byte 0
__str_1:
    .text "string length: %d
"
    .byte 0
__str_2:
    .text "hello"
    .byte 0
__str_3:
    .text "done!"
    .byte 0

__zp_save_buf:
