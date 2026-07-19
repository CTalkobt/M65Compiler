; __builtin_stack.s — Stack introspection builtins
;
; void* __builtin_frame_address(unsigned int level);
; void* __builtin_return_address(unsigned int level);

.global ___builtin_frame_address
.global ___builtin_return_address
.extern __sp_base

.segment "code"

; --- __builtin_frame_address ---
; Returns the current value in $FD/$FE (the frame pointer used by cc45)

___builtin_frame_address:
    lda $FD
    ldx $FE
    rts

; --- __builtin_return_address ---
; Returns the return address of the CALLER (level 0).
; When we are called via JSR, the RA of the CALLER is on the hardware stack.

___builtin_return_address:
    ; JSR pushed PCH then PCL.
    ; SP points to PCL.
    ; SP+1: PCL
    ; SP+2: PCH
    ; X = SP
    ; __sp_base = $0101
    ; addr = __sp_base + X = $0101 + SP = SP+1 (relative to $0100)
    tsx
    lda __sp_base, x        ; PCL
    pha
    lda __sp_base+1, x      ; PCH
    tax
    pla
    ; Result in AX (PCL in A, PCH in X)
    rts
