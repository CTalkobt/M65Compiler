; strcmp.s — Compare two strings
;
; int strcmp(char *s1, char *s2);
;   Returns 0 if equal, <0 if s1 < s2, >0 if s1 > s2.
;   Unsigned byte comparison.

.global _strcmp

.segment "code"

proc _strcmp, W#_p_s1, W#_p_s2
    .var _fp = 0
    ldax _p_s1, s
    stax $02
    ldax _p_s2, s
    stax $04
    ldy #0
@loop:
    lda ($02),y
    cmp ($04),y
    bne @diff
    ; both equal — check if NUL
    lda ($02),y
    beq @equal
    iny
    bne @loop
    inc $03
    inc $05
    bra @loop
@equal:
    ldax #$0000
    rtn #0
@diff:
    bcs @greater
    ; s1 < s2: return -1 (0xFFFF)
    ldax #$FFFF
    rtn #0
@greater:
    ; s1 > s2: return 1
    ldax #$0001
    rtn #0
    endproc
