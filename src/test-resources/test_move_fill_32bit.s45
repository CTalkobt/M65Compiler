; Test 32-bit (28-bit) MOVE and FILL operations
; Tests MOVE/FILL with:
;   - 32-bit numeric literals (e.g., $08000000 for Attic)
;   - 32-bit register quads (.AXYZ, .Q)
;   - 24-bit registers zero-extended to 32-bit (.AXY, .AXZ, .AYZ)
;   - 32-bit symbol values
;   - Mixed 16-bit and 32-bit operands

.cpu _45gs02
.segmentOrder code, data, bss

.global test_move_fill_32bit

.code

; Test 1: MOVE with 32-bit Attic address literal as source
test_move_32bit_src_literal:
    ; Copy from Attic ($08000000) to RAM ($4000), 256 bytes
    MOVE $08000000, $4000, #256
    rts

; Test 2: MOVE with 32-bit Attic address literal as destination
test_move_32bit_dst_literal:
    ; Copy from RAM ($3000) to Attic ($08000000), 512 bytes
    MOVE $3000, $08000000, #512
    rts

; Test 3: FILL with 32-bit Attic address
test_fill_32bit_literal:
    lda #$AA
    ; Fill Attic buffer with $AA, 256 bytes
    FILL $08000000, #256
    rts

; Test 4: MOVE with .AXYZ (4-byte register quad) as source
test_move_axyz_src:
    ; Set address in .AXYZ = $08000000
    lda #$00
    ldx #$00
    ldy #$00
    ldz #$08
    ; Copy from .AXYZ to $4000, 128 bytes
    MOVE .AXYZ, $4000, #128
    rts

; Test 5: MOVE with .AXYZ as destination
test_move_axyz_dst:
    ; Set address in .AXYZ = $08001000
    lda #$00
    ldx #$00
    ldy #$10
    ldz #$08
    ; Copy from $3000 to .AXYZ, 256 bytes
    MOVE $3000, .AXYZ, #256
    rts

; Test 6: MOVE with .AXYZ as length (32-bit length)
test_move_axyz_len:
    lda #$00
    ldx #$00
    ldy #$01
    ldz #$00
    ; Copy with length in .AXYZ (256 bytes)
    MOVE $3000, $5000, .AXYZ
    rts

; Test 7: FILL with .AXY (24-bit, zero-extended to 32-bit)
test_fill_axy_24bit:
    ; Set address in .AXY = $08 << 16 | $0000 = $08000000
    lda #$00
    ldx #$00
    ldy #$08
    FILL .AXY, #512
    rts

; Test 8: FILL with .AXZ (24-bit)
test_fill_axz_24bit:
    lda #$00
    ldx #$00
    ldz #$08
    FILL .AXZ, #256
    rts

; Test 9: FILL with .AYZ (24-bit)
test_fill_ayz_24bit:
    lda #$00
    ldy #$00
    ldz #$08
    FILL .AYZ, #256
    rts

; Test 10: MOVE with .Q register (synonym for .AXYZ)
test_move_q_register:
    lda #$00
    ldx #$00
    ldy #$08
    ldz #$00
    MOVE .Q, $6000, #128
    rts

; Test 11: 28-bit Attic address (common case)
test_move_28bit_attic:
    ; $0FFFFFF = 28-bit mask, test within typical Attic range
    MOVE $08100000, $4000, #256
    rts

; Test 12: MOVE with 32-bit symbol source
test_move_32bit_symbol_src:
    MOVE attic_symbol, $4000, #100
    rts

; Test 13: FILL with 32-bit symbol
test_fill_32bit_symbol:
    lda #$BB
    FILL attic_symbol, #200
    rts

; Test 14: Mixed 16-bit source, 32-bit destination
test_move_16bit_src_32bit_dst:
    MOVE $2000, $08002000, #256
    rts

; Test 15: Register pair in source, 32-bit literal destination
test_move_az_32bit_dst:
    lda #$30
    ldz #$00
    MOVE .AZ, $08003000, #128
    rts

.data

; 32-bit symbols for testing symbol relocation
attic_symbol = $08000000
attic_symbol2 = $08010000
ram_symbol = $4000

; Buffer for testing
test_buffer: .fill 256, 0

.code

; Entry point: run all tests
test_move_fill_32bit:
    ; Could call individual tests here
    ; For now, just load the program and each test can be invoked
    rts
