; Comprehensive test for addr_elem.16 and struct_elem.16 APIs
; All documented interfaces, with validation output to $4000

* = $2000

; Test data
test_array:
    .byte $11, $22, $33, $44, $55, $66, $77, $88

global_struct_array:
    .byte $AA, $BB, $CC, $DD, $EE, $FF, $01, $02

; Zero page variables
zp_base = $20
zp_index = $22
zp_result = $24

output = $4000

main:
    ; Clear output buffer
    lda #0
    ldx #0
.clear:
    sta output, x
    inx
    cpx #$40
    bne .clear

    ldy #0  ; Output index

    ; ====== struct_elem.16 TESTS ======

    ; TEST 1: struct_elem.16 with global base (#symbol), #offset
    ; Signature: struct_elem.16 dest, #base_symbol, #offset
    ; Expected: address of test_array + 2
    struct_elem.16 zp_result, #test_array, #2
    lda (zp_result)
    sta output, y  ; Should load byte at test_array[2] = $33
    iny

    ; TEST 2: struct_elem.16 with ZP base, #offset
    ; Signature: struct_elem.16 dest, zp_address, #offset
    lda #<test_array
    ldx #>test_array
    sta zp_base
    stx zp_base+1
    struct_elem.16 zp_result, zp_base, #1
    lda (zp_result)
    sta output, y  ; Should load byte at test_array[1] = $22
    iny

    ; TEST 3: struct_elem.16 with .AX register base, #offset
    ; Signature: struct_elem.16 dest, .AX, #offset
    lda #<test_array
    ldx #>test_array
    struct_elem.16 zp_result, .AX, #3
    lda (zp_result)
    sta output, y  ; Should load byte at test_array[3] = $44
    iny

    ; TEST 4: struct_elem.16 with #offset = 0
    lda #<test_array
    ldx #>test_array
    struct_elem.16 zp_result, .AX, #0
    lda (zp_result)
    sta output, y  ; Should load test_array[0] = $11
    iny

    ; TEST 5: struct_elem.16 with large offset
    lda #<test_array
    ldx #>test_array
    struct_elem.16 zp_result, .AX, #5
    lda (zp_result)
    sta output, y  ; Should load test_array[5] = $66
    iny

    ; ====== addr_elem.16 TESTS ======

    ; TEST 6: addr_elem.16 with .AX index, stride=1
    ; Signature: addr_elem.16 dest, #base, index, #stride
    ; Index in .AX, stride=1 (byte array)
    lda #2
    ldx #0
    addr_elem.16 zp_result, #test_array, .AX, #1
    lda (zp_result)
    sta output, y  ; Should load test_array[2] = $33
    iny

    ; TEST 7: addr_elem.16 with ZP index, stride=1
    ; Signature: addr_elem.16 dest, #base, zp_index, #stride
    lda #1
    sta zp_index
    lda #0
    sta zp_index+1
    addr_elem.16 zp_result, #test_array, zp_index, #1
    lda (zp_result)
    sta output, y  ; Should load test_array[1] = $22
    iny

    ; TEST 8: addr_elem.16 with stride=2
    ; Index=1, stride=2: 1 * 2 = +2 bytes offset
    lda #1
    ldx #0
    addr_elem.16 zp_result, #global_struct_array, .AX, #2
    lda (zp_result)
    sta output, y  ; Should load byte at global_struct_array+2 = $CC
    iny

    ; TEST 9: addr_elem.16 with stride=4
    ; Index=1, stride=4: 1 * 4 = +4 bytes offset
    lda #1
    ldx #0
    addr_elem.16 zp_result, #global_struct_array, .AX, #4
    lda (zp_result)
    sta output, y  ; Should load byte at global_struct_array+4 = $EE
    iny

    ; TEST 10: addr_elem.16 with index=0 (edge case)
    lda #0
    ldx #0
    addr_elem.16 zp_result, #test_array, .AX, #1
    lda (zp_result)
    sta output, y  ; Should load test_array[0] = $11
    iny

    ; TEST 11: addr_elem.16 with large index
    lda #7
    ldx #0
    addr_elem.16 zp_result, #test_array, .AX, #1
    lda (zp_result)
    sta output, y  ; Should load test_array[7] = $88
    iny

    ; TEST 12: struct_elem.16 result to different ZP location
    lda #<test_array
    ldx #>test_array
    struct_elem.16 $26, .AX, #4  ; Result goes to $26-$27 instead of $24-$25
    lda ($26)
    sta output, y  ; Should load test_array[4] = $55
    iny

    ; Mark test complete
    lda #$AA
    sta output + $1F

    rts
