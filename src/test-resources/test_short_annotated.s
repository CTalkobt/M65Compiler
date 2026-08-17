; ============= C SOURCE CODE =============
;
; // Test: short type (alias for int on 16-bit target)
; // Validates: short, unsigned short, signed short, short in function params/returns,
; //            sizeof(short), short pointers, short arrays.
;
; volatile char *r = (char *)0x4000;
;
; short add_short(short a, short b) {
;     return a + b;
; }
;
; unsigned short mul_short(unsigned short a, unsigned short b) {
;     return a * b;
; }
;
; void main() {
;     short x = 10;
;     short y = 20;
;     unsigned short z = add_short(x, y);
;     signed short neg = -5;
;     short arr[3] = {100, 200, 300};
;     short *p = &x;
;
;     r[0] = z;              // 30 = 0x1E
;     r[1] = neg + 10;       // 5
;     r[2] = sizeof(short);  // 2
;     r[3] = mul_short(3, 4); // 12
;     r[4] = *p;             // 10 = 0x0A
;     r[5] = arr[1];         // 200 = 0xC8
;     r[6] = 0xAA;           // marker
;
;     __asm__("brk");        // Signal test complete to mmemu
; }

; ============= GENERATED ASSEMBLY =============

    .o45
    .extern __sp_base
    .global _add_short__ar
    .global _main__ar
    .global _mul_short__ar
    .weak __static_chain
    .weak __zp_scratch
    .weak __zp_scratch2
    .weak __zp_scratch3
    .weak __zp_scratch4
    .global cc45.zeroPageStart
    __static_chain = $06
    __zp_scratch = $08
    __zp_scratch2 = $0A
    __zp_scratch3 = $0C
    __zp_scratch4 = $0E
    cc45.zeroPageStart = $08

    .global _r
    .global _add_short
    .global _mul_short
    .global _main

    .segment "data"
    .byte 0
_r:
; .debug_var: @global _r offset=0 size=2 type=ptr scope=global
    .word 16384

    .segment "code"

; ============= FUNCTION: add_short(short a, short b) =============
; C: return a + b;
;
; function _add_short
; SAC inline storage: 4 bytes
    .global _add_short__param_a
    _add_short__param_a: .word 0
    .global _add_short__param_b
    _add_short__param_b: .word 0
    _add_short__local_0: .word 0
    _add_short__local_1: .word 0
    proc _add_short, W#@_p_a, W#@_p_b
    .sac
    .var _fp = 0
    .loc "src/test-resources/test_short.c", 7
    .var @_p_a = 2
    .var @_p_b = 4

@entry:
    .loc "src/test-resources/test_short.c", 8
    ; Load parameter b
    lda _add_short__param_b
    ldx _add_short__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    ; Load parameter a
    lda _add_short__param_a
    ldx _add_short__param_a+1
    ; Add: a + b (result in A:X)
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; ============= FUNCTION: mul_short(unsigned short a, unsigned short b) =============
; C: return a * b;
;
; function _mul_short
; SAC inline storage: 4 bytes
    .global _mul_short__param_a
    _mul_short__param_a: .word 0
    .global _mul_short__param_b
    _mul_short__param_b: .word 0
    _mul_short__local_0: .word 0
    _mul_short__local_1: .word 0
    proc _mul_short, W#@_p_a, W#@_p_b
    .sac
    .var _fp = 0
    .loc "src/test-resources/test_short.c", 11
    .var @_p_a = 2
    .var @_p_b = 4

@entry:
    .loc "src/test-resources/test_short.c", 12
    ; Load parameter b
    lda _mul_short__param_b
    ldx _mul_short__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    ; Load parameter a
    lda _mul_short__param_a
    ldx _mul_short__param_a+1
    ; Multiply: a * b (result in A:X)
    mul.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; ============= FUNCTION: main() =============
; C: short x = 10;
; C: short y = 20;
; C: unsigned short z = add_short(x, y);
; C: signed short neg = -5;
; C: short arr[3] = {100, 200, 300};
; C: short *p = &x;
; C: r[0] = z;
; C: r[1] = neg + 10;
; C: r[2] = sizeof(short);
; C: r[3] = mul_short(3, 4);
; C: r[4] = *p;
; C: r[5] = arr[1];
; C: r[6] = 0xAA;
; C: __asm__("brk");
;
; function _main
; SAC inline storage: 16 bytes
    _main__local_0: .word 0
    _main__local_2: .word 0
    _main__local_4: .word 0
    _main__local_6: .word 0
    _main__local_8: .word 0
    _main__local_16: .word 0
    _main__local_32: .word 0
    _main__local_33: .word 0
    proc _main
    .sac
    .var _fp = 0
    .loc "src/test-resources/test_short.c", 15

