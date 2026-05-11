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
;   ZP usage: $02-$06 (saved/restored to stack)

.global _itoa
.extern __sp_base

.segment "code"

proc _itoa, W#_p_value, W#_p_str, W#_p_base
    .var _fp = 0
    ; Save ZP $02-$06 to stack
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

    ldax _p_value+5, s
    stax $02
    ldax _p_base+5, s
    stax $04

    stz $06             ; digit count on stack
    ldy #0              ; Y = write index into output

    ; --- Validate base (2-36) ---
    lda $05
    bne @valid          ; high byte nonzero => base >= 256, ok
    lda $04
    cmp #2
    bcs @valid
    ; base < 2: write "0\0" and return
    lda #$30
    sta (_p_str+5, sp),y
    iny
    lda #0
    sta (_p_str+5, sp),y
    ldax _p_str+5, s
    bra @restore

@valid:
    ; --- Handle negative for base 10 only ---
    lda $04
    cmp #10
    bne @poscheck
    lda $05
    bne @poscheck
    ; base == 10: check sign bit
    lda $03
    bpl @poscheck
    ; Negative: write '-' and negate
    lda #$2d
    sta (_p_str+5, sp),y
    iny
    sec
    lda #0
    sbc $02
    sta $02
    lda #0
    sbc $03
    sta $03
@poscheck:

    ; --- Check for zero ---
    lda $02
    ora $03
    bne @extract
    lda #$30
    sta (_p_str+5, sp),y
    iny
    bra @nulterm

    ; --- Extract digits: divide, push remainder as char ---
@extract:
    ; Divide value by base using hardware divider
    lda $02
    sta $d760
    lda $03
    sta $d761
    lda #0
    sta $d762
    sta $d763
    lda $04
    sta $d764
    lda $05
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
    adc #$30            ; '0'
    bra @pushdig
@hexdig:
    sec
    sbc #10
    clc
    adc #$41            ; PETSCII lowercase 'a'
@pushdig:
    pha                 ; push digit char onto hardware stack
    inc $06             ; count it

    ; Quotient becomes new value
    lda $d768
    sta $02
    lda $d769
    sta $03

    ; Continue while value != 0
    ora $02
    bne @extract

    ; --- Pop digits into output buffer (correct order) ---
@poploop:
    pla
    sta (_p_str+5, sp),y
    iny
    dec $06
    bne @poploop

@nulterm:
    lda #0
    sta (_p_str+5, sp),y

    ldax _p_str+5, s
@restore:
    ; Return value in AX is preserved; PLZ only clobbers Z and flags.
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
