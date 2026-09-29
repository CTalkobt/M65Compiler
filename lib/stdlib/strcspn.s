; strcspn.s — Length of initial segment not matching reject characters
; Hand-optimized assembly version
;
; int strcspn(char *s, char *reject)
;   AX = pointer to string s
;   Next param on stack: reject pointer
; Returns:
;   AX = count of characters before first reject char match

.global _strcspn

proc _strcspn, W#@_p_s, W#@_p_reject
    .var _fp = 0

    ldax @_p_s, sp
    stax $20                ; s pointer

    ldax @_p_reject, sp
    stax $22                ; reject pointer

    lda #0
    tax
    stax $24                ; count = 0

@outer_loop:
    ; Get current char from s
    ldy #0
    lda ($20),y             ; s[count]
    beq @done                ; if null, done

    ; Inner loop: check against each reject char
    stax $26                ; save current s char

@inner_loop:
    ldy #0
    lda ($22),y             ; reject[inner_idx]
    beq @inner_done         ; if null reject, no match

    ; Compare s[count] with reject[inner_idx]
    cmp $26
    beq @found_reject       ; if match, return count

    ; Increment reject pointer
    inc $22
    bne @inner_loop
    inc $23
    bra @inner_loop

@inner_done:
    ; No match found, increment count and s
    inc $20
    bne @next_count
    inc $21
@next_count:
    inc $24
    bne @outer_loop
    inc $25
    bra @outer_loop

@found_reject:
    ldax $24
    rtn #0

@done:
    ldax $24
    rtn #0

endproc
