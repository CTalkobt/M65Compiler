; ldiv.s — Long division with quotient and remainder (ZP calling convention)
;
; ldiv_t ldiv(long numer, long denom);
;   Returns ldiv_t { long quot; long rem; } via hidden return pointer in $03/$04.
;   numer in $05-$08, denom in $09-$0C.

.global _ldiv

.segment "code"

proc _ldiv
    .zp_uses $03, $04, $05, $06, $07, $08, $09, $0A, $0B, $0C
    .zp_clobbers $03, $04, $05, $06, $07, $08, $09, $0A, $0B, $0C
    .zp_release $03, $04, $05, $06, $07, $08, $09, $0A, $0B, $0C
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z

    lda $02
    pha
    lda $0D
    pha

    stz $02                 ; numer_neg
    stz $0D                 ; quot sign XOR

    ; --- abs(numer) ---
    lda $08                 ; byte3
    bpl @numer_pos
    inc $02
    inc $0D
    sec
    lda #0
    sbc $05
    sta $D760
    lda #0
    sbc $06
    sta $D761
    lda #0
    sbc $07
    sta $D762
    lda #0
    sbc $08
    sta $D763
    bra @do_denom
@numer_pos:
    lda $05
    sta $D760
    lda $06
    sta $D761
    lda $07
    sta $D762
    lda $08
    sta $D763

@do_denom:
    ; --- abs(denom) ---
    lda $0C                 ; byte3
    bpl @denom_pos
    lda $0D
    eor #1
    sta $0D
    sec
    lda #0
    sbc $09
    sta $D764
    lda #0
    sbc $0A
    sta $D765
    lda #0
    sbc $0B
    sta $D766
    lda #0
    sbc $0C
    sta $D767
    bra @divide
@denom_pos:
    lda $09
    sta $D764
    lda $0A
    sta $D765
    lda $0B
    sta $D766
    lda $0C
    sta $D767

@divide:
@wait:
    bit $D70F
    bne @wait

    ; --- Quotient with sign ---
    lda $0D
    beq @quot_pos
    sec
    lda #0
    sbc $D768
    ldy #0
    sta ($03), y
    lda #0
    sbc $D769
    iny
    sta ($03), y
    lda #0
    sbc $D76A
    iny
    sta ($03), y
    lda #0
    sbc $D76B
    iny
    sta ($03), y
    bra @do_rem
@quot_pos:
    ldy #0
    lda $D768
    sta ($03), y
    iny
    lda $D769
    sta ($03), y
    iny
    lda $D76A
    sta ($03), y
    iny
    lda $D76B
    sta ($03), y

@do_rem:
    ; --- Remainder with numer sign ---
    lda $02
    beq @rem_pos
    sec
    lda #0
    sbc $D770
    ldy #4
    sta ($03), y
    lda #0
    sbc $D771
    iny
    sta ($03), y
    lda #0
    sbc $D772
    iny
    sta ($03), y
    lda #0
    sbc $D773
    iny
    sta ($03), y
    bra @done
@rem_pos:
    ldy #4
    lda $D770
    sta ($03), y
    iny
    lda $D771
    sta ($03), y
    iny
    lda $D772
    sta ($03), y
    iny
    lda $D773
    sta ($03), y

@done:
    lda $03
    ldx $04

    pla
    sta $0D
    pla
    sta $02
    rts
    endproc
