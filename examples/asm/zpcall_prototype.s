; zpcall_prototype.s — Prototype of ZP parameter block calling convention
;
; Demonstrates:
;   1. Parameter passing via ZP $03-$12
;   2. Nested calls with caller-save
;   3. Long (32-bit) return in AXYZ
;   4. ltoa-style conversion using ZP params
;
; Expected result at $4000: "42" (ASCII $34 $32 $00)
; Expected result at $4010: "100" (ASCII $31 $30 $30 $00)

.org $2000
__sp_base = $0101
__zp_scratch = $02

    ; === Test 1: Simple call — multiply_by_two(21) → 42, then itoa ===
    ; Set up param: long value = 21 at $03-$06
    lda #21
    sta $03
    lda #0
    sta $04
    sta $05
    sta $06
    jsr multiply_by_two
    ; Result in AXYZ = 42
    ; Now convert to string: set up params for simple_ltoa
    ; $03-$06 = value (already has result from multiply? No — we need to store AXYZ)
    sta $03
    stx $04
    sty $05
    stz $06
    ; $07/$08 = output buffer pointer
    lda #<$4000
    sta $07
    lda #>$4000
    sta $08
    jsr simple_ltoa

    ; === Test 2: Nested call — add_and_convert(50, 50) ===
    ; $03-$06 = first long (50)
    lda #50
    sta $03
    lda #0
    sta $04
    sta $05
    sta $06
    ; $07-$0A = second long (50)
    lda #50
    sta $07
    lda #0
    sta $08
    sta $09
    sta $0a
    ; $0B/$0C = output buffer
    lda #<$4010
    sta $0b
    lda #>$4010
    sta $0c
    jsr add_and_convert

    rts

; ============================================================
; multiply_by_two(long val) → long result in AXYZ
; Params: $03-$06 = val (32-bit)
; Returns: AXYZ = val * 2
; ============================================================
multiply_by_two:
    ldq $03         ; load val from ZP
    aslq            ; * 2
    rts

; ============================================================
; add_and_convert(long a, long b, char *out)
; Params: $03-$06 = a, $07-$0A = b, $0B/$0C = out
; Demonstrates nested call with caller-save
; ============================================================
add_and_convert:
    ; Compute a + b
    ldq $03         ; AXYZ = a
    clc
    adcq $07        ; AXYZ = a + b

    ; Now we need to call simple_ltoa, which uses $03-$08
    ; Save our output pointer ($0B/$0C) to stack (it won't be clobbered
    ; since simple_ltoa uses $03-$08, but let's demonstrate the pattern)
    ; Actually simple_ltoa uses $03-$08, so set up params:
    ; $03-$06 = value to convert (currently in AXYZ)
    sta $03
    stx $04
    sty $05
    stz $06
    ; $07/$08 = output buffer (from our $0B/$0C param)
    lda $0b
    sta $07
    lda $0c
    sta $08
    jsr simple_ltoa
    rts

; ============================================================
; simple_ltoa(long val, char *buf)
; Params: $03-$06 = value, $07/$08 = output buffer pointer
; Minimal unsigned decimal conversion (no negative handling)
; Uses $09 = digit count, hardware divider
; ============================================================
simple_ltoa:
    ldy #0          ; Y = output index
    stz $09         ; digit count

    ; Check for zero
    lda $03
    ora $04
    ora $05
    ora $06
    bne @extract
    lda #$30        ; '0'
    sta ($07),y
    iny
    bra @done

@extract:
    ; Divide value by 10 using hardware divider
    lda $03
    sta $d760
    lda $04
    sta $d761
    lda $05
    sta $d762
    lda $06
    sta $d763
    lda #10
    sta $d764
    lda #0
    sta $d765
    sta $d766
    sta $d767

    ; Remainder → digit char
    lda $d770
    clc
    adc #$30        ; + '0'
    pha             ; push digit (reverse order)
    inc $09

    ; Quotient → new value
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

    ; Pop digits into buffer (correct order)
@pop:
    pla
    sta ($07),y
    iny
    dec $09
    bne @pop

@done:
    lda #0
    sta ($07),y     ; NUL terminate
    rts