@entry:
    ; C: short x = 10;
    .loc "src/test-resources/test_short.c", 16
    lda #10
    sta _main__local_0
    lda #0
    sta _main__local_0+1

    ; C: short y = 20;
    .loc "src/test-resources/test_short.c", 17
    lda #20
    sta _main__local_2
    lda #0
    sta _main__local_2+1

    ; C: unsigned short z = add_short(x, y);
    .loc "src/test-resources/test_short.c", 18
    ; Store parameter b (y) to add_short's inline storage
    lda _main__local_2
    ldx _main__local_2+1
    sta _add_short__param_b
    stx _add_short__param_b+1
    ; Store parameter a (x) to add_short's inline storage
    lda _main__local_0
    ldx _main__local_0+1
    sta _add_short__param_a
    stx _add_short__param_a+1
    ; Call add_short
    jsr _add_short
    ; Receive result in A:X, store to local variable z
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_4
    stx _main__local_4+1

    ; C: signed short neg = -5;
    .loc "src/test-resources/test_short.c", 19
    lda #251
    sta _main__local_6
    lda #255
    sta _main__local_6+1

    ; C: short arr[3] = {100, 200, 300};
    .loc "src/test-resources/test_short.c", 20
    leax.local 10
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
    sta (__zp_scratch)

    lda #200
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    pha
    phx
    lda __zp_scratch
    ldx __zp_scratch+1
    clc
    adc #2
    bcc @no_carry_0
    inx
@no_carry_0:
    sta __zp_scratch
    stx __zp_scratch+1
    plx
    pla
    sta (__zp_scratch)

    lda #200
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    pha
    phx
    lda __zp_scratch
    ldx __zp_scratch+1
    clc
    adc #4
    bcc @no_carry_1
    inx
@no_carry_1:
    sta __zp_scratch
    stx __zp_scratch+1
    plx
    pla
    sta (__zp_scratch)

    ; C: short *p = &x;
    .loc "src/test-resources/test_short.c", 21
    lda __bp_of_main_x
    ldx __bp_of_main_x+1
    sta _main__local_8
    stx _main__local_8+1

    ; C: r[0] = z;
    .loc "src/test-resources/test_short.c", 23
    lda _r
    ldx _r+1
    sta $22
    stx $23
    lda #0
    sta $24
    sta $25
    lda _main__local_4
    ldx #0
    pha
    lda $24
    ldx $25
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $22
    ldx $22+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y

    ; C: r[1] = neg + 10;
    .loc "src/test-resources/test_short.c", 24
    lda _main__local_6
    ldx _main__local_6+1
    clc
    adc #10
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    lda _r
    ldx _r+1
    sta $22
    stx $23
    lda #1
    sta $24
    sta $25
    lda $20
    ldx #0
    pha
    lda $24
    ldx $25
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $22
    ldx $22+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y

    ; C: r[2] = sizeof(short);
    .loc "src/test-resources/test_short.c", 25
    lda #2
    sta $20
    lda _r
    ldx _r+1
    sta $22
    stx $23
    lda #2
    sta $24
    sta $25
    lda $20
    ldx #0
    pha
    lda $24
    ldx $25
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $22
    ldx $22+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y

    ; C: r[3] = mul_short(3, 4);
    .loc "src/test-resources/test_short.c", 26
    ; Store parameter b (4) to mul_short's inline storage
    lda #4
    sta _mul_short__param_b
    lda #0
    sta _mul_short__param_b+1
    ; Store parameter a (3) to mul_short's inline storage
    lda #3
    sta _mul_short__param_a
    lda #0
    sta _mul_short__param_a+1
    ; Call mul_short
    jsr _mul_short
    ; Receive result in A:X
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_16
    stx _main__local_16+1

    ; Write result to r[3]
    lda _r
    ldx _r+1
    sta $22
    stx $23
    lda #3
    sta $24
    sta $25
    lda _main__local_16
    ldx #0
    pha
    lda $24
    ldx $25
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $22
    ldx $22+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y

    ; C: r[4] = *p;
    .loc "src/test-resources/test_short.c", 27
    lda _main__local_8
    ldx _main__local_8+1
    sta __zp_scratch
    stx __zp_scratch+1
    lda (__zp_scratch)
    ldx #0
    sta $20
    lda _r
    ldx _r+1
    sta $22
    stx $23
    lda #4
    sta $24
    sta $25
    lda $20
    ldx #0
    pha
    lda $24
    ldx $25
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $22
    ldx $22+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y

    ; C: r[5] = arr[1];
    .loc "src/test-resources/test_short.c", 28
    leax.local 10
    sta $22
    stx $23
    lda #1
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    clc
    adc #2
    sta __zp_scratch
    stx __zp_scratch+1
    lda (__zp_scratch)
    ldx #0
    sta $20
    lda _r
    ldx _r+1
    sta $22
    stx $23
    lda #5
    sta $24
    sta $25
    lda $20
    ldx #0
    pha
    lda $24
    ldx $25
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $22
    ldx $22+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y

    ; C: r[6] = 0xAA;
    .loc "src/test-resources/test_short.c", 29
    lda #170
    sta $20
    lda _r
    ldx _r+1
    sta $22
    stx $23
    lda #6
    sta $24
    sta $25
    lda $20
    ldx #0
    pha
    lda $24
    ldx $25
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $22
    ldx $22+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y

    ; C: __asm__("brk");
    .loc "src/test-resources/test_short.c", 31
    brk

@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 16
    endproc
