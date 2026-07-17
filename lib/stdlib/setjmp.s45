; setjmp.s — Non-local jumps
;
; int setjmp(jmp_buf env);
;   Saves return address and stack pointer into env[0..3].
;   Returns 0.
;
; void longjmp(jmp_buf env, int val);
;   Restores SP and jumps to saved return address.
;   setjmp appears to return val (or 1 if val==0).
;
; jmp_buf layout:
;   [0] PC low
;   [1] PC high
;   [2] SPL
;   [3] SPH
;
; Stack convention: env is a pointer (2 bytes) passed on the software stack.
; On entry to setjmp, the hardware stack has the return address from JSR.

.global _setjmp
.global _longjmp
.extern __sp_base
.extern __zp_scratch

.segment "code"

; --- setjmp ---
; The return address on the hardware stack points to the byte AFTER the JSR,
; minus 1 (6502 convention: JSR pushes PC+2, RTS adds 1).
; We save the raw value — RTS will handle the +1 on restore.

proc _setjmp, W#_p_env
    .var _fp = 0

    ; Get pointer to jmp_buf
    ldax _p_env, sp
    sta __zp_scratch
    stx __zp_scratch+1

    ; Save return address from hardware stack
    ; JSR pushed PCH then PCL; they sit at SP+1 (PCL) and SP+2 (PCH)
    tsx
    lda __sp_base+1, x      ; PCL (return addr low)
    ldy #0
    sta (__zp_scratch), y
    lda __sp_base+2, x      ; PCH (return addr high)
    iny
    sta (__zp_scratch), y

    ; Save stack pointer (16-bit)
    tsx
    txa
    ldy #2
    sta (__zp_scratch), y    ; SPL
    tsy
    tya
    iny                      ; Y=3
    sta (__zp_scratch), y    ; SPH

    ; Return 0
    lda #0
    tax
    rtn #0
    endproc

; --- longjmp ---

proc _longjmp, W#_p_env, W#_p_val
    .var _fp = 0

    ; Load val
    ldax _p_val, sp
    ; Check if AX == 0 (OR low and high bytes)
    sta __zp_scratch+2
    stx __zp_scratch+3
    txa
    ora __zp_scratch+2
    bne @nonzero
    ; val was 0, force to 1
    lda #1
    sta __zp_scratch+2
    stz __zp_scratch+3
@nonzero:

    ; Load jmp_buf pointer
    ldax _p_env, sp
    sta __zp_scratch
    stx __zp_scratch+1

    ; Restore SP
    ldy #3
    lda (__zp_scratch), y   ; SPH
    tay
    tys                     ; restore SPH
    ldy #2
    lda (__zp_scratch), y   ; SPL
    tax
    txs                     ; restore SPL

    ; Push saved return address back onto hardware stack
    ; RTS expects: PCH at SP+2, PCL at SP+1 (6502 convention)
    ldy #1
    lda (__zp_scratch), y   ; PCH
    pha
    dey
    lda (__zp_scratch), y   ; PCL
    pha

    ; Load return value into AX
    lda __zp_scratch+2
    ldx __zp_scratch+3

    ; RTS will jump to saved return address + 1
    rts
    endproc
