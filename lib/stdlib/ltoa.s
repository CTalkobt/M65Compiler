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
;   ZP usage: $0B-$13 (avoids compiler ZP pool at $02-$0A)

.global _ltoa
.extern __sp_base

.segment "code"

proc _ltoa, D#_p_value, W#_p_str, W#_p_base
    .var _fp = 0

    ldax _p_str, s
    stax $0b            ; $0b/$0c = output pointer (avoid __zp_scratch at $02)
    ; Load 32-bit value into $04-$07
    lda _p_value, s
    sta $0d
    lda _p_value+1, s
    sta $0e
    lda _p_value+2, s
    sta $0f
    lda _p_value+3, s
    sta $10
    ldax _p_base, s
    stax $11            ; $08/$09 = base

    stz $13             ; $0a = digit count on stack
    ldy #0              ; Y = write index into output

    ; --- Validate base (2-36) ---
    lda $12
    bne @valid          ; high byte nonzero => base >= 256, ok
    lda $11
    cmp #2
    bcs @valid
    ; base < 2: write "0\0" and return
    lda #$30
    sta ($0b),y
    iny
    lda #0
    sta ($0b),y
    ldax _p_str, s
    rtn #0

@valid:
    ; --- Handle negative for base 10 only ---
    lda $11
    cmp #10
    bne @poscheck
    lda $12
    bne @poscheck
    ; base == 10: check sign bit (byte 3)
    lda $10
    bpl @poscheck
    ; Negative: write '-' and negate 32-bit value
    lda #$2d
    sta ($0b),y
    iny
    sec
    lda #0
    sbc $0d
    sta $0d
    lda #0
    sbc $0e
    sta $0e
    lda #0
    sbc $0f
    sta $0f
    lda #0
    sbc $10
    sta $10
@poscheck:

    ; --- Check for zero ---
    lda $0d
    ora $0e
    ora $0f
    ora $10
    bne @extract
    lda #$30
    sta ($0b),y
    iny
    bra @nulterm

    ; --- Extract digits: divide 32-bit value by base ---
@extract:
    ; Dividend: $D760-$D763 (32-bit)
    lda $0d
    sta $d760
    lda $0e
    sta $d761
    lda $0f
    sta $d762
    lda $10
    sta $d763
    ; Divisor: $D764-$D767 (base, 16-bit zero-extended)
    lda $11
    sta $d764
    lda $12
    sta $d765
    lda #0
    sta $d766
    sta $d767

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
    inc $13             ; count it

    ; Quotient ($D768-$D76B) becomes new value
    lda $d768
    sta $0d
    lda $d769
    sta $0e
    lda $d76a
    sta $0f
    lda $d76b
    sta $10

    ; Continue while value != 0
    ora $0d
    ora $0e
    ora $0f
    bne @extract

    ; --- Pop digits into output buffer (correct order) ---
@poploop:
    pla
    sta ($0b),y
    iny
    dec $13
    bne @poploop

@nulterm:
    lda #0
    sta ($0b),y

    ldax _p_str, s
    rtn #0
    endproc
