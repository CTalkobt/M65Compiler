; strstr.s — Locate substring in string
; Hand-optimized assembly version
;
; char *strstr(char *haystack, char *needle)
;   AX = haystack pointer
;   Next param on stack: needle pointer
; Returns:
;   AX = pointer to first occurrence of needle in haystack, or 0

.global _strstr

proc _strstr, W#@_p_haystack, W#@_p_needle
    .var _fp = 0

    ldax @_p_haystack, sp
    stax $20                ; haystack pointer

    ldax @_p_needle, sp
    stax $22                ; needle pointer

    ; Check if needle is empty
    ldy #0
    lda ($22),y
    beq @return_haystack    ; if empty needle, return haystack

@outer_loop:
    ; Get current char from haystack
    ldy #0
    lda ($20),y
    beq @not_found          ; if null, end of haystack

    ; Save haystack position
    ldax $20
    stax $24                ; start = haystack

    ; Initialize h and n pointers
    ldax $20
    stax $26                ; h = haystack
    ldax $22
    stax $28                ; n = needle

@inner_loop:
    ; Get char from h
    ldy #0
    lda ($26),y
    beq @check_needle       ; if h is null, check if n is also null
    sta $2A                 ; save h char

    ; Get char from n
    ldy #0
    lda ($28),y
    beq @check_needle       ; if n is null, match found
    cmp $2A
    bne @next_haystack      ; if not equal, move to next haystack position

    ; Characters match, increment both pointers
    inc $26
    bne @inc_n
    inc $27
@inc_n:
    inc $28
    bne @inner_loop
    inc $29
    bra @inner_loop

@check_needle:
    ; Check if n is at null terminator (match found)
    ldy #0
    lda ($28),y
    beq @found              ; if n is null, we found a match

@next_haystack:
    ; Move to next haystack position
    inc $20
    bne @outer_loop
    inc $21
    bra @outer_loop

@found:
    ; Return start (the position where match began)
    ldax $24
    rtn #0

@return_haystack:
    ; Empty needle, return haystack as-is
    ldax $20
    rtn #0

@not_found:
    ; No match found
    lda #0
    tax
    rtn #0

endproc
