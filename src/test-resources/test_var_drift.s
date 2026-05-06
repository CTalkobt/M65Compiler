    jmp start
.org $2100
start:
.var v = target
    beq v
    .res 100
target:
    rts
