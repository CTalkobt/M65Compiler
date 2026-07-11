; Minimal test for addr_elem.16 and struct_elem.16
; Just test the simulated ops themselves

* = $2000

main:
    ; TEST 1: struct_elem.16 with global base and offset
    struct_elem.16 $20, #$1000, #5

    ; TEST 2: addr_elem.16 with register index and stride=2
    lda #3
    ldx #0
    addr_elem.16 $22, #$2000, .AX, #2

    rts
