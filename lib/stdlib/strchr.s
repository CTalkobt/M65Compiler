; strchr.s — Locate character in string
;
; char *strchr(char *s, int c);
;   Returns pointer to first occurrence of (char)c in s,
;   or NULL (0) if not found. The NUL terminator is considered
;   part of the string.

.global _strchr
.extern __sp_base

.segment "code"

proc _strchr, W#_p_s, W#_p_c
    .var _fp = 0
    ldax _p_s, sp
    stax $02
    lda.sp _p_c
    sta $04
    ldy #0
@loop:
    lda ($02),y
    cmp $04
    beq @found
    lda ($02),y
    beq @notfound
    iny
    bne @loop
    inc $03
    bra @loop
@found:
    ; return pointer = base + Y
    tya
    clc
    adc $02
    pha                  ; save lo
    lda #0
    adc $03
    tax                  ; X = hi
    pla                  ; A = lo
    rtn #0
@notfound:
    ldax #$0000
    rtn #0
    endproc
