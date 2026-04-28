.extern _printf, _puts

.global _main
.global _count

.segment "code"
* = $2000

_main:
    jsr _printf
    lda _count
    rts

.segment "data"
_count: .byte $42
