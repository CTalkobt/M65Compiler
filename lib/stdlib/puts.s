; puts.s — Write a NUL-terminated string to screen
;
; int puts(char *s);
;   Writes string s to the screen via KERNAL CHROUT, then a newline.
;   Returns 0 on success.

.global _puts
.extern __sp_base

.segment "code"

proc _puts, W#_p_s
    .var _fp = 0
    ; Load pointer into ZP for indirect access
    ldax _p_s, s
    stax $02
    ldy #0
@loop:
    lda ($02),y
    beq @done
    jsr $FFD2
    iny
    bne @loop
@done:
    lda #$0D
    jsr $FFD2
    ldax #$0000         ; return 0
    rtn #0
    endproc
