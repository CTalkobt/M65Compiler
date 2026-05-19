; Test: chknonzero / chkzero / select simulated op branch targets
; Validates that branch displacements are correct (regression test for
; off-by-one bug where BNE/BEQ +3 should have been +4, landing on CLE
; opcode $02 on 45GS02 and corrupting the stack).
;
; Each test writes a 1-byte result to $4000+N.
; Expected: all results = $01 (pass), sentinel $AA at end.

.org $2000
    results = $4000

; =====================================================================
; Test 0: chknonzero.8 with nonzero value → expect 1
; Expected: $4000 = $01
; =====================================================================
    lda #$42
    chknonzero.8 .A
    sta results+0

; =====================================================================
; Test 1: chknonzero.8 with zero value → expect 0
; Expected: $4001 = $00
; =====================================================================
    lda #$00
    chknonzero.8 .A
    sta results+1

; =====================================================================
; Test 2: chkzero.8 with zero value → expect 1
; Expected: $4002 = $01
; =====================================================================
    lda #$00
    chkzero.8 .A
    sta results+2

; =====================================================================
; Test 3: chkzero.8 with nonzero value → expect 0
; Expected: $4003 = $00
; =====================================================================
    lda #$FF
    chkzero.8 .A
    sta results+3

; =====================================================================
; Test 4: chknonzero.16 with nonzero low byte → expect 1
; Expected: $4004 = $01
; =====================================================================
    lda #$42
    ldx #$00
    chknonzero.16 .AX
    sta results+4

; =====================================================================
; Test 5: chknonzero.16 with nonzero high byte only → expect 1
; Expected: $4005 = $01
; =====================================================================
    lda #$00
    ldx #$01
    chknonzero.16 .AX
    sta results+5

; =====================================================================
; Test 6: chknonzero.16 with both zero → expect 0
; Expected: $4006 = $00
; =====================================================================
    lda #$00
    ldx #$00
    chknonzero.16 .AX
    sta results+6

; =====================================================================
; Test 7: chkzero.16 with both zero → expect 1
; Expected: $4007 = $01
; =====================================================================
    lda #$00
    ldx #$00
    chkzero.16 .AX
    sta results+7

; =====================================================================
; Test 8: chkzero.16 with nonzero low byte → expect 0
; Expected: $4008 = $00
; =====================================================================
    lda #$01
    ldx #$00
    chkzero.16 .AX
    sta results+8

; =====================================================================
; Test 9: chkzero.16 with nonzero high byte only → expect 0
; Expected: $4009 = $00
; =====================================================================
    lda #$00
    ldx #$01
    chkzero.16 .AX
    sta results+9

; =====================================================================
; Test 10: select with nonzero condition → expect val1 ($BB)
; Expected: $400A = $BB
; =====================================================================
    lda #$01
    cmp #$00
    select .A, #$BB, #$CC
    sta results+10

; =====================================================================
; Test 11: select with zero condition → expect val2 ($CC)
; Expected: $400B = $CC
; =====================================================================
    lda #$00
    cmp #$00
    select .A, #$BB, #$CC
    sta results+11

; =====================================================================
; Sentinel
; =====================================================================
    lda #$AA
    sta results+12

    brk
