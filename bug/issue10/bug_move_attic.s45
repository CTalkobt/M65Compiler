.cpu _45gs02
.segmentOrder code, data, bss

.global _test

.code

_test:

    ; destination = $08000000 (Attic)
    lda #$00
    ldz #$08

    ; length = 16
    ldx #$10
    ldy #$00

    move .AZ, srcbuf, .XY

@loop:
    jmp @loop

.data

srcbuf:
    .byte $01,$02,$03,$04,$05,$06,$07,$08
    .byte $09,$0A,$0B,$0C,$0D,$0E,$0F,$10