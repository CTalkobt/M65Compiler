; strncmp.s — Compare two strings with length limit
;
; int strncmp(char *s1, char *s2, int n);
;   Compares at most n bytes. Returns 0 if equal, <0 or >0 otherwise.

.global _strncmp
.extern __sp_base

.segment "code"

proc _strncmp, W#_p_s1, W#_p_s2, W#_p_n
    .var _fp = 0
    ldax _p_s1, sp
    stax $02
    ldax _p_s2, sp
    stax $04
    ldax _p_n, sp
    stax $06            ; $06/$07 = count
    ldy #0
@loop:
    ; check count == 0
    lda $06
    ora $07
    beq @equal          ; exhausted n — strings equal so far
    lda ($02),y
    cmp ($04),y
    bne @diff
    ; both equal — check if NUL
    lda ($02),y
    beq @equal
    iny
    bne @no_inc
    inc $03
    inc $05
@no_inc:
    ; decrement count
    lda $06
    bne @dec_lo
    dec $07
@dec_lo:
    dec $06
    bra @loop
@equal:
    ldax #$0000
    rtn #0
@diff:
    bcs @greater
    ldax #$FFFF
    rtn #0
@greater:
    ldax #$0001
    rtn #0
    endproc
