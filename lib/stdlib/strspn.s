; strspn.s — Length of initial segment matching accept characters
; Hand-optimized assembly version
;
; int strspn(char *s, char *accept)
;   AX = pointer to string s
;   Next param on stack: accept pointer
; Returns:
;   AX = count of characters in s that are all in accept set

.global _strspn

proc _strspn, W#@_p_s, W#@_p_accept
    .var _fp = 0

    ldax @_p_s, sp
    stax $20                ; s pointer

    ldax @_p_accept, sp
    stax $22                ; accept pointer

    lda #0
    tax
    stax $24                ; count = 0

@outer_loop:
    ; Get current char from s
    ldy #0
    lda ($20),y             ; s[count]
    beq @done                ; if null, done

    ; Save current char and reset accept pointer
    stax $26                ; save current s char
    lda $22
    ldx $23
    stax $28                ; save accept pointer start

@inner_loop:
    ; Load char from accept
    ldy #0
    lda ($22),y             ; accept[inner_idx]
    beq @no_match           ; if null, no match found

    ; Compare s char with accept char
    cmp $26
    beq @found_match        ; if match, continue outer loop

    ; Move to next accept char
    inc $22
    bne @inner_loop
    inc $23
    bra @inner_loop

@found_match:
    ; Char was in accept set, increment count and s
    inc $20
    bne @inc_count
    inc $21
@inc_count:
    inc $24
    bne @restore_accept
    inc $25
@restore_accept:
    ldax $28                ; restore accept pointer
    stax $22
    bra @outer_loop

@no_match:
    ; Char was NOT in accept set, return count
    ldax $24
    rtn #0

@done:
    ldax $24
    rtn #0

endproc
