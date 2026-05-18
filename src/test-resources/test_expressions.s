; Test: Assembler expression evaluation coverage
; Exercises parseExprAST paths: arithmetic, bitwise, shifts, comparisons,
; logical ops, unary ops, defined(), lo()/hi()/bank(), binary literals,
; parenthesized expressions, current address ($), and ternary-like patterns.

.org $2000

; --- Constants and symbols ---
MY_CONST = 42
BIG_CONST = $1234
ADDR_24BIT = $012345

; --- Arithmetic expressions ---
arith_add:
    lda #MY_CONST + 8       ; addition
arith_sub:
    lda #100 - MY_CONST     ; subtraction
arith_mul:
    lda #6 * 7              ; multiplication
arith_div:
    lda #100 / 10           ; division
arith_complex:
    lda #(10 + 5) * 3       ; parenthesized

; --- Bitwise expressions ---
bitwise_and:
    lda #$FF & $0F          ; AND
bitwise_or:
    lda #$F0 | $0F          ; OR
bitwise_xor:
    lda #$FF ^ $AA          ; XOR

; --- Shift expressions ---
shift_left:
    lda #1 << 4             ; left shift
shift_right:
    lda #$80 >> 3           ; right shift

; --- Comparison expressions (used in .if) ---
.if MY_CONST == 42
compare_eq:
    eom                     ; == true
.endif

.if MY_CONST != 0
compare_ne:
    eom                     ; != true
.endif

.if MY_CONST > 10
compare_gt:
    eom                     ; > true
.endif

.if MY_CONST < 100
compare_lt:
    eom                     ; < true
.endif

.if MY_CONST >= 42
compare_ge:
    eom                     ; >= true
.endif

.if MY_CONST <= 42
compare_le:
    eom                     ; <= true
.endif

; --- Logical expressions ---
.if MY_CONST > 0 && MY_CONST < 100
logical_and:
    eom                     ; && true
.endif

.if MY_CONST == 0 || MY_CONST == 42
logical_or:
    eom                     ; || true
.endif

; --- Unary operators ---
unary_neg:
    .word -1                ; negation (results in $FFFF)
unary_not_bitwise:
    lda #~$F0               ; bitwise NOT ($0F)
unary_lo:
    lda #<BIG_CONST         ; lo byte ($34)
unary_hi:
    lda #>BIG_CONST         ; hi byte ($12)

; --- lo()/hi()/bank() function syntax ---
func_lo:
    lda #lo(BIG_CONST)      ; lo() function
func_hi:
    lda #hi(BIG_CONST)      ; hi() function
func_bank:
    lda #bank(ADDR_24BIT)   ; bank() function ($01)

; --- defined() function ---
.if defined(MY_CONST)
defined_yes:
    eom                     ; symbol exists
.endif

.if !defined(NONEXISTENT_SYMBOL)
defined_no:
    eom                     ; symbol doesn't exist
.endif

; --- Binary literal ---
binary_lit:
    lda #%10101010          ; binary literal ($AA)

; --- Label arithmetic ---
label_arith:
    jmp label_arith + 3     ; label + offset

; --- Nested expressions with operator precedence ---
precedence_1:
    lda #2 + 3 * 4          ; mul before add = 14
precedence_2:
    lda #(2 + 3) * 4        ; parens override = 20
precedence_3:
    lda #$FF & $0F | $30    ; and before or = $3F

; --- Multi-term expressions ---
multi_term:
    lda #1 + 2 + 3 + 4     ; chained addition = 10

; --- Expressions in equates (exercises full evaluator) ---
EXPR_ADD = $1234 + $100      ; $1334
EXPR_SUB = $1234 - $34       ; $1200
EXPR_AND = $FF00 & $0F0F     ; $0F00
EXPR_OR  = $F000 | $000F     ; $F00F
EXPR_XOR = $FFFF ^ $AA55     ; $55AA
EXPR_SHL = 1 << 8            ; $0100
EXPR_SHR = $8000 >> 4        ; $0800

equate_use:
    lda #<EXPR_ADD          ; uses lo() of computed equate
    lda #>EXPR_ADD          ; uses hi() of computed equate
    lda #<EXPR_AND
    lda #<EXPR_OR
    lda #<EXPR_XOR
    lda #<EXPR_SHL
    lda #<EXPR_SHR
