    .o45
    .extern __sp_base
    __zp_scratch = $08
    __zp_scratch2 = $0A
    __zp_scratch3 = $0C
    __zp_scratch4 = $0E

    .extern _isalnum
    .extern _isalpha
    .extern _isdigit
    .extern _isprint
    .extern _isspace
    .extern _putchar
    .extern _puts
    .extern _tolower
    .extern _toupper

    .global _print_hex
    .global _check_char
    .global _main

    .segment "code"

; function _print_hex
    proc _print_hex, W#@_p_val
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdio.h", 14
; frame: 12 bytes (frame-allocated vRegs only)
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    .local __vr1 = 0
    .local __vr2 = 2
    .local __vr13 = 4
    .local __vr18 = 6
    .local __vr23 = 8
    .local __vr28 = 10
    .local @_l_hi = 0
    .local @_l_lo = 2
    .var @_p_val = 14

    ldax.fp @_p_val
    sta $22
    stx $23
@entry:
    .loc "ctype_demo.c", 17
    lda $22
    ldx $23
    lsr.16 .AX
    lsr.16 .AX
    lsr.16 .AX
    lsr.16 .AX
    sta $20
    stx $21
    lda #15
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx $21
    and.16 .AX, $24
    sta $26
    stx $27
    lda $26
    ldx $27
    stax.fp __vr1
    .loc "ctype_demo.c", 18
    lda #15
    ldx #0
    sta $20
    stx $21
    lda $22
    ldx $23
    and.16 .AX, $20
    sta $24
    stx $25
    lda $24
    ldx $25
    stax.fp __vr2
    .loc "ctype_demo.c", 19
    phw #36
    .var _fp = _fp + 2
    jsr _putchar
    .var _fp = _fp - 2
    .loc "ctype_demo.c", 20
    ldax.fp __vr1
    cmp.16 .AX, #10
    bcc @if_then0
    bra @if_else1
@if_then0:
    lda #48
    ldx #0
    sta $20
    stx $21
    ldax.fp __vr1
    add.16 .AX, $20
    stax.fp __vr13
    ldax.fp __vr13
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _putchar
    .var _fp = _fp - 2
    bra @if_end2
@if_else1:
    lda #65
    ldx #0
    sta $20
    stx $21
    ldax.fp __vr1
    add.16 .AX, $20
    sta $24
    stx $25
    lda $24
    ldx $25
    sub.16 .AX, #10
    stax.fp __vr18
    ldax.fp __vr18
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _putchar
    .var _fp = _fp - 2
@if_end2:
    .loc "ctype_demo.c", 21
    ldax.fp __vr2
    cmp.16 .AX, #10
    bcc @if_then3
    bra @if_else4
@if_then3:
    lda #48
    ldx #0
    sta $20
    stx $21
    ldax.fp __vr2
    add.16 .AX, $20
    stax.fp __vr23
    ldax.fp __vr23
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _putchar
    .var _fp = _fp - 2
    bra @if_end5
@if_else4:
    lda #65
    ldx #0
    sta $20
    stx $21
    ldax.fp __vr2
    add.16 .AX, $20
    sta $24
    stx $25
    lda $24
    ldx $25
    sub.16 .AX, #10
    stax.fp __vr28
    ldax.fp __vr28
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _putchar
    .var _fp = _fp - 2
@if_end5:
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
    .func_flags stack_call
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc

; function _check_char
    proc _check_char, W#@_p_c
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/ctype.h", 9
; frame: 2 bytes (frame-allocated vRegs only)
    phw #0
    .local __vr0 = 0
    .var @_p_c = 4

    ldax.fp @_p_c
    stax.fp __vr0
@entry:
    .loc "ctype_demo.c", 25
    phw #32
    .var _fp = _fp + 2
    jsr _putchar
    .var _fp = _fp - 2
    .loc "ctype_demo.c", 26
    ldax.fp __vr0
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _print_hex
    .var _fp = _fp - 2
    .loc "ctype_demo.c", 27
    phw #58
    .var _fp = _fp + 2
    jsr _putchar
    .var _fp = _fp - 2
    .loc "ctype_demo.c", 28
    ldax.fp __vr0
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _isalpha
    .var _fp = _fp - 2
    sta $20
    stx $21
    lda $20
    ldx $21
    bne @if_then6
    cmp #$00
    bne @if_then6
    bra @if_else7
