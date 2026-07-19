; toupper.s — Convert lowercase letter to uppercase (PETSCII)
;
; int toupper(int c);
;   If c is lowercase ($41-$5A), returns uppercase ($C1-$DA).
;   Otherwise returns c unchanged.
;
;   PETSCII: lowercase + $80 = uppercase

.global _toupper
.extern __sp_base

.segment "code"

proc _toupper, W#_p_c
    .var _fp = 0
    lda.sp _p_c
    cmp #$41
    bcc @done
    cmp #$5b
    bcs @done
    ; Add $80 to convert lowercase -> uppercase
    ora #$80
@done:
    ldx #0
    rtn #0
    endproc
