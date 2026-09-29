; float_rom_indirect.s — CBM 40-bit float runtime via BASIC 65 jump table
;
; Alternative to float_rom.s that uses MOVFM/MOVMF/MOVFA/MOVAF via JSRFAR
; instead of direct ZP access to FAC ($63-$68) and ARG ($6A-$6F).
;
; Trade-offs:
;   - Direct (float_rom.s): faster, but tied to specific ZP addresses
;   - Indirect (this file): slower (JSR overhead), but portable across ROM versions
;
; Build with: ca45 -c float_rom_indirect.s -o float_rom.o45
; Use instead of float_rom.s when -ffp-indirect is specified.

    .cpu 45gs02

; KERNAL far-call entry
JSRFAR      = $FF6E
BASIC_BANK  = $03      ; BASIC ROM bank (C65 bank 3)

; BASIC 65 jump table addresses
BJT_AYINT   = $7F00
BJT_GIVAYF  = $7F03
BJT_FADDT   = $7F1B
BJT_FSUBT   = $7F15
BJT_FMULTT  = $7F21
BJT_FDIVT   = $7F27
BJT_NEGOP   = $7F33
BJT_FCOMP   = $7F54
BJT_MOVFA   = $7F69
BJT_MOVAF   = $7F6C
BJT_MOVFM   = $7F63
BJT_MOVMF   = $7F66
BJT_LOG     = $7F2A
BJT_SQR     = $7F30
BJT_EXP     = $7F3C
BJT_SIN     = $7F42
BJT_COS     = $7F3F
BJT_TAN     = $7F45
BJT_ATN     = $7F48
BJT_ABS     = $7F4E

; FAC/ARG ZP locations (still needed for reading results after ROM calls)
FAC_EX = $63
FAC_M3 = $66
FAC_M4 = $67
FAC_SI = $68

; ===========================================================================
; Helper: call BASIC ROM routine via JSRFAR
; Call with target address in AX (A=low, X=high)
; ===========================================================================
__jsrfar_basic:
    sta $04             ; PCL
    stx $03             ; PCH
    lda #BASIC_BANK
    sta $02             ; Bank
    jsr JSRFAR
    rts

; ===========================================================================
; Helper: call BASIC ROM with register preload via JSRFAR
; JSRFAR loads A from $06, X from $07, Y from $09 before calling target
; ===========================================================================
__jsrfar_basic_regs:
    lda #BASIC_BANK
    sta $02             ; Bank
    jsr JSRFAR
    rts

; ===========================================================================
; Load 5-byte float from memory (AX = pointer) into FAC
; Uses MOVFM: loads FAC from memory address in A(low)/Y(high)
; ===========================================================================
.global __float_load_fac
__float_load_fac:
    ; JSRFAR preloads A from $06, Y from $09 for the ROM call
    sta $06             ; A register = low byte of source pointer
    stx $09             ; Y register = high byte of source pointer
    lda #>BJT_MOVFM
    sta $03             ; PCH
    lda #<BJT_MOVFM
    sta $04             ; PCL
    jsr __jsrfar_basic_regs
    rts

; ===========================================================================
; Load 5-byte float from memory (AX = pointer) into ARG
; Uses MOVFM to load into FAC, then MOVAF to copy FAC → ARG
; ===========================================================================
.global __float_load_arg
__float_load_arg:
    ; First load into FAC
    sta $06             ; A = low byte
    stx $09             ; Y = high byte
    lda #>BJT_MOVFM
    sta $03
    lda #<BJT_MOVFM
    sta $04
    jsr __jsrfar_basic_regs
    ; Copy FAC → ARG
    lda #<BJT_MOVAF
    ldx #>BJT_MOVAF
    jsr __jsrfar_basic
    rts

; ===========================================================================
; Store FAC to memory (AX = destination, 5 bytes)
; Uses MOVMF: stores FAC to memory address in X(high)/Y(low)
; ===========================================================================
.global __float_store_fac
__float_store_fac:
    ; JSRFAR preloads X from $07, Y from $09 for the ROM call
    sta $09             ; Y register = low byte of dest pointer
    stx $07             ; X register = high byte of dest pointer
    lda #>BJT_MOVMF
    sta $03             ; PCH
    lda #<BJT_MOVMF
    sta $04             ; PCL
    jsr __jsrfar_basic_regs
    rts

; ===========================================================================
; Shared data buffers
; ===========================================================================
.global __float_a
__float_a: .res 5

.global __float_b
__float_b: .res 5

; ===========================================================================
; Binary: __float_add  (result = a + b)
; ===========================================================================
.global __float_add
__float_add:
    lda #<__float_b
    ldx #>__float_b
    jsr __float_load_fac
    lda #<__float_a
    ldx #>__float_a
    jsr __float_load_arg
    lda #<BJT_FADDT
    ldx #>BJT_FADDT
    jsr __jsrfar_basic
    lda #<__float_a
    ldx #>__float_a
    jsr __float_store_fac
    rts

; ===========================================================================
; Binary: __float_sub  (result = a - b)
; ===========================================================================
.global __float_sub
__float_sub:
    lda #<__float_b
    ldx #>__float_b
    jsr __float_load_fac
    lda #<__float_a
    ldx #>__float_a
    jsr __float_load_arg
    lda #<BJT_FSUBT
    ldx #>BJT_FSUBT
    jsr __jsrfar_basic
    lda #<__float_a
    ldx #>__float_a
    jsr __float_store_fac
    rts