@if_then6:
    phw #65
    .var _fp = _fp + 2
    jsr _putchar
    .var _fp = _fp - 2
    bra @if_end8
@if_else7:
    phw #45
    .var _fp = _fp + 2
    jsr _putchar
    .var _fp = _fp - 2
@if_end8:
    .loc "ctype_demo.c", 29
    ldax.fp __vr0
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _isdigit
    .var _fp = _fp - 2
    sta $20
    stx $21
    lda $20
    ldx $21
    bne @if_then9
    cmp #$00
    bne @if_then9
    bra @if_else10
@if_then9:
    phw #68
    .var _fp = _fp + 2
    jsr _putchar
    .var _fp = _fp - 2
    bra @if_end11
@if_else10:
    phw #45
    .var _fp = _fp + 2
    jsr _putchar
    .var _fp = _fp - 2
@if_end11:
    .loc "ctype_demo.c", 30
    ldax.fp __vr0
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _isalnum
    .var _fp = _fp - 2
    sta $20
    stx $21
    lda $20
    ldx $21
    bne @if_then12
    cmp #$00
    bne @if_then12
    bra @if_else13
@if_then12:
    phw #78
    .var _fp = _fp + 2
    jsr _putchar
    .var _fp = _fp - 2
    bra @if_end14
@if_else13:
    phw #45
    .var _fp = _fp + 2
    jsr _putchar
    .var _fp = _fp - 2
@if_end14:
    .loc "ctype_demo.c", 31
    ldax.fp __vr0
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _isspace
    .var _fp = _fp - 2
    sta $20
    stx $21
    lda $20
    ldx $21
    bne @if_then15
    cmp #$00
    bne @if_then15
    bra @if_else16
@if_then15:
    phw #83
    .var _fp = _fp + 2
    jsr _putchar
    .var _fp = _fp - 2
    bra @if_end17
@if_else16:
    phw #45
    .var _fp = _fp + 2
    jsr _putchar
    .var _fp = _fp - 2
@if_end17:
    .loc "ctype_demo.c", 32
    ldax.fp __vr0
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _isprint
    .var _fp = _fp - 2
    sta $20
    stx $21
    lda $20
    ldx $21
    bne @if_then18
    cmp #$00
    bne @if_then18
    bra @if_else19
@if_then18:
    phw #80
    .var _fp = _fp + 2
    jsr _putchar
    .var _fp = _fp - 2
    bra @if_end20
@if_else19:
    phw #45
    .var _fp = _fp + 2
    jsr _putchar
    .var _fp = _fp - 2
@if_end20:
    .loc "ctype_demo.c", 33
    phw #32
    .var _fp = _fp + 2
    jsr _putchar
    .var _fp = _fp - 2
@__return:
    plz
    plz
    .func_flags stack_call
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc

; function _main
    proc _main
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/ctype.h", 21
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
    .local __vr7 = 6
    .local __vr10 = 8
    .local __vr13 = 10
    .local __vr16 = 12
    .local __vr19 = 14
    .local __vr21 = 16
    .local __vr23 = 18
    .local __vr28 = 20
    .local __vr29 = 22
    .local __vr34 = 24
    .local __vr35 = 26

