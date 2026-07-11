; Test mul.16 opcode directly

* = $2000

main:
    ; Test: 1 * 2 should give 2
    lda #1
    ldx #0
    mul.16 .AX, #2      ; Should set A=2, X=0
    sta $4000           ; Store result
    stx $4001

    ; Test: 100 * 2 should give 200 (0xC8)
    lda #100
    ldx #0
    mul.16 .AX, #2      ; Should set A=0xC8, X=0
    sta $4002           ; Store result
    stx $4003

    ; Test: 256 * 2 should give 512 (0x200)
    lda #0
    ldx #1
    mul.16 .AX, #2      ; Should set A=0, X=2
    sta $4004           ; Store result
    stx $4005

    ; Mark complete
    lda #0xAA
    sta $4006
    brk
