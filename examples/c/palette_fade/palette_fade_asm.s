; palette_fade_asm.s — VIC-IV palette manipulation for MEGA65
;
; Four assembly functions exported to C:
;   _unlock_viciv()          — Unlock VIC-IV palette registers
;   _save_palette()          — Save current palette (colors 0-15) to BSS
;   _restore_palette()       — Restore saved palette
;   _apply_fade(level)       — Scale palette colors by fade level (0-256)
;
; ZP calling convention:
;   Input:  Parameters in zero page ($10+)
;   Output: 16-bit result in A,X

.cpu _45gs02

.global _unlock_viciv
.global _save_palette
.global _restore_palette
.global _apply_fade

; Palette buffer at $A000-$A02F (safe high RAM, doesn't conflict with BASIC)
; $0800-$0BFF: Screen RAM
; $1000-$1FFF: Sprite/bitmap area
; $2000+: BASIC program area
; $A000-$BFFF: Safe for custom data

.segment "code"

; _unlock_viciv() — Write $47, $53 to $D02F to unlock VIC-IV
_unlock_viciv:
    lda #$47
    sta $D02F
    lda #$53
    sta $D02F
    rts


; _save_palette() — Read colors 0-15 from palette registers
; Stores in BSS buffer (palette_buffer)
; Uses absolute indexed addressing to avoid clobbering ZP
_save_palette:
    ldx #0
@save_loop:
    lda $D100,x
    sta $A000, x              ; RED offset 0
    lda $D200,x
    sta $A000 + 16, x         ; GREEN offset 16
    lda $D300,x
    sta $A000 + 32, x         ; BLUE offset 32
    inx
    cpx #16
    bne @save_loop
    rts


; _restore_palette() — Write saved palette back to registers
; Reads from BSS buffer (palette_buffer)
; Uses absolute indexed addressing to avoid clobbering ZP
_restore_palette:
    ldx #0
@restore_loop:
    lda $A000, x              ; RED offset 0
    sta $D100,x
    lda $A000 + 16, x         ; GREEN offset 16
    sta $D200,x
    lda $A000 + 32, x         ; BLUE offset 32
    sta $D300,x
    inx
    cpx #16
    bne @restore_loop
    rts


; _apply_fade(unsigned char level) — Scale palette by fade level
; ZP calling convention: level parameter at $10
; Reads from BSS buffer (palette_buffer)
; Uses ZP: $10 (parameter), $04-$07 (temp storage)
_apply_fade:
    ldx #0
@fade_loop:
    ; Process RED
    lda $A000, x              ; load original red
    sta $04                   ; store multiplicand at $04
    lda $10                   ; load fade level into A
    sta $07                   ; SAVE multiplier at $07

    ; Inline multiply: ($04 * $07) >> 8
    ; Using shift-and-add: shift multiplier right, add multiplicand when bit set
    lda #0                    ; A = accumulator (result)
    ldy #8                    ; Y = loop counter
@red_mul:
    lsr $07                   ; shift multiplier right, carry = current bit
    bcc @red_skip             ; if bit was 0, skip add
    clc
    adc $04                   ; add multiplicand to result
@red_skip:
    ror a                     ; rotate result right (arithmetic shift)
    dey
    bne @red_mul
    sta $D100,x               ; write result to RED register

    ; Process GREEN
    lda $A000 + 16, x         ; load original green
    sta $04
    lda $10                   ; load fade level
    sta $07                   ; save multiplier

    lda #0
    ldy #8
@green_mul:
    lsr $07
    bcc @green_skip
    clc
    adc $04
@green_skip:
    ror a
    dey
    bne @green_mul
    sta $D200,x

    ; Process BLUE
    lda $A000 + 32, x         ; load original blue
    sta $04
    lda $10                   ; load fade level
    sta $07                   ; save multiplier

    lda #0
    ldy #8
@blue_mul:
    lsr $07
    bcc @blue_skip
    clc
    adc $04
@blue_skip:
    ror a
    dey
    bne @blue_mul
    sta $D300,x

    inx
    cpx #16
    bne @fade_loop
    rts
