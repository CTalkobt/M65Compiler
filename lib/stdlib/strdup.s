; strdup.s — Allocate and duplicate string for cc45 / MEGA65
; Hand-optimized assembly version
;
; char *strdup(const char *s)

.global _strdup
.extern _strlen
.extern _malloc
.extern _memcpy

proc _strdup, W#@_p_s
    .var _fp = 0
    ; Get s parameter
    ldax @_p_s, sp
    stax $20                ; Save s pointer

    ; strlen(s)
    pha
    phx
    jsr _strlen             ; Returns length in AX

    ; Add 1 for null terminator
    clc
    adc #1
    bne @len_done
    inx
@len_done:
    stax $22                ; Save len+1

    ; malloc(len+1)
    pha
    phx
    jsr _malloc             ; Returns in AX

    ; Check if malloc failed (result == 0)
    ora $03
    beq @fail

    ; Save malloc result
    stax $24                ; Save copy pointer

    ; memcpy(copy, s, len+1)
    lda $24
    ldx $25
    pha                     ; push copy
    lda $20
    ldx $21
    pha                     ; push s
    lda $22
    ldx $23
    pha                     ; push len+1
    jsr _memcpy

    ; Return copy pointer
    ldax $24
    rtn #0

@fail:
    lda #0
    tax
    rtn #0

endproc
