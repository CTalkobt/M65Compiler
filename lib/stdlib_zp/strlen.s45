; strlen.s — Return length of a NUL-terminated string (ZP calling convention)
;
; int strlen(char *s);
;   Returns the number of bytes before the NUL terminator.
;   Limited to 255 (single-byte Y counter).

.global _strlen

.segment "code"

proc _strlen
    .zp_uses $03, $04
    .zp_clobbers $03, $04
    .zp_release $03, $04
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z

    ; s is already at $03/$04
    ldy #0
@loop:
    lda ($03),y
    beq @done
    iny
    bne @loop
@done:
    tya
    ldx #0
    rts
    endproc
