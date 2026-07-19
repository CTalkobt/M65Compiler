; ispunct.s — Test if character is punctuation (PETSCII)
;
; int ispunct(int c);
;   Returns 1 if c is printable but not alphanumeric and not space.
;   Printable PETSCII: $20-$7E, $A0-$FF (excluding letters and digits).

.global _ispunct
.extern __sp_base

.segment "code"

proc _ispunct, W#_p_c
    .var _fp = 0
    lda.sp _p_c
    ; Must be printable: reject control chars ($00-$1F)
    cmp #$20
    bcc @no
    ; Reject space
    cmp #$20
    beq @no
    ; Reject digits $30-$39
    cmp #$30
    bcc @yes
    cmp #$3a
    bcc @no
    ; Reject lowercase a-z ($41-$5A)
    cmp #$41
    bcc @yes
    cmp #$5b
    bcc @no
    ; Accept $5B-$C0 range (various PETSCII punctuation/graphics)
    cmp #$c1
    bcc @yes
    ; Reject uppercase A-Z ($C1-$DA)
    cmp #$db
    bcc @no
    ; Accept $DB-$FF
@yes:
    lda #1
    ldx #0
    rtn #0
@no:
    lda #0
    ldx #0
    rtn #0
    endproc
