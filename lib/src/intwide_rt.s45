; intwide_rt.s — Width-parameterized integer arithmetic runtime for cc45
;
; All routines operate on byte arrays of arbitrary width.
; Convention: operands passed via ZP pointers, byte count in A.
;
;   __intN_a  ($08/$09) = pointer to operand A / result
;   __intN_b  ($0A/$0B) = pointer to operand B
;   A register = byte count on entry
;
; Results stored back to __intN_a (in-place).
; For comparison, result in A: $FF (-1) if A<B, $00 if A==B, $01 if A>B.

    .cpu 45gs02

; ZP pointers for operands (use scratch area)
__intN_a = $08
__intN_b = $0A

; ===========================================================================
; __intN_add: A = byteCount. *(__intN_a) += *(__intN_b)
; ===========================================================================
.global __intN_add
__intN_add:
    taz                     ; Z = byte count
    ldy #0
    clc
@add_loop:
    lda (__intN_a),y
    adc (__intN_b),y
    sta (__intN_a),y
    iny
    dez
    bne @add_loop
    rts

; ===========================================================================
; __intN_sub: A = byteCount. *(__intN_a) -= *(__intN_b)
; ===========================================================================
.global __intN_sub
__intN_sub:
    taz                     ; Z = byte count
    ldy #0
    sec
@sub_loop:
    lda (__intN_a),y
    sbc (__intN_b),y
    sta (__intN_a),y
    iny
    dez
    bne @sub_loop
    rts

; ===========================================================================
; __intN_cmp_u: A = byteCount. Unsigned compare *(__intN_a) vs *(__intN_b)
; Returns A = $FF (A<B), $00 (A==B), $01 (A>B)
; ===========================================================================
.global __intN_cmp_u
__intN_cmp_u:
    ; Compare from MSB down to LSB
    tay                     ; Y = byte count
    dey                     ; Y = index of MSB
@cmp_loop:
    lda (__intN_a),y
    cmp (__intN_b),y
    bcc @cmp_lt
    bne @cmp_gt
    dey
    bpl @cmp_loop
    ; All bytes equal
    lda #$00
    rts
@cmp_lt:
    lda #$FF
    rts
@cmp_gt:
    lda #$01
    rts

; ===========================================================================
; __intN_neg: A = byteCount. *(__intN_a) = -(*(__intN_a)) (two's complement)
; ===========================================================================
.global __intN_neg
__intN_neg:
    taz                     ; Z = byte count
    ; First: one's complement (EOR #$FF all bytes)
    ldy #0
@neg_not:
    lda (__intN_a),y
    eor #$FF
    sta (__intN_a),y
    iny
    dez
    bne @neg_not
    ; Then: increment (add 1)
    ldy #0
    sec
@neg_inc:
    lda (__intN_a),y
    adc #0
    sta (__intN_a),y
    bcc @neg_done           ; no carry = done
    iny
    bra @neg_inc
@neg_done:
    rts

; ===========================================================================
; __intN_not: A = byteCount. *(__intN_a) = ~(*(__intN_a)) (bitwise NOT)
; ===========================================================================
.global __intN_not
__intN_not:
    taz
    ldy #0
@not_loop:
    lda (__intN_a),y
    eor #$FF
    sta (__intN_a),y
    iny
    dez
    bne @not_loop
    rts

; ===========================================================================
; __intN_and: A = byteCount. *(__intN_a) &= *(__intN_b)
; ===========================================================================
.global __intN_and
__intN_and:
    taz
    ldy #0
@and_loop:
    lda (__intN_a),y
    and (__intN_b),y
    sta (__intN_a),y
    iny
    dez
    bne @and_loop
    rts

; ===========================================================================
; __intN_or: A = byteCount. *(__intN_a) |= *(__intN_b)
; ===========================================================================
.global __intN_or
__intN_or:
    taz
    ldy #0
@or_loop:
    lda (__intN_a),y
    ora (__intN_b),y
    sta (__intN_a),y
    iny
    dez
    bne @or_loop
    rts

; ===========================================================================
; __intN_xor: A = byteCount. *(__intN_a) ^= *(__intN_b)
; ===========================================================================
.global __intN_xor
__intN_xor:
    taz
    ldy #0
@xor_loop:
    lda (__intN_a),y
    eor (__intN_b),y
    sta (__intN_a),y
    iny
    dez
    bne @xor_loop
    rts

; ===========================================================================
; __intN_shl: A = byteCount, X = shift amount. *(__intN_a) <<= X
; ===========================================================================
.global __intN_shl
__intN_shl:
    sta $0C                 ; save byte count
@shl_outer:
    cpx #0
    beq @shl_done
    ; Shift all bytes left by 1
    ldy #0
    ldz $0C                 ; Z = byte count
    clc
@shl_inner:
    lda (__intN_a),y
    rol
    sta (__intN_a),y
    iny
    dez
    bne @shl_inner
    dex
    bra @shl_outer
@shl_done:
    rts

; ===========================================================================
; __intN_shr_u: A = byteCount, X = shift amount. *(__intN_a) >>= X (unsigned)
; ===========================================================================
.global __intN_shr_u
__intN_shr_u:
    sta $0C                 ; save byte count
@shr_outer:
    cpx #0
    beq @shr_done
    ; Shift all bytes right by 1, MSB first
    ldy $0C
    dey                     ; Y = MSB index
    clc
@shr_inner:
    lda (__intN_a),y
    ror
    sta (__intN_a),y
    dey
    bpl @shr_inner
    dex
    bra @shr_outer
@shr_done:
    rts

; ===========================================================================
; __intN_mul: A = byteCount. *(__intN_a) *= *(__intN_b)
; Shift-and-add multiplication. Uses $0C-$0D as scratch.
; Result stored in __intN_a.
; ===========================================================================
.global __intN_mul
__intN_mul:
    ; Save byte count
    sta $0C                 ; byte count
    ; We need a temporary result buffer and a copy of A.
    ; Use stack frame or ZP scratch. For simplicity, use a fixed
    ; 16-byte buffer at $10-$1F for the result (supports up to 128-bit).
    ; Clear result buffer
    taz
    ldy #0
    lda #0
@mul_clr:
    sta $10,y
    iny
    dez
    bne @mul_clr
    ; Outer loop: for each bit of B
    ldx $0C                 ; total bits = byteCount * 8
    stx $0D                 ; save byte count in $0D too
    lda $0C
    asl                     ; *2
    asl                     ; *4
    asl                     ; *8 = bit count
    tax                     ; X = bit count
@mul_bit:
    cpx #0
    beq @mul_store
    ; Check LSB of B
    phy
    ldy #0
    lda (__intN_b),y
    ply
    and #1
    beq @mul_noAdd
    ; Add A to result: result += A
    ldy #0
    ldz $0D
    clc
@mul_addA:
    lda $10,y
    adc (__intN_a),y
    sta $10,y
    iny
    dez
    bne @mul_addA
@mul_noAdd:
    ; Shift A left by 1
    ldy #0
    ldz $0D
    clc
@mul_shlA:
    lda (__intN_a),y
    rol
    sta (__intN_a),y
    iny
    dez
    bne @mul_shlA
    ; Shift B right by 1
    ldy $0D
    dey
    clc
@mul_shrB:
    lda (__intN_b),y
    ror
    sta (__intN_b),y
    dey
    bpl @mul_shrB
    dex
    bra @mul_bit
@mul_store:
    ; Copy result back to A
    ldy #0
    ldz $0D
@mul_copy:
    lda $10,y
    sta (__intN_a),y
    iny
    dez
    bne @mul_copy
    rts
