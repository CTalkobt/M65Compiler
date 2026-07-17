; Regression test for BinaryExpr reentrancy (issue #35)
; Tests nested arithmetic expressions where the right operand is itself a BinaryExpr
; that writes to hardware math registers ($D770+/$D760+)
;
; If this assembles without errors, the reentrancy bug is fixed.
; The expressions test the two main clobber scenarios:
; 1. Nested + inside * — (a+b)*(c+d) = 7*11 = 77
; 2. Nested / inside + — (a/b)+(c/d) = 5+2 = 7

.var a = $20
.var b = $21
.var c = $22
.var d = $23

_test_binary_expr_reentrant:
    ; Test 1: (3 + 4) * (5 + 6)
    ; Without fix: right operand (5+6) clobbers left operand (3+4) during hardware register write
    expr .A, (a + b) * (c + d)

    ; Test 2: (10 / 2) + (8 / 4)
    ; Without fix: right operand (8/4) clobbers left operand (10/2) during hardware register write
    expr .A, (a / b) + (c / d)

    ; Test 3: Simple case without nested hardware math (should zero-overhead)
    ; No push/pop should be generated for this
    expr .A, (a + b) * c

    rts
