; Assembler expression validation test
; Verifies ExpressionUtils unification for binary and unary operators in assembler

* = $0800

; Binary arithmetic operations
test_add:
    lda #(5 + 3)        ; should be 8
    cmp #8
    bne fail

test_sub:
    lda #(10 - 2)       ; should be 8
    cmp #8
    bne fail

test_mul:
    lda #(3 * 4)        ; should be 12
    cmp #12
    bne fail

test_div:
    lda #(16 / 2)       ; should be 8
    cmp #8
    bne fail

test_mod:
    lda #(17 % 5)       ; should be 2
    cmp #2
    bne fail

test_exp:
    lda #(2 ** 3)       ; should be 8
    cmp #8
    bne fail

; Binary bitwise operations
test_and:
    lda #(12 & 10)      ; should be 8
    cmp #8
    bne fail

test_or:
    lda #(8 | 4)        ; should be 12
    cmp #12
    bne fail

test_xor:
    lda #(15 ^ 6)       ; should be 9
    cmp #9
    bne fail

test_lshift:
    lda #(3 << 2)       ; should be 12
    cmp #12
    bne fail

test_rshift:
    lda #(16 >> 2)      ; should be 4
    cmp #4
    bne fail

; Binary comparison operations
test_eq:
    lda #(5 == 5)       ; should be 1
    cmp #1
    bne fail

test_ne:
    lda #(3 != 4)       ; should be 1
    cmp #1
    bne fail

test_lt:
    lda #(3 < 5)        ; should be 1
    cmp #1
    bne fail

test_le:
    lda #(5 <= 5)       ; should be 1
    cmp #1
    bne fail

test_gt:
    lda #(6 > 4)        ; should be 1
    cmp #1
    bne fail

test_ge:
    lda #(5 >= 5)       ; should be 1
    cmp #1
    bne fail

; Binary logical operations
test_land:
    lda #(1 && 1)       ; should be 1
    cmp #1
    bne fail

test_lor:
    lda #(0 || 1)       ; should be 1
    cmp #1
    bne fail

; Unary operations
test_bnot:
    lda #(~5)           ; should be 0xFA in low byte
    and #$FF
    cmp #$FA
    bne fail

test_lnot_zero:
    lda #(!0)           ; should be 1
    cmp #1
    bne fail

test_lnot_nonzero:
    lda #(!5)           ; should be 0
    cmp #0
    bne fail

test_neg:
    lda #(-10)          ; should be 0xF6 in low byte
    and #$FF
    cmp #$F6
    bne fail

; Ternary operation
test_ternary_true:
    lda #(1 ? 42 : 99)  ; should be 42
    cmp #42
    bne fail

test_ternary_false:
    lda #(0 ? 42 : 99)  ; should be 99
    cmp #99
    bne fail

; Precedence test
test_precedence:
    lda #(2 + 3 * 4)    ; should be 14
    cmp #14
    bne fail

pass:
    lda #0
    rts

fail:
    lda #1
    rts
