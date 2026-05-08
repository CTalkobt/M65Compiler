; palette_fade_asm.s — VIC-IV palette manipulation for MEGA65
;
; Four assembly functions exported to C:
;   _unlock_viciv()          — Unlock VIC-IV palette registers
;   _save_palette()          — Save current palette (colors 0-15) to RAM
;   _restore_palette()       — Restore saved palette
;   _apply_fade(level)       — Scale palette colors by fade level (0-256)
;
; ZP calling convention:
;   Input:  Parameters in zero page ($02+)
;   Output: 16-bit result in A,X

.cpu _45gs02

.global _unlock_viciv
.global _save_palette
.global _restore_palette
.global _apply_fade

; Palette storage at fixed RAM location: $0900-$0930
; 16 colors × 3 bytes each = 48 bytes
.define PALETTE_BASE $0900

.segment "code"

; _unlock_viciv() — Write $47, $53 to $D02F to unlock VIC-IV
_unlock_viciv:
    lda #$47
    sta $D02F
    lda #$53
    sta $D02F
    rts


; _save_palette() — Read colors 0-15 from palette registers
; Stores at $0900: [R0, G0, B0, R1, G1, B1, ..., R15, G15, B15]
_save_palette:
    ldx #0
@save_loop:
    lda $D100,x
    sta PALETTE_BASE, x
    lda $D200,x
    sta PALETTE_BASE + 16, x
    lda $D300,x
    sta PALETTE_BASE + 32, x
    inx
    cpx #16
    bne @save_loop
    rts


; _restore_palette() — Write saved palette back to registers
_restore_palette:
    ldx #0
@restore_loop:
    lda PALETTE_BASE, x
    sta $D100,x
    lda PALETTE_BASE + 16, x
    sta $D200,x
    lda PALETTE_BASE + 32, x
    sta $D300,x
    inx
    cpx #16
    bne @restore_loop
    rts


; _apply_fade(unsigned char level) — Scale palette by fade level
; ZP calling convention: level parameter at $03
_apply_fade:
    ldx #0
@fade_loop:
    ; Process RED
    lda PALETTE_BASE, x     ; load original red
    sta $04                 ; store multiplicand
    lda $03                 ; load fade level (multiplier)

    ; Inline multiply: (A * $04) >> 8
    ldy #8
    sty $06
    lda #0
    sta $05
@red_mul:
    asl $05
    ror a
    bcc @red_skip
    lda $05
    clc
    adc $04
    sta $05
@red_skip:
    dec $06
    bne @red_mul
    lda $05
    sta $D100,x

    ; Process GREEN
    lda PALETTE_BASE + 16, x
    sta $04
    lda $03

    ; Inline multiply: (A * $04) >> 8
    ldy #8
    sty $06
    lda #0
    sta $05
@green_mul:
    asl $05
    ror a
    bcc @green_skip
    lda $05
    clc
    adc $04
    sta $05
@green_skip:
    dec $06
    bne @green_mul
    lda $05
    sta $D200,x

    ; Process BLUE
    lda PALETTE_BASE + 32, x
    sta $04
    lda $03

    ; Inline multiply: (A * $04) >> 8
    ldy #8
    sty $06
    lda #0
    sta $05
@blue_mul:
    asl $05
    ror a
    bcc @blue_skip
    lda $05
    clc
    adc $04
    sta $05
@blue_skip:
    dec $06
    bne @blue_mul
    lda $05
    sta $D300,x

    inx
    cpx #16
    bne @fade_loop
    rts
