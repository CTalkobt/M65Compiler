.cpu _45gs02
.segmentOrder code, data, bss

.global _test

.code

_test:
    lda #$00
    sta state
    sta flag

    jsr setup

@mainloop:
    jsr getcmd

    lda err
    bne @done

    lda state
    cmp #$01
    beq @vertex
    cmp #$02
    beq @face

    jmp @mainloop

@vertex:
    jsr dovtx
    jmp @mainloop

@face:
    lda flag
    bne @skipinit

    jsr initface
    lda #$01
    sta flag

@skipinit:
    jsr doface
    jmp @mainloop

@done:
    jsr cleanup
    rts


setup:
    lda #$00
    sta err
    rts

getcmd:
    lda state
    eor #$01
    sta state
    rts

dovtx:
    lda #$11
    sta $0400
    rts

initface:
    lda #$22
    sta $0401
    rts

doface:
    lda #$33
    sta $0402
    rts

cleanup:
    lda #$44
    sta $0403
    rts


.bss

state: .res 1
flag:  .res 1
err:   .res 1