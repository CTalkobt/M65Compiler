; tolower.s — Convert uppercase letter to lowercase (ZP calling convention)
;
; int tolower(int c);
;   If c is uppercase ($C1-$DA), returns lowercase ($41-$5A).

.global _tolower

.segment "code"

proc _tolower
    .zp_uses $03, $04
    .zp_clobbers $03, $04
    .zp_release $03, $04
    .reg_clobbers A, X
    .flag_clobbers C, N, Z

    lda $03
    cmp #$c1
    bcc @done
    cmp #$db
    bcs @done
    and #$7f
@done:
    ldx #0
    rts
    endproc
