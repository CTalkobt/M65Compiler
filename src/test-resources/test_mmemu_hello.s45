; Test program for mmemu-cli validation
; This program copies "hello world!" to $4000
.org $2000

start:
    ldx #0
loop:
    lda msg, x
    sta $4000, x
    inx
    cpx #12
    bne loop
    
    ; Signal end of test
    brk

msg:
    .text "hello world!"
