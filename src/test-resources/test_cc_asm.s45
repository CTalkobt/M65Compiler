; Test: Hand-written ZP calling convention with .func_flags directives
; Validates that .func_flags directive parsing and function attribute records work correctly

.cpu _45gs02

* = $2000

main:
    ; Test 1: Call zp_add (ZP convention)
    lda #$0A        ; arg 1 = 10
    sta $03
    ldx #$00
    stx $04
    lda #$14        ; arg 2 = 20
    sta $05
    ldx #$00
    stx $06
    jsr zp_add
    sta $4000       ; Store result (30)

    ; Test 2: Call stack_add (Stack convention)
    phw.sp #$14     ; Push 20 (arg 2)
    phw.sp #$0A     ; Push 10 (arg 1)
    jsr stack_add
    sta $4001       ; Store result (30)

    ; Test 3: Call zp_mul (ZP with char result)
    lda #$05        ; arg 1 = 5
    sta $03
    lda #$07        ; arg 2 = 7
    sta $05
    jsr zp_mul
    sta $4002       ; Store result (35)

    brk

; ===== ZP-convention functions =====

zp_add:
    proc
    .func_flags zp_call
    ; a at $03/$04, b at $05/$06
    lda $03
    clc
    adc $05
    tax
    lda $04
    adc $06
    rts
    endproc

zp_mul:
    proc
    .func_flags zp_call, leaf
    ; a at $03, b at $05 (both char)
    lda $03
    ldy $05
    mul.8 a, y
    rts
    endproc

; ===== Stack-convention functions =====

stack_add:
    proc
    .func_flags stack_call
    ; arg 1 at __sp_base+4,X; arg 2 at __sp_base+2,X
    tsx
    lda $0104,x     ; Load arg 1 lo
    clc
    adc $0102,x     ; Add arg 2 lo
    tax
    lda $0105,x
    adc $0103,x
    rts #4          ; Remove 4 param bytes
    endproc
