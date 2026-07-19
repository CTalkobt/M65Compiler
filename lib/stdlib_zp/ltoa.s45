; ltoa.s — Convert 32-bit long to string (ZP calling convention)
;
; char *ltoa(long value, char *str, int base);
;   Converts a 32-bit integer to a NUL-terminated string.
;   Returns str.
;
;   Uses MEGA65 hardware divider at $D760-$D773.
;   ZP param layout: value=$03-$06, str=$07/$08, base=$09/$0A
;   Internal: $02=digit count, value modified in-place at $03-$06

.global _ltoa

.segment "code"

proc _ltoa
    .zp_uses $03, $04, $05, $06, $07, $08, $09, $0A
    .zp_clobbers $03, $04, $05, $06, $07, $08, $09, $0A
    .zp_release $03, $04, $05, $06, $09, $0A
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z

    ; value=$03-$06, str=$07/$08, base=$09/$0A
    ; value is modified in-place during division
    stz $02             ; $02 = digit count (__zp_scratch)
    ldy #0              ; Y = write index into output

    ; --- Validate base (2-36) ---
    lda $0A
    bne @valid
    lda $09
    cmp #2
    bcs @valid
    ; base < 2: write "0\0" and return
    lda #$30
    sta ($07),y
    iny
    lda #0
    sta ($07),y
    lda $07
    ldx $08
    rts

@valid:
    ; --- Handle negative for base 10 only ---
    lda $09
    cmp #10
    bne @poscheck
    lda $0A
    bne @poscheck
    ; base == 10: check sign bit (byte 3)
    lda $06
    bpl @poscheck
    ; Negative: write '-' and negate 32-bit value
    lda #$2d
    sta ($07),y
    iny
    sec
    lda #0
    sbc $03
    sta $03
    lda #0
    sbc $04
    sta $04
    lda #0
    sbc $05
    sta $05
    lda #0
    sbc $06
    sta $06
@poscheck:

    ; --- Check for zero ---
    lda $03
    ora $04
    ora $05
    ora $06
    bne @extract
    lda #$30
    sta ($07),y
    iny
    bra @nulterm

    ; --- Extract digits: divide 32-bit value by base ---
@extract:
    ; Dividend: $D760-$D763 (32-bit)
    lda $03
    sta $d760
    lda $04
    sta $d761
    lda $05
    sta $d762
    lda $06
    sta $d763
    ; Divisor: $D764-$D767 (base, 16-bit zero-extended)
    lda $09
    sta $d764
    lda $0A
    sta $d765
    lda #0
    sta $d766
    sta $d767

@waitdiv:
    bit $D70F
    bne @waitdiv

    ; Convert remainder ($D770) to character
    lda $d770
    cmp #10
    bcs @hexdig
    clc
    adc #$30
    bra @pushdig
@hexdig:
    sec
    sbc #10
    clc
    adc #$41
@pushdig:
    pha
    inc $02

    ; Quotient ($D768-$D76B) becomes new value
    lda $d768
    sta $03
    lda $d769
    sta $04
    lda $d76a
    sta $05
    lda $d76b
    sta $06

    ; Continue while value != 0
    ora $03
    ora $04
    ora $05
    bne @extract

    ; --- Pop digits into output buffer (correct order) ---
@poploop:
    pla
    sta ($07),y
    iny
    dec $02
    bne @poploop

@nulterm:
    lda #0
    sta ($07),y

    lda $07
    ldx $08
    rts
    endproc
