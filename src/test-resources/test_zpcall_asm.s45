; test_zpcall_asm.s — Hand-written ZP calling convention test
;
; Tests:
;   1. Basic ZP param passing (2 char params → add)
;   2. 16-bit ZP param passing (int add)
;   3. Nested call with caller-save/restore
;   4. Function attributes (.zp_uses / .zp_clobbers / .reg_clobbers)
;   5. Long return in AXYZ
;
; Expected at $4000: 0F 30 75 E0 93 04 00 AA
;   [0] = $0F (add_chars(5,10) = 15)
;   [1] = $30 (add_int(20,28) = 48)
;   [2] = $75 (outer_add(50,17) = double(50)+17 = 117)
;   [3..6] = $E0 $93 $04 $00 (add_long(100000,200000) = 300000)
;   [7] = $AA sentinel

.org $2000
__sp_base = $0101
__zp_scratch = $02

.global add_chars
.global add_int
.global double_int
.global outer_add
.global add_long_fn

    ; Init 16-bit stack
    cle
    ldy #$01
    tys
    ldx #$FF
    txs

    ; === Test 1: add_chars(5, 10) → 15 ===
    lda #5
    sta $03
    lda #10
    sta $04
    jsr add_chars
    sta $4000       ; A = 15 = $0F

    ; === Test 2: add_int($0014, $001C) → $0030 (20 + 28 = 48) ===
    lda #$14
    sta $03
    lda #$00
    sta $04
    lda #$1C
    sta $05
    lda #$00
    sta $06
    jsr add_int
    sta $4001       ; lo byte = $30

    ; === Test 3: outer_add(50, 17) → double(50)+17 = 100+17 = 117 = $75 ===
    lda #50
    sta $03
    lda #0
    sta $04
    lda #17
    sta $05
    lda #0
    sta $06
    jsr outer_add
    sta $4002       ; lo = $75

    ; === Test 4: add_long_fn(100000, 200000) → 300000 = $000493E0 ===
    ; 100000 = $000186A0
    lda #$A0
    sta $03
    lda #$86
    sta $04
    lda #$01
    sta $05
    lda #$00
    sta $06
    ; 200000 = $00030D40
    lda #$40
    sta $07
    lda #$0D
    sta $08
    lda #$03
    sta $09
    lda #$00
    sta $0A
    jsr add_long_fn
    ; Result in AXYZ
    sta $4003       ; $E0
    stx $4004       ; $93
    sty $4005       ; $04
    stz $4006       ; $00

    ; Sentinel
    lda #$AA
    sta $4007

    brk

; ---------------------------------------------------------------
; add_chars: $03 + $04 → A
; ---------------------------------------------------------------
proc add_chars
    .zp_uses $03, $04
    .zp_clobbers $03, $04
    .reg_clobbers A

    lda $03
    clc
    adc $04
    rts
    endproc

; ---------------------------------------------------------------
; add_int: ($03/$04) + ($05/$06) → AX (lo/hi)
; ---------------------------------------------------------------
proc add_int
    .zp_uses $03, $04, $05, $06
    .zp_clobbers $03, $04, $05, $06
    .reg_clobbers A, X

    lda $03
    clc
    adc $05
    pha             ; save lo
    lda $04
    adc $06
    tax             ; hi → X
    pla             ; lo → A
    rts
    endproc

; ---------------------------------------------------------------
; double_int: ($03/$04) * 2 → AX
; Clobbers $03/$04
; ---------------------------------------------------------------
proc double_int
    .zp_uses $03, $04
    .zp_clobbers $03, $04
    .reg_clobbers A, X

    lda $03
    asl
    sta $03
    lda $04
    rol
    sta $04
    lda $03
    ldx $04
    rts
    endproc

; ---------------------------------------------------------------
; outer_add: double_int($03/$04) + ($05/$06)
; Must save $05/$06 across call to double_int
; ---------------------------------------------------------------
proc outer_add
    .zp_uses $03, $04, $05, $06
    .zp_clobbers $03, $04, $05, $06
    .reg_clobbers A, X

    ; Caller-save: push b ($05/$06) onto stack
    lda $06
    pha
    lda $05
    pha

    ; Call double_int($03/$04) — result in AX
    jsr double_int
    ; A = lo(doubled), X = hi(doubled)

    ; Save doubled result temporarily
    sta $03
    stx $04

    ; Restore b from stack
    pla
    sta $05         ; lo of b
    pla
    sta $06         ; hi of b

    ; Add: doubled ($03/$04) + b ($05/$06) → AX
    lda $03
    clc
    adc $05
    pha             ; save result lo
    lda $04
    adc $06
    tax             ; result hi → X
    pla             ; result lo → A
    rts
    endproc

; ---------------------------------------------------------------
; add_long_fn: ($03-$06) + ($07-$0A) → AXYZ (32-bit)
; ---------------------------------------------------------------
proc add_long_fn
    .zp_uses $03, $04, $05, $06, $07, $08, $09, $0A
    .zp_clobbers $03, $04, $05, $06, $07, $08, $09, $0A
    .zp_release $07, $08, $09, $0A
    .reg_clobbers A, X, Y, Z

    ldq $03
    clc
    adcq $07
    ; Q = AXYZ (byte0=A, byte1=X, byte2=Y, byte3=Z)
    rts
    endproc
