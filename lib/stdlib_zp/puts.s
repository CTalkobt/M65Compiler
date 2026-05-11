; puts.s — Write a NUL-terminated string to screen (ZP calling convention)
;
; int puts(char *s);
;   Writes string s to the screen via KERNAL CHROUT, then a newline.
;   Returns 0 on success.

.global _puts

.segment "code"

proc _puts
    .zp_uses $03, $04
    .zp_clobbers $03, $04
    .zp_release $03, $04
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V

    ; s is already at $03/$04 — use as indirect pointer
    ldy #0
@loop:
    lda ($03),y
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
    lda #0
    tax
    rts
    endproc
