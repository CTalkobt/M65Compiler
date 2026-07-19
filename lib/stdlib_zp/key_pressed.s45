; key_pressed.s — Direct keyboard matrix scan (ZP calling convention)
;
; __regparm unsigned char key_pressed(unsigned char keycode);
;   keycode = (column << 3) | row  (see KEY_* constants in mega65.h)
;   Returns 1 if key is pressed, 0 if not.
;   Scans CIA1 keyboard matrix directly (no KERNAL).
;
; First param arrives in A via __regparm convention.

.global _key_pressed

.segment "code"

proc _key_pressed
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z

    ; Save keycode
    tay                 ; Y = keycode (preserve for row extraction)

    ; Extract column: (keycode >> 3) & 7
    lsr
    lsr
    lsr
    and #$07
    tax                 ; X = column number (0-7)

    ; Select column: write ~(1 << col) to CIA1 Port A ($DC00)
    lda @bit_table,x
    eor #$FF            ; invert: all bits set except column bit
    sta $DC00

    ; Extract row: keycode & 7
    tya                 ; A = original keycode
    and #$07
    tax                 ; X = row number (0-7)

    ; Read row data from CIA1 Port B ($DC01)
    ; Bits are active-low: 0 = pressed
    lda $DC01
    eor #$FF            ; invert: now 1 = pressed
    and @bit_table,x    ; test the row bit
    beq @not_pressed
    lda #1
    rts
@not_pressed:
    lda #0
    rts

@bit_table:
    .byte $01, $02, $04, $08, $10, $20, $40, $80

    .func_flags zp_call, leaf
    endproc
