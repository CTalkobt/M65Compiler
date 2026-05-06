.cpu _45gs02
.segmentOrder code, data, bss

.global _test

.code

_test:
    lda #$12
    sta table+4
    lda #$34
    sta table+5

@loop:
    jmp @loop

table:
    .byte $81
    .byte $00,$00,$00
    .word $0000
    .byte $00,$00,$00
    .word $0000
    .byte $00
    .word $0000