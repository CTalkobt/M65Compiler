; itoa.s — Convert integer to string (ZP calling convention)
;
; char *itoa(int value, char *str, int base);
;   Converts a 16-bit integer to a NUL-terminated string.
;   Returns str.
;
;   Uses MEGA65 hardware divider at $D760-$D771.
;   ZP param layout: value=$03/$04, str=$05/$06, base=$07/$08
;   Internal scratch: $02 (digit count), $09/$0A (working value)

.global _itoa

.segment "code"

proc _itoa
    .zp_uses $03, $04, $05, $06, $07, $08
    .zp_clobbers $03, $04, $05, $06, $07, $08, $09, $0A
    .zp_release $03, $04, $07, $08
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z

    ; Copy value to working location ($09/$0A) so we can modify it
    lda $03
    sta $09
    lda $04
    sta $0A

    stz $02             ; $02 = digit count (__zp_scratch)
    ldy #0              ; Y = write index into output

    ; --- Validate base (2-36) ---
    lda $08
    bne @valid
    lda $07
    cmp #2
    bcs @valid
    ; base < 2: write "0\0" and return
    lda #$30
    sta ($05),y
    iny
    lda #0
    sta ($05),y
    lda $05
    ldx $06
    rts

@valid:
    ; --- Handle negative for base 10 only ---
    lda $07
    cmp #10
    bne @poscheck
    lda $08
    bne @poscheck
    ; base == 10: check sign bit
    lda $0A
    bpl @poscheck
    ; Negative: write '-' and negate
    lda #$2d
    sta ($05),y
    iny
    sec
    lda #0
    sbc $09
    sta $09
    lda #0
    sbc $0A
    sta $0A
@poscheck:

    ; --- Check for zero ---
    lda $09
    ora $0A
    bne @extract
    lda #$30
    sta ($05),y
    iny
    bra @nulterm

    ; --- Extract digits: divide, push remainder as char ---
@extract:
    ; Divide value by base using hardware divider
    lda $09
    sta $d760
    lda $0A
    sta $d761
    lda #0
    sta $d762
    sta $d763
    lda $07
    sta $d764
    lda $08
    sta $d765
    lda #0
    sta $d766
    sta $d767

@waitdiv:
    lda $d7fe
    bmi @waitdiv

    ; Convert remainder to character
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

    ; Quotient becomes new value
    lda $d768
    sta $09
    lda $d769
    sta $0A

    ; Continue while value != 0
    ora $09
    bne @extract

    ; --- Pop digits into output buffer (correct order) ---
@poploop:
    pla
    sta ($05),y
    iny
    dec $02
    bne @poploop

@nulterm:
    lda #0
    sta ($05),y

    lda $05
    ldx $06
    rts
    endproc
