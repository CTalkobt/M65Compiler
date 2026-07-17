; Test: 16-bit stack pointer via TYS (SPH) / TSY
; Relocate stack to page $40, push known values, verify they land at $40xx.
; Results written to $4000: DD CC BB AA 40 FF
.org $2000

    ; Set SPL to $FF
    ldx #$FF
    txs

    ; Relocate stack to page $40 via TYS
    ldy #$40
    tys             ; SPH = $40, SP = $40FF

    ; Push known test values
    lda #$AA
    pha             ; → $40FF
    lda #$BB
    pha             ; → $40FE
    lda #$CC
    pha             ; → $40FD
    lda #$DD
    pha             ; → $40FC

    ; PLA back in LIFO order
    pla
    sta $4000       ; expect $DD
    pla
    sta $4001       ; expect $CC
    pla
    sta $4002       ; expect $BB
    pla
    sta $4003       ; expect $AA

    ; TSY should read back SPH = $40
    tsy
    sty $4004       ; expect $40

    ; TSX: SPL should be $FF (balanced push/pull)
    tsx
    stx $4005       ; expect $FF

    brk
