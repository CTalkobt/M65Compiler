.cpu _45gs02

.global my_label

.code
* = $2000

_test:
    sta my_label
    lda my_label
    rts

.data
* = $3000

my_label:
    .byte 0
