; ltoa.s — Convert 32-bit long to string
;
; char *ltoa(long value, char *str, int base);
;   Converts a 32-bit integer to a NUL-terminated string in the
;   given base (2-36). For base 10, negative values get a '-' prefix.
;   Returns str.
;
;   Strategy: divide repeatedly by base using MEGA65 hardware divider
;   (full 32-bit), push digit chars onto hardware stack in reverse,
;   then pop them into the output buffer in correct order.
;
;   Uses MEGA65 hardware divider at $D760-$D773.
;   ZP usage: $02-$08 (saved/restored to stack)

.global _ltoa
.extern __sp_base

.segment "code"

proc _ltoa, D#_p_value, W#_p_str, W#_p_base
    .var _fp = 0
    ; Save ZP $02-$08
    lda $02
    pha
    lda $03
    pha
    lda $04
    pha
    lda $05
    pha
    lda $06
    pha
    lda $07
    pha
    lda $08
    pha

    ; Load 32-bit value into $02-$05
    lda _p_value+7, s
    sta $02
    lda _p_value+8, s
    sta $03
    lda _p_value+9, s
    sta $04
    lda _p_value+10, s
    sta $05
    ldax _p_base+7, s
    stax $06

    stz $08             ; digit count on stack
    ldy #0              ; Y = write index into output

    ; --- Validate base (2-36) ---
    lda $07
    bne @valid          ; high byte nonzero => base >= 256, ok
    lda $06
    cmp #2
    bcs @valid
    ; base < 2: write "0\0" and return
    lda #$30
    sta (_p_str+8, sp),y
    iny
    lda #0
    sta (_p_str+8, sp),y
    ldax _p_str+7, s
    bra @restore

@valid:
    ; --- Handle negative for base 10 only ---
    lda $06
    cmp #10
    bne @poscheck
    lda $07
    bne @poscheck
    ; base == 10: check sign bit (byte 3)
    lda $05
    bpl @poscheck
    ; Negative: write '-' and negate 32-bit value
    lda #$2d
    sta (_p_str+8, sp),y
    iny
    sec
    lda #0
    sbc $02
    sta $02
    lda #0
    sbc $03
    sta $03
    lda #0
    sbc $04
    sta $04
    lda #0
    sbc $05
    sta $05
@poscheck:

    ; --- Check for zero ---
    lda $02
    ora $03
    ora $04
    ora $05
    bne @extract
    lda #$30
    sta (_p_str+8, sp),y
    iny
    bra @nulterm

    ; --- Extract digits: divide 32-bit value by base ---
@extract:
    ; Dividend: $D760-$D763 (32-bit)
    lda $02
    sta $d760
    lda $03
    sta $d761
    lda $04
    sta $d762
    lda $05
    sta $d763
    ; Divisor: $D764-$D767 (base, 16-bit zero-extended)
    lda $06
    sta $d764
    lda $07
    sta $d765
    lda #0
    sta $d766
    sta $d767

@waitdiv:
    lda $d7fe
    bmi @waitdiv

    ; Convert remainder ($D770) to character
    lda $d770
    cmp #10
    bcs @hexdig
    clc
    adc #$30            ; '0'
    bra @pushdig
@hexdig:
    sec
    sbc #10
    clc
    adc #$41            ; PETSCII lowercase 'a'
@pushdig:
    pha                 ; push digit char onto hardware stack
    inc $08             ; count it

    ; Quotient ($D768-$D76B) becomes new value
    lda $d768
    sta $02
    lda $d769
    sta $03
    lda $d76a
    sta $04
    lda $d76b
    sta $05

    ; Continue while value != 0
    ora $02
    ora $03
    ora $04
    bne @extract

    ; --- Pop digits into output buffer (correct order) ---
@poploop:
    pla
    sta (_p_str+8, sp),y
    iny
    dec $08
    bne @poploop

@nulterm:
    lda #0
    sta (_p_str+8, sp),y

    ldax _p_str+7, s
@restore:
    ; Return value in AX preserved (PLZ doesn't touch A/X).
    plz
    stz $08
    plz
    stz $07
    plz
    stz $06
    plz
    stz $05
    plz
    stz $04
    plz
    stz $03
    plz
    stz $02
    rts
    endproc
