; Comprehensive test for addr_elem.16 and struct_elem.16 simulated opcodes
; Tests all documented API interfaces
; Output results to memory at 0x4000

* = $0801

; BASIC stub
    .byte $0B, $08, $0A, $00, $9E, $32, $30, $36, $31, $00, $00, $00

    .org $080D

; Test data
test_array:
    .byte $11, $22, $33, $44, $55, $66

; Global symbol for immediate addressing tests
global_data = $3000
zp_base = $20
zp_index = $22
zp_result = $24
zp_scratch = $26

; Output buffer
output = $4000

main:
    ; Initialize result buffer
    lda #0
    ldx #0
.clear_loop:
    sta output, x
    inx
    cpx #$30
    bne .clear_loop

    ldy #0  ; Result index

    ; ===== TEST 1: struct_elem.16 with global constant base, constant offset =====
    ; Expected: Load from test_array + 2 offset
    struct_elem.16 zp_result, #test_array, #2
    lda (zp_result)
    sta output, y  ; Result 0: should be 0x33 (test_array[1])
    iny

    ; ===== TEST 2: struct_elem.16 with ZP base, constant offset =====
    ; Set up ZP base pointing to test_array
    lda #<test_array
    ldx #>test_array
    sta zp_base
    stx zp_base+1

    struct_elem.16 zp_result, zp_base, #1
    lda (zp_result)
    sta output, y  ; Result 1: should be 0x22 (test_array[0] + 1 byte)
    iny

    ; ===== TEST 3: struct_elem.16 with register base (.AX), constant offset =====
    ; Load base into AX
    lda #<test_array
    ldx #>test_array
    struct_elem.16 zp_result, .AX, #3
    lda (zp_result)
    sta output, y  ; Result 2: should be 0x44 (test_array[1] + 1 byte offset within word)
    iny

    ; ===== TEST 4: struct_elem.16 with zero offset =====
    lda #<test_array
    ldx #>test_array
    struct_elem.16 zp_result, .AX, #0
    lda (zp_result)
    sta output, y  ; Result 3: should be 0x11 (test_array[0])
    iny

    ; ===== TEST 5: struct_elem.16 with larger offset =====
    lda #<test_array
    ldx #>test_array
    struct_elem.16 zp_result, .AX, #4
    lda (zp_result)
    sta output, y  ; Result 4: should be 0x55 (test_array[2])
    iny

    ; ===== TEST 6: addr_elem.16 with constant index, stride=1 =====
    ; Index = 2, stride = 1, base = test_array
    ; Expected address: test_array + 2*1 = test_array + 2
    lda #2
    ldx #0
    addr_elem.16 zp_result, #test_array, .AX, #1
    lda (zp_result)
    sta output, y  ; Result 5: should be 0x33 (test_array[2])
    iny

    ; ===== TEST 7: addr_elem.16 with register index, stride=1 =====
    lda #3
    ldx #0
    addr_elem.16 zp_result, #test_array, .AX, #1
    lda (zp_result)
    sta output, y  ; Result 6: should be 0x44 (test_array[3])
    iny

    ; ===== TEST 8: addr_elem.16 with ZP index, stride=1 =====
    lda #1
    sta zp_index
    lda #0
    sta zp_index+1
    addr_elem.16 zp_result, #test_array, zp_index, #1
    lda (zp_result)
    sta output, y  ; Result 7: should be 0x22 (test_array[1])
    iny

    ; ===== TEST 9: addr_elem.16 with stride=2 (16-bit elements) =====
    ; Create 16-bit array
    ; Index = 1, stride = 2, base = global_data (must have data there)
    lda #1
    ldx #0
    addr_elem.16 zp_result, #global_data, .AX, #2
    ; Note: Result depends on data at global_data + 2
    sta output, y
    iny

    ; ===== TEST 10: addr_elem.16 with index=0 (edge case) =====
    lda #0
    ldx #0
    addr_elem.16 zp_result, #test_array, .AX, #1
    lda (zp_result)
    sta output, y  ; Result 9: should be 0x11 (test_array[0])
    iny

    ; ===== TEST 11: struct_elem.16 with ZP base and ZP destination =====
    lda #<test_array
    ldx #>test_array
    sta zp_base
    stx zp_base+1

    struct_elem.16 zp_result, zp_base, #2
    lda (zp_result)
    sta output, y  ; Result 10: should be 0x33
    iny

    ; ===== TEST 12: addr_elem.16 with stride=4 (32-bit elements) =====
    lda #1
    ldx #0
    addr_elem.16 zp_result, #global_data, .AX, #4
    sta output, y
    iny

    ; Mark end of test
    lda #$AA
    sta output + $1F

    rts

.end
