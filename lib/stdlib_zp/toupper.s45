; toupper.s — Convert lowercase letter to uppercase (ZP calling convention)
;
; int toupper(int c);
;   If c is lowercase ($41-$5A), returns uppercase ($C1-$DA).

.global _toupper

.segment "code"

proc _toupper
    .zp_uses $03, $04
    .zp_clobbers $03, $04
    .zp_release $03, $04
    .reg_clobbers A, X
    .flag_clobbers C, N, Z

    lda $03
    cmp #$41
    bcc @done
    cmp #$5b
    bcs @done
    ora #$80
@done:
    ldx #0
    rts
    endproc