@entry:
    .loc "ctype_demo.c", 37
    ldax #__str_21
    stax.fp __vr0
    ldax.fp __vr0
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _puts
    .var _fp = _fp - 2
    .loc "ctype_demo.c", 38
    ldax #__str_22
    stax.fp __vr2
    ldax.fp __vr2
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _puts
    .var _fp = _fp - 2
    .loc "ctype_demo.c", 39
    ldax #__str_23
    stax.fp __vr4
    ldax.fp __vr4
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _puts
    .var _fp = _fp - 2
    .loc "ctype_demo.c", 42
    phw #72
    .var _fp = _fp + 2
    jsr _check_char
    .var _fp = _fp - 2
    .loc "ctype_demo.c", 43
    ldax #__str_24
    stax.fp __vr7
    ldax.fp __vr7
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _puts
    .var _fp = _fp - 2
    .loc "ctype_demo.c", 46
    phw #200
    .var _fp = _fp + 2
    jsr _check_char
    .var _fp = _fp - 2
    .loc "ctype_demo.c", 47
    ldax #__str_25
    stax.fp __vr10
    ldax.fp __vr10
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _puts
    .var _fp = _fp - 2
    .loc "ctype_demo.c", 50
    phw #53
    .var _fp = _fp + 2
    jsr _check_char
    .var _fp = _fp - 2
    .loc "ctype_demo.c", 51
    ldax #__str_26
    stax.fp __vr13
    ldax.fp __vr13
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _puts
    .var _fp = _fp - 2
    .loc "ctype_demo.c", 54
    phw #32
    .var _fp = _fp + 2
    jsr _check_char
    .var _fp = _fp - 2
    .loc "ctype_demo.c", 55
    ldax #__str_27
    stax.fp __vr16
    ldax.fp __vr16
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _puts
    .var _fp = _fp - 2
    .loc "ctype_demo.c", 58
    phw #13
    .var _fp = _fp + 2
    jsr _check_char
    .var _fp = _fp - 2
    .loc "ctype_demo.c", 59
    ldax #__str_28
    stax.fp __vr19
    ldax.fp __vr19
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _puts
    .var _fp = _fp - 2
    .loc "ctype_demo.c", 62
    ldax #__str_29
    stax.fp __vr21
    ldax.fp __vr21
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _puts
    .var _fp = _fp - 2
    .loc "ctype_demo.c", 63
    ldax #__str_30
    stax.fp __vr23
    ldax.fp __vr23
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _puts
    .var _fp = _fp - 2
    .loc "ctype_demo.c", 64
    phw #32
    .var _fp = _fp + 2
    jsr _putchar
    .var _fp = _fp - 2
    .loc "ctype_demo.c", 65
    phw #72
    .var _fp = _fp + 2
    jsr _toupper
    .var _fp = _fp - 2
    stax.fp __vr28
    ldax.fp __vr28
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _print_hex
    .var _fp = _fp - 2
    .loc "ctype_demo.c", 66
    ldax #__str_31
    stax.fp __vr29
    ldax.fp __vr29
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _puts
    .var _fp = _fp - 2
    .loc "ctype_demo.c", 67
    phw #32
    .var _fp = _fp + 2
    jsr _putchar
    .var _fp = _fp - 2
    .loc "ctype_demo.c", 68
    phw #200
    .var _fp = _fp + 2
    jsr _tolower
    .var _fp = _fp - 2
    stax.fp __vr34
    ldax.fp __vr34
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _print_hex
    .var _fp = _fp - 2
    .loc "ctype_demo.c", 69
    ldax #__str_32
    stax.fp __vr35
    ldax.fp __vr35
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _puts
    .var _fp = _fp - 2
    .loc "ctype_demo.c", 71
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


    .segment "data"
__str_21:
    .text "ctype.h demo"
    .byte 0
__str_22:
    .text "flags: a=alpha d=digit n=alnum s=space p=print"
    .byte 0
__str_23:
    .text ""
    .byte 0
__str_24:
    .text "'h' lowercase"
    .byte 0
__str_25:
    .text "'h' uppercase"
    .byte 0
__str_26:
    .text "'5' digit"
    .byte 0
__str_27:
    .text "' ' space"
    .byte 0
__str_28:
    .text "cr control"
    .byte 0
__str_29:
    .text ""
    .byte 0
__str_30:
    .text "case conversion:"
    .byte 0
__str_31:
    .text " = toupper('h')"
    .byte 0
__str_32:
    .text " = tolower('h')"
    .byte 0

__zp_save_buf:
