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
    ldax _p_str+7, sp
    stax $07                ; $07/$08 = output buffer pointer
    ldax _p_base+7, sp
    stax $04                ; $04/$05 = base
    ldax _p_value+7, sp
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
    ; MEGA65 math unit registers (mmsim version):
    ;   DIVIDEND ($D760-$D763)
    ;   DIVISOR  ($D764-$D767)
    ;   DIVOUT whole quotient at $D768-$D76B
    ;   DIVOUT remainder at $D770-$D773 (aliased with MULTINA)
    ;   DIVBUSY at $D70F bit 7
@extract:
    ; Load numerator (value) into DIVIDEND ($D760-$D763)
    lda $02
    sta $d760
    lda $03
    sta $d761
    lda #0
    sta $d762
    sta $d763
    ; Load denominator (base) into DIVISOR ($D764-$D767)
    lda $04
    sta $d764
    lda $05
    sta $d765
    lda #0
    sta $d766
    sta $d767

@waitdiv:
    bit $d70f
    bne @waitdiv

    ; Read quotient from DIVOUT whole ($D768/$D769).
    ; Store quotient to $02/$03 (becomes next iteration's value).
    lda $d768
    sta $02             ; quotient lo → new value lo
    lda $d769
    sta $03             ; quotient hi → new value hi

    ; Read remainder from DIVOUT remainder ($D770).
    lda $d770           ; A = remainder

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

    ; Continue while value != 0
    lda $03
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
