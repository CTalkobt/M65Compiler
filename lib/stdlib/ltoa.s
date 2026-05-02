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
;   ZP usage: $02-$0A

.global _ltoa
.extern __sp_base

.segment "code"

proc _ltoa, D#_p_value, W#_p_str, W#_p_base
    .var _fp = 0

    ldax _p_str, s
    stax $02            ; $02/$03 = output pointer
    ; Load 32-bit value into $04-$07
    lda _p_value, s
    sta $04
    lda _p_value+1, s
    sta $05
    lda _p_value+2, s
    sta $06
    lda _p_value+3, s
    sta $07
    ldax _p_base, s
    stax $08            ; $08/$09 = base

    stz $0a             ; $0a = digit count on stack
    ldy #0              ; Y = write index into output

    ; --- Validate base (2-36) ---
    lda $09
    bne @valid          ; high byte nonzero => base >= 256, ok
    lda $08
    cmp #2
    bcs @valid
    ; base < 2: write "0\0" and return
    lda #$30
    sta ($02),y
    iny
    lda #0
    sta ($02),y
    ldax _p_str, s
    rtn #0

@valid:
    ; --- Handle negative for base 10 only ---
    lda $08
    cmp #10
    bne @poscheck
    lda $09
    bne @poscheck
    ; base == 10: check sign bit (byte 3)
    lda $07
    bpl @poscheck
    ; Negative: write '-' and negate 32-bit value
    lda #$2d
    sta ($02),y
    iny
    sec
    lda #0
    sbc $04
    sta $04
    lda #0
    sbc $05
    sta $05
    lda #0
    sbc $06
    sta $06
    lda #0
    sbc $07
    sta $07
@poscheck:

    ; --- Check for zero ---
    lda $04
    ora $05
    ora $06
    ora $07
    bne @extract
    lda #$30
    sta ($02),y
    iny
    bra @nulterm

    ; --- Extract digits: divide 32-bit value by base ---
@extract:
    ; Dividend: $D760-$D763 (32-bit)
    lda $04
    sta $d760
    lda $05
    sta $d761
    lda $06
    sta $d762
    lda $07
    sta $d763
    ; Divisor: $D764-$D767 (base, 16-bit zero-extended)
    lda $08
    sta $d764
    lda $09
    sta $d765
    lda #0
    sta $d766
    sta $d767

    ; Wait for divider (bit test $D70F)
    bit $d70f
    bne *-3

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
    inc $0a             ; count it

    ; Quotient ($D768-$D76B) becomes new value
    lda $d768
    sta $04
    lda $d769
    sta $05
    lda $d76a
    sta $06
    lda $d76b
    sta $07

    ; Continue while value != 0
    ora $04
    ora $05
    ora $06
    bne @extract

    ; --- Pop digits into output buffer (correct order) ---
@poploop:
    pla
    sta ($02),y
    iny
    dec $0a
    bne @poploop

@nulterm:
    lda #0
    sta ($02),y

    ldax _p_str, s
    rtn #0
    endproc
