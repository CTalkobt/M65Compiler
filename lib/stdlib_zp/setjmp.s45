; setjmp.s — Non-local jumps (ZP calling convention)
;
; int setjmp(jmp_buf env);
;   env pointer in $03/$04.
;   Saves return address and stack pointer into env[0..3].
;   Returns 0.
;
; void longjmp(jmp_buf env, int val);
;   env pointer in $03/$04, val in $05/$06.
;   Restores SP and jumps to saved return address.
;   setjmp appears to return val (or 1 if val==0).
;
; jmp_buf layout: [0] PCL, [1] PCH, [2] SPL, [3] SPH

.global _setjmp
.global _longjmp

.segment "code"

; --- setjmp ---

proc _setjmp
    .zp_uses $03, $04
    .zp_clobbers $03, $04
    .zp_release $03, $04
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z

    ; Save return address from hardware stack
    tsx
    lda $0101, x            ; PCL
    ldy #0
    sta ($03), y
    lda $0102, x            ; PCH
    iny
    sta ($03), y

    ; Save SP (16-bit)
    txa
    ldy #2
    sta ($03), y            ; SPL
    tsy
    tya
    iny
    sta ($03), y            ; SPH

    ; Return 0
    lda #0
    tax
    rts
    endproc

; --- longjmp ---

proc _longjmp
    .zp_uses $03, $04, $05, $06
    .zp_clobbers $03, $04, $05, $06
    .zp_release $03, $04, $05, $06
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V

    ; Check val == 0, force to 1
    lda $05
    ora $06
    bne @nonzero
    lda #1
    sta $05
    stz $06
@nonzero:

    ; Restore SPH
    ldy #3
    lda ($03), y
    tay
    tys

    ; Restore SPL
    ldy #2
    lda ($03), y
    tax
    txs

    ; Push saved return address onto hardware stack
    ldy #1
    lda ($03), y            ; PCH
    pha
    dey
    lda ($03), y            ; PCL
    pha

    ; Return val in AX
    lda $05
    ldx $06

    rts
    endproc
