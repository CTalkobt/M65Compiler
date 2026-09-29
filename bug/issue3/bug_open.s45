.cpu _45gs02
.code

.global _openobj

setnam = $ffbd

_openobj:
    lda #objname_end - objname
    ldx #<objname
    ldy #>objname
    jsr setnam
    rts

.data

objname:
    .byte $43,$2c,$53    ; C,S
objname_end: