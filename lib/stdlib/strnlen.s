; strnlen.s — Bounded string length for cc45 / MEGA65
; Hand-optimized assembly version
;
; int strnlen(const char *s, int maxlen)

.global _strnlen

proc _strnlen, W#@_p_s, W#@_p_maxlen
    .var _fp = 0

    ldax @_p_s, sp
    stax $20                ; Save s pointer

    ldax @_p_maxlen, sp
    stax $22                ; Save maxlen

    lda #0
    tax
    stax $24                ; len = 0

@loop:
    ; Check if len >= maxlen
    lda $24
    cmp $22
    bne @not_at_max
    lda $25
    cmp $23
    beq @done
@not_at_max:

    ; Load char at s[len]
    ldy $24
    lda ($20),y

    ; Check if null terminator
    beq @done

    ; Increment len
    inc $24
    bne @loop
    inc $25
    bra @loop

@done:
    ldax $24
    rtn #0

endproc
