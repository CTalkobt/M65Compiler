.cpu _45gs02
.segmentOrder code, data, bss

.global _test

ptr = $20

.code

_test:
lda #<buffer
sta ptr
lda #>buffer
sta ptr+1

lda ptr
sta $0400
lda ptr+1
sta $0401

lda #$AA
sta $0402
loop:
jmp loop

rts
.bss

buffer:
.res 1024

