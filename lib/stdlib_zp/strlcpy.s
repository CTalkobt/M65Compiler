; strlcpy.s — Safe string copy for cc45 / MEGA65
; Hand-optimized assembly version
;
; int strlcpy(char *dst, const char *src, int size)

.global _strlcpy
.extern _strlen

proc _strlcpy, W#@_p_dst, W#@_p_src, W#@_p_size
    .var _fp = 0

    ldax @_p_dst, sp
    stax $20                ; dst pointer

    ldax @_p_src, sp
    stax $22                ; src pointer

    ldax @_p_size, sp
    stax $24                ; size

    ; Get src length first
    ldax @_p_src, sp
    pha
    phx
    jsr _strlen             ; Returns strlen(src) in AX
    stax $26                ; src_len

    ; Check if size <= 0
    lda $24
    ora $25
    beq @return_src_len

    ; Copy min(size-1, strlen(src)) bytes
    lda $24
    sec
    sbc #1
    sta $28
    lda $25
    sbc #0
    sta $29

    ; Compare size-1 with strlen(src)
    lda $28
    cmp $26
    bcc @size_smaller       ; branch if size-1 < src_len
    bne @start_copy         ; branch if size-1 > src_len
    lda $29
    cmp $27
    bcc @size_smaller
    beq @start_copy

@size_smaller:
    ; src_len is smaller, use strlen(src) instead
    lda $26
    sta $28
    lda $27
    sta $29

@start_copy:
    ; Copy $28/$29 bytes from src to dst
    ldy #0

@copy_loop:
    cpy $28
    bne @copy_cont
    ; Check high byte
    lda $29
    beq @copy_done
    dec $29
    ldy #0

@copy_cont:
    lda ($22),y
    sta ($20),y
    iny
    bra @copy_loop

@copy_done:
    ; Null-terminate
    ldy #0
    lda #0
    sta ($20),y

@return_src_len:
    ldax $26
    rtn #0

endproc
