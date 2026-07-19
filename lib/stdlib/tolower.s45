; tolower.s — Convert uppercase letter to lowercase (PETSCII)
;
; int tolower(int c);
;   If c is uppercase ($C1-$DA), returns lowercase ($41-$5A).
;   Otherwise returns c unchanged.
;
;   PETSCII: uppercase - $80 = lowercase (clear bit 7)

.global _tolower
.extern __sp_base

.segment "code"

proc _tolower, W#_p_c
    .var _fp = 0
    lda.sp _p_c
    cmp #$c1
    bcc @done
    cmp #$db
    bcs @done
    ; Clear bit 7 to convert uppercase -> lowercase
    and #$7f
@done:
    ldx #0
    rtn #0
    endproc
