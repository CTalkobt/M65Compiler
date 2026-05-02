; Test 32-bit native Q register operations and simulated opcodes
; Expected bytes at $4000 after execution:
;   4000: E0 93 04 00  — sum of $186A0 + $30D40 = $493E0
;   4004: A0 86 01 00  — diff $30D40 - $186A0 = $186A0
;   4008: E0 8F 03 00  — OR of $186A0 | $30D40 = $38FE0
;   400C: 60 79 FE FF  — NEG of $186A0 = $FFFE7960
;   4010: FF FF FF FF  — SXT.16 of $FFFF = $FFFFFFFF

.org $2000

__sp_base = $0101
__zp_scratch = $02

; Use ZP $10-$17 for temps (avoid overlap with scratch)

; --- Test ADD.32 via native adcq ---
    ldq val_a
    stq $10          ; ZP $10-$13 = val_a
    ldq val_b
    clc
    adcq $10         ; AXYZ = val_b + val_a
    stq $4000        ; store result

; --- Test SUB.32 via native sbcq ---
    ldq val_a
    stq $14          ; ZP $14-$17 = val_a
    ldq val_b        ; AXYZ = val_b
    sec
    sbcq $14         ; AXYZ = val_b - val_a
    stq $4004

; --- Test ORA via native oraq ---
    ldq val_a
    stq $10          ; ZP $10-$13 = val_a
    ldq val_b
    oraq $10          ; AXYZ = val_b | val_a
    stq $4008

; --- Test NEG.32 ---
    ldq val_a
    neg.32 .AXYZ
    stq $400C

; --- Test SXT.16 (sign extend -1 from 16 to 32 bits) ---
    lda #$FF
    ldx #$FF
    sxt.16
    stq $4010

    rts

val_a:
    .dword $000186A0  ; 100000
val_b:
    .dword $00030D40  ; 200000
