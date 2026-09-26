; puts.s — Write a NUL-terminated string to screen
;
; int puts(char *s);
;   Writes string s to the screen via KERNAL CHROUT, then a newline.
;   Returns 0 on success.
;
; Uses ZP $FB/$FC for the string pointer — these are "free" ZP
; locations on both C64 and MEGA65 that the kernal does not use.

.global _puts
.extern __sp_base

.segment "code"

proc _puts, W#_p_s
    .var _fp = 0
    ; Save $FB/$FC and load string pointer there
    lda $FB
    pha
    lda $FC
    pha
    ldax _p_s+2, sp
    sta $FB
    stx $FC
    ldy #0
@loop:
    lda ($FB),y
    beq @done
    phz
    phy
    jsr $FFD2
    ply
    plz
    iny
    bne @loop
@done:
    lda #$0D
    phz
    jsr $FFD2
    plz
    ; Restore $FB/$FC
    pla
    sta $FC
    pla
    sta $FB
    ldax #$0000         ; return 0
    rtn #0
    endproc
