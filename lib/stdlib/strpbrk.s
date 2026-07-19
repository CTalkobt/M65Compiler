; strpbrk.s — Find first occurrence of any character from accept in s
; Hand-optimized assembly version
;
; char *strpbrk(char *s, char *accept)
;   AX = pointer to string s
;   Next param on stack: accept pointer
; Returns:
;   AX = pointer to first char in s that is in accept, or 0

.global _strpbrk

proc _strpbrk, W#@_p_s, W#@_p_accept
    .var _fp = 0

    ldax @_p_s, sp
    stax $20                ; s pointer

    ldax @_p_accept, sp
    stax $22                ; accept pointer

@loop:
    ; Get current char from s
    ldy #0
    lda ($20),y
    beq @not_found          ; if null, no match

    ; Save current char
    stax $24                ; save current s char and pointer high byte

@inner_loop:
    ; Get char from accept
    ldy #0
    lda ($22),y
    beq @next_s             ; if null, no match in accept set

    ; Compare s char with accept char
    cmp $24
    beq @found              ; if match, return current s pointer

    ; Move to next accept char
    inc $22
    bne @inner_loop
    inc $23
    bra @inner_loop

@next_s:
    ; No match found in accept, move to next s char
    inc $20
    bne @loop
    inc $21
    bra @loop

@found:
    ; Return current s pointer
    ldax $20
    rtn #0

@not_found:
    ; No char found in entire s
    lda #0
    tax
    rtn #0

endproc
