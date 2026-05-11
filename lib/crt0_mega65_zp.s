; crt0_mega65_zp.s — MEGA65 Platform Init (ZP calling convention)
;
; Same as crt0_mega65.s — platform init doesn't use param passing.

.global _init_features

.segment "init"

_init_features:
    ; Enable MEGA65 I/O
    lda #$47        ; 'G'
    sta $D02F
    lda #$53        ; 'S'
    sta $D02F

    ; Set CPU speed to 40 MHz
    lda #$40
    sta $D054

    rts
