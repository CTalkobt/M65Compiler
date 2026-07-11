; Test addr_elem.16 and struct_elem.16 simulated opcodes
; Simple assembly test

* = $2000

test_array:
    .byte $11, $22, $33, $44, $55, $66

main:
    ; Test struct_elem.16 with immediate offset
    ; struct_elem.16 dest, base, #offset
    lda #<test_array
    ldx #>test_array
    struct_elem.16 $20, .AX, #2

    ; Test addr_elem.16 with runtime index
    ; addr_elem.16 dest, base, index, #stride
    lda #1
    ldx #0
    addr_elem.16 $22, #test_array, .AX, #1

    rts
