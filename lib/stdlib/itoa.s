; itoa.s — Convert integer to string
;
; char *itoa(int value, char *str, int base);
;   Converts a 16-bit integer to a NUL-terminated string in the
;   given base (2-36). For base 10, negative values get a '-' prefix.
;   Returns str.
;
;   Strategy: divide repeatedly by base, push digit chars onto the
;   hardware stack (producing them in reverse), then pop them into
;   the output buffer in the correct order.
;
;   Uses MEGA65 hardware divider at $D760-$D771.
;   ZP usage: $02-$08

.global _itoa
.extern __sp_base

.segment "code"

proc _itoa, W#_p_value, W#_p_str, W#_p_base
    .var _fp = 0

    ldax _p_str, s
    stax $02            ; $02/$03 = output pointer
    ldax _p_value, s
    stax $04            ; $04/$05 = value
    ldax _p_base, s
    stax $06            ; $06/$07 = base

    stz $08             ; $08 = digit count on stack
    ldy #0              ; Y = write index into output

    ; --- Validate base (2-36) ---
    lda $07
    bne @valid          ; high byte nonzero => base >= 256, ok
    lda $06
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
    lda $06
    cmp #10
    bne @poscheck
    lda $07
    bne @poscheck
    ; base == 10: check sign bit
    lda $05
    bpl @poscheck
    ; Negative: write '-' and negate
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
@poscheck:

    ; --- Check for zero ---
    lda $04
    ora $05
    bne @extract
    lda #$30
    sta ($02),y
    iny
    bra @nulterm

    ; --- Extract digits: divide, push remainder as char ---
@extract:
    ; Divide value by base using hardware divider
    ; Dividend: $D760-$D763, Divisor: $D764-$D767
    ; Quotient: $D768-$D76B, Remainder: $D770-$D773
    lda $04
    sta $d760
    lda $05
    sta $d761
    lda #0
    sta $d762
    sta $d763
    lda $06
    sta $d764
    lda $07
    sta $d765
    lda #0
    sta $d766
    sta $d767

    ; Convert remainder to character
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

    ; Quotient becomes new value
    lda $d768
    sta $04
    lda $d769
    sta $05

    ; Continue while value != 0
    ora $04
    bne @extract

    ; --- Pop digits into output buffer (correct order) ---
@poploop:
    pla
    sta ($02),y
    iny
    dec $08
    bne @poploop

@nulterm:
    lda #0
    sta ($02),y

    ldax _p_str, s
    rtn #0
    endproc
