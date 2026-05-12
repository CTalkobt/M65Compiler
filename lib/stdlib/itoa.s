; itoa.s — Convert integer to string
;
; char *itoa(int value, char *str, int base);
;   Converts a 16-bit integer to a NUL-terminated string in the
;   given base (2-36). For base 10, negative values get a '-' prefix.
;   Returns str.
;
;   Strategy: divide repeatedly by base using MEGA65 hardware divider
;   (full 32-bit), push digit chars onto hardware stack in reverse,
;   then pop them into the output buffer in correct order.
;
;   Uses MEGA65 hardware divider at $D760-$D771.
;   ZP usage: $02-$08 (saved/restored to stack)
;   $02/$03 = value, $04/$05 = base, $06 = digit count
;   $07/$08 = output buffer pointer (loaded from str param)

.global _itoa
.extern __sp_base

.segment "code"

proc _itoa, W#_p_value, W#_p_str, W#_p_base
    .var _fp = 0
    ; Save ZP $02-$08 to stack
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

    ; Load params into ZP working registers.
    ; Load in reverse order since ldax uses $02 as scratch.
    ldax _p_str+7, s
    stax $07                ; $07/$08 = output buffer pointer
    ldax _p_base+7, s
    stax $04                ; $04/$05 = base
    ldax _p_value+7, s
    stax $02                ; $02/$03 = value

    stz $06                 ; digit count on stack
    ldy #0                  ; Y = write index into output

    ; --- Validate base (2-36) ---
    lda $05
    bne @valid              ; high byte nonzero => base >= 256, ok
    lda $04
    cmp #2
    bcs @valid
    ; base < 2: write "0\0" and return
    lda #$30
    sta ($07),y
    iny
    lda #0
    sta ($07),y
    ldax $07
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
    sta ($07),y
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
    sta ($07),y
    iny
    bra @nulterm

    ; --- Extract digits: divide, push remainder as char ---
    ; MEGA65 math unit registers:
    ;   MULTINA ($D770-$D773) = numerator / multiplier input A
    ;   MULTINB ($D774-$D777) = denominator / multiplier input B
    ;   DIVOUT whole quotient at $D76C-$D76F
    ;   MULTOUT product at $D778-$D77F (1-cycle, no wait)
    ;   DIVBUSY at $D70F bit 7
    ; Remainder = value - quotient * base (computed via multiplier).
@extract:
    ; Load numerator (value) into MULTINA ($D770-$D773)
    lda $02
    sta $d770
    lda $03
    sta $d771
    lda #0
    sta $d772
    sta $d773
    ; Load denominator (base) into MULTINB ($D774-$D777)
    lda $04
    sta $d774
    lda $05
    sta $d775
    lda #0
    sta $d776
    sta $d777

@waitdiv:
    bit $d70f
    bne @waitdiv

    ; Save quotient from DIVOUT whole ($D76C/$D76D) before overwriting MULTINA.
    ; Use $07/$08 as temp (within our saved ZP range).
    lda $d76c
    sta $07             ; quotient lo
    lda $d76d
    sta $08             ; quotient hi

    ; Compute quotient * base via multiplier to derive remainder:
    ; MULTINA = quotient, MULTINB still = base from above.
    lda $07
    sta $d770
    lda $08
    sta $d771
    lda #0
    sta $d772
    sta $d773
    ; MULTOUT ($D778) = quotient * base (available in 1 cycle, no wait)

    ; remainder = value - quotient*base (low byte only, always < base ≤ 36)
    sec
    lda $02
    sbc $d778

    ; Convert remainder to character
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
    lda $07
    sta $02
    lda $08
    sta $03

    ; Continue while value != 0
    ora $02
    bne @extract

    ; --- Pop digits into output buffer (correct order) ---
@poploop:
    pla
    sta ($07),y
    iny
    dec $06
    bne @poploop

@nulterm:
    lda #0
    sta ($07),y

    ldax $07                ; return str pointer
@restore:
    ; Return value in AX preserved; PLZ doesn't touch A/X.
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
