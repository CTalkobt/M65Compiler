; Advanced tests for FILL and MOVE operations
    .org $2000
start:
    ; Test 1: FILL with various destination operand types
    lda #$42

    ; Immediate address
    FILL $1000, #256

    ; Register pairs - destination
    lda #$20
    ldx #$00
    FILL .AX, #100

    lda #$30
    ldy #$00
    FILL .AY, #100

    lda #$40
    ldz #$00
    FILL .AZ, #100

    ldx #$50
    ldy #$00
    FILL .XY, #100

    ldx #$60
    ldz #$00
    FILL .XZ, #100

    ldy #$70
    ldz #$00
    FILL .YZ, #100

    ; Test 2: FILL with various length operand types
    lda #$80
    FILL $2000, #512

    ldx #$02
    ldy #$00
    FILL $2100, .XY

    lda #$00
    ldx #$01
    FILL $2200, .AX

    ; Test 3: MOVE with various destination operand types
    ldx #$00
    ldy #$01

    ; Immediate to immediate
    MOVE $3000, $4000

    ; Register pair destination
    MOVE $5000, .AX

    ; Register pair source
    MOVE .AX, $6000

    lda #$50
    ldz #$00
    MOVE .AZ, $7000

    ; Test 4: Complex address values
    lda #$12
    ldx #$34
    FILL .AX, #$5678

    ; Test 5: Stack-relative operations (in function context)
    .function advanced_move_fill_test
    .var src_addr = 2
    .var dst_addr = 4
    .var fill_len = 6

    ; Stack-relative FILL
    FILL.SP $8000, #100

    ; Stack-relative MOVE
    ldx #$00
    ldy #$01
    MOVE.SP $9000, #200

    rts
    .endfunction

    ; Test 6: Symbols with relocation
    .data
    buffer1: .fill 256, 0
    buffer2: .fill 256, 0
    buffer3: .fill 256, 0
    .code

    FILL buffer1, #128
    MOVE buffer1, buffer2
    FILL buffer3, #256

    rts

    ; Test 7: MOVE with 3 operands (explicit length)
    ; Three immediate operands: MOVE src, dest, len
    MOVE $3000, $4000, #256
    MOVE $5000, $6000, #512

    ; Register pair as length
    ldx #10
    ldy #2
    MOVE $7000, $8000, .XY

    ; Register pair source with immediate length
    lda #$10
    ldx #$00
    MOVE .AX, $9000, #128

    ; Register pair destination with immediate length
    lda #$20
    ldy #$00
    MOVE $A000, .AY, #64

    ; Three operands with symbols
    MOVE buffer1, buffer2, #100

    rts
