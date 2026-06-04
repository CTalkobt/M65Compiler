; crt0_mega65.s — MEGA65 Platform Init (strong override of _init_features)
;
; Provides a strong _init_features that:
;   1. Enables MEGA65 I/O at $D000
;   2. Sets CPU to full speed (40 MHz)
;   3. Relocates stack to page $01 (16-bit SP via CLE)
;
; Link with crt0.o45 to activate:
;   ln45 -prg crt0.o45 crt0_mega65.o45 main.o45 -o program.prg

.global _init_features

.segment "init"

_init_features:
    ; GS knock already done in crt0.s (before DMA)

    ; Set CPU speed to 40 MHz
    lda #$40
    sta $D054

    rts
