; Static test for addr_elem and struct_elem
; This test verifies the simulated ops can be assembled and generate code

* = $2000

; Test 1: struct_elem.16 basic test
; Load address test_data + offset into $20-$21
test1_struct:
    lda #<test_data
    ldx #>test_data
    struct_elem.16 $20, .AX, #2
    ; Result: $20-$21 should contain address of test_data + 2

; Test 2: addr_elem.16 basic test
; Load address test_data + (1 * 2) into $22-$23
test2_addr:
    lda #1
    ldx #0
    addr_elem.16 $22, #test_data, .AX, #2
    ; Result: $22-$23 should contain address of test_data + 2

; Test 3: struct_elem.16 with different offset
test3_struct:
    lda #<test_data
    ldx #>test_data
    struct_elem.16 $24, .AX, #4
    ; Result: $24-$25 should contain address of test_data + 4

; Test 4: addr_elem.16 with larger index
test4_addr:
    lda #3
    ldx #0
    addr_elem.16 $26, #test_data, .AX, #2
    ; Result: $26-$27 should contain address of test_data + 6

; Stop
    jmp *

test_data:
    .byte $AA, $BB, $CC, $DD, $EE, $FF, $11, $22
