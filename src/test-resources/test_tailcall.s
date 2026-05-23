; Test tail-call optimization (JSR+RTS -> JMP)
; The optimizer should convert jsr+rts into jmp

.org $2000

; Simple tail-call: jsr helper; rts → jmp helper
proc _main
    lda #$42
    jsr _helper    ; should be converted to jmp _helper
    rts            ; should be deleted
endproc

proc _helper
    sta $0400
    rts
endproc