; ===========================================================================
; Binary: __float_mul  (result = a * b)
; ===========================================================================
.global __float_mul
__float_mul:
    lda #<__float_b
    ldx #>__float_b
    jsr __float_load_fac
    lda #<__float_a
    ldx #>__float_a
    jsr __float_load_arg
    lda #<BJT_FMULTT
    ldx #>BJT_FMULTT
    jsr __jsrfar_basic
    lda #<__float_a
    ldx #>__float_a
    jsr __float_store_fac
    rts

; ===========================================================================
; Binary: __float_div  (result = a / b)
; ===========================================================================
.global __float_div
__float_div:
    lda #<__float_b
    ldx #>__float_b
    jsr __float_load_fac
    lda #<__float_a
    ldx #>__float_a
    jsr __float_load_arg
    lda #<BJT_FDIVT
    ldx #>BJT_FDIVT
    jsr __jsrfar_basic
    lda #<__float_a
    ldx #>__float_a
    jsr __float_store_fac
    rts

; ===========================================================================
; Unary: __float_neg  (result = -a)
; ===========================================================================
.global __float_neg
__float_neg:
    lda #<__float_a
    ldx #>__float_a
    jsr __float_load_fac
    lda #<BJT_NEGOP
    ldx #>BJT_NEGOP
    jsr __jsrfar_basic
    lda #<__float_a
    ldx #>__float_a
    jsr __float_store_fac
    rts

; ===========================================================================
; Compare: __float_cmp  (a vs b → A = $FF/-1, $00, $01)
; ===========================================================================
.global __float_cmp
__float_cmp:
    lda #<__float_a
    ldx #>__float_a
    jsr __float_load_fac
    lda #<__float_b
    ldx #>__float_b
    jsr __float_load_arg
    lda #<BJT_MOVFA
    ldx #>BJT_MOVFA
    jsr __jsrfar_basic
    ; FAC = a. Subtract b to compare.
    lda #<BJT_FSUBT
    ldx #>BJT_FSUBT
    jsr __jsrfar_basic
    ; FAC = a - b. Check sign via direct ZP read (result is already there).
    lda FAC_EX
    beq @zero
    lda FAC_SI
    bne @negative
    lda #$01
    rts
@negative:
    lda #$FF
    rts
@zero:
    lda #$00
    rts

; ===========================================================================
; Conversion: __float_itof (int AX → float at __float_a)
; ===========================================================================
.global __float_itof
__float_itof:
    tay
    txa
    sta $06
    sty $09
    lda #BASIC_BANK
    sta $02
    lda #>BJT_GIVAYF
    sta $03
    lda #<BJT_GIVAYF
    sta $04
    jsr JSRFAR
    lda #<__float_a
    ldx #>__float_a
    jsr __float_store_fac
    rts

; ===========================================================================
; Conversion: __float_ftoi (float at __float_a → int AX)
; ===========================================================================
.global __float_ftoi
__float_ftoi:
    lda #<__float_a
    ldx #>__float_a
    jsr __float_load_fac
    lda #<BJT_AYINT
    ldx #>BJT_AYINT
    jsr __jsrfar_basic
    ; Result in FAC mantissa bytes (direct ZP read — ROM already wrote here)
    lda FAC_M3
    ldx FAC_M4
    rts

; ===========================================================================
; Math: unary float functions
; ===========================================================================
.global __float_sin
__float_sin:
    lda #<__float_a
    ldx #>__float_a
    jsr __float_load_fac
    lda #<BJT_SIN
    ldx #>BJT_SIN
    jsr __jsrfar_basic
    lda #<__float_a
    ldx #>__float_a
    jsr __float_store_fac
    rts

.global __float_cos
__float_cos:
    lda #<__float_a
    ldx #>__float_a
    jsr __float_load_fac
    lda #<BJT_COS
    ldx #>BJT_COS
    jsr __jsrfar_basic
    lda #<__float_a
    ldx #>__float_a
    jsr __float_store_fac
    rts

.global __float_tan
__float_tan:
    lda #<__float_a
    ldx #>__float_a
    jsr __float_load_fac
    lda #<BJT_TAN
    ldx #>BJT_TAN
    jsr __jsrfar_basic
    lda #<__float_a
    ldx #>__float_a
    jsr __float_store_fac
    rts

.global __float_atn
__float_atn:
    lda #<__float_a
    ldx #>__float_a
    jsr __float_load_fac
    lda #<BJT_ATN
    ldx #>BJT_ATN
    jsr __jsrfar_basic
    lda #<__float_a
    ldx #>__float_a
    jsr __float_store_fac
    rts

.global __float_log
__float_log:
    lda #<__float_a
    ldx #>__float_a
    jsr __float_load_fac
    lda #<BJT_LOG
    ldx #>BJT_LOG
    jsr __jsrfar_basic
    lda #<__float_a
    ldx #>__float_a
    jsr __float_store_fac
    rts

.global __float_exp
__float_exp:
    lda #<__float_a
    ldx #>__float_a
    jsr __float_load_fac
    lda #<BJT_EXP
    ldx #>BJT_EXP
    jsr __jsrfar_basic
    lda #<__float_a
    ldx #>__float_a
    jsr __float_store_fac
    rts

.global __float_sqr
__float_sqr:
    lda #<__float_a
    ldx #>__float_a
    jsr __float_load_fac
    lda #<BJT_SQR
    ldx #>BJT_SQR
    jsr __jsrfar_basic
    lda #<__float_a
    ldx #>__float_a
    jsr __float_store_fac
    rts

.global __float_abs
__float_abs:
    lda #<__float_a
    ldx #>__float_a
    jsr __float_load_fac
    lda #<BJT_ABS
    ldx #>BJT_ABS
    jsr __jsrfar_basic
    lda #<__float_a
    ldx #>__float_a
    jsr __float_store_fac
    rts
