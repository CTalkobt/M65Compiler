; puts.s — Write a NUL-terminated string to screen (ZP calling convention)
;
; int puts(char *s);
;   Writes string s to the screen via KERNAL CHROUT, then a newline.
;   Returns 0 on success.
;
; Uses ZP $FB/$FC for the string pointer — kernal-safe locations.

.global _puts

.segment "code"

proc _puts
    .zp_uses $03, $04
    .zp_clobbers $03, $04
    .zp_release $03, $04
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V

    ; Save $FB/$FC and copy string pointer there
    lda $FB
    pha
    lda $FC
    pha
    lda $03
    sta $FB
    lda $04
    sta $FC
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
    lda #0
    tax
    rts
    endproc
