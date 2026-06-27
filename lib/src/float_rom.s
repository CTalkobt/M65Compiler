; float_rom.s — CBM 40-bit float runtime via BASIC 65 ROM
;
; Uses MAP instruction to bank in BASIC ROM at $2000-$7FFF,
; calls float routines via jump table at $7F00, then restores MAP.
;
; FAC (Floating Point Accumulator) at $63-$68
; ARG (secondary accumulator) at $6A-$6F
; Rounding byte at $71
;
; Float values are 5 bytes: exponent (1) + mantissa (4), big-endian.
; Stored in memory as: [exponent, mantissa_msb, ..., mantissa_lsb]

    .cpu 45gs02

; BASIC 65 jump table addresses
BJT_AYINT   = $7F00   ; float → integer
BJT_GIVAYF  = $7F03   ; .AY integer → float FAC
BJT_FADDT   = $7F1B   ; FAC = ARG + FAC
BJT_FSUBT   = $7F15   ; FAC = ARG - FAC
BJT_FMULTT  = $7F21   ; FAC = ARG * FAC
BJT_FDIVT   = $7F27   ; FAC = ARG / FAC
BJT_NEGOP   = $7F33   ; FAC = -FAC
BJT_FCOMP   = $7F54   ; compare FAC with MEM
BJT_MOVFA   = $7F69   ; FAC = ARG
BJT_MOVAF   = $7F6C   ; ARG = FAC
BJT_MOVFM   = $7F63   ; MEM → FAC (ROM)
BJT_MOVMF   = $7F66   ; FAC → MEM
BJT_LOG     = $7F2A   ; FAC = ln(FAC)
BJT_SQR     = $7F30   ; FAC = sqrt(FAC)
BJT_EXP     = $7F3C   ; FAC = exp(FAC)
BJT_SIN     = $7F42   ; FAC = sin(FAC)
BJT_COS     = $7F3F   ; FAC = cos(FAC)
BJT_TAN     = $7F45   ; FAC = tan(FAC)
BJT_ATN     = $7F48   ; FAC = atn(FAC)
BJT_ABS     = $7F4E   ; FAC = abs(FAC)

; FAC/ARG ZP locations
FAC_EX = $63
FAC_M1 = $64
FAC_M2 = $65
FAC_M3 = $66
FAC_M4 = $67
FAC_SI = $68
ARG_EX = $6A

; ===========================================================================
; MAP helpers: bank in BASIC ROM, restore after
; ===========================================================================

.macro map_basic_in
    ldz #$83
    ldy #$00
    ldx #$e0
    lda #$00
    map
    eom
.endmacro

.macro map_basic_out
    ldz #$b3
    ldy #$00
    ldx #$e3
    lda #$00
    map
    eom
.endmacro

; ===========================================================================
; Load 5-byte float from memory (AX = pointer) into FAC
; ===========================================================================
.global __float_load_fac
__float_load_fac:
    ; AX = source address
    sta $02
    stx $03
    ldy #0
    lda ($02),y : sta FAC_EX
    iny : lda ($02),y : sta FAC_M1
    iny : lda ($02),y : sta FAC_M2
    iny : lda ($02),y : sta FAC_M3
    iny : lda ($02),y : sta FAC_M4
    lda #0 : sta FAC_SI
    rts

; ===========================================================================
; Load 5-byte float from memory (AX = pointer) into ARG
; ===========================================================================
.global __float_load_arg
__float_load_arg:
    sta $02
    stx $03
    ldy #0
    lda ($02),y : sta ARG_EX
    iny : lda ($02),y : sta $6B
    iny : lda ($02),y : sta $6C
    iny : lda ($02),y : sta $6D
    iny : lda ($02),y : sta $6E
    lda #0 : sta $6F
    rts

; ===========================================================================
; Store FAC to memory (AX = destination, 5 bytes)
; ===========================================================================
.global __float_store_fac
__float_store_fac:
    sta $02
    stx $03
    ldy #0
    lda FAC_EX : sta ($02),y
    iny : lda FAC_M1 : sta ($02),y
    iny : lda FAC_M2 : sta ($02),y
    iny : lda FAC_M3 : sta ($02),y
    iny : lda FAC_M4 : sta ($02),y
    rts

; ===========================================================================
; Binary float ops: __float_add, __float_sub, __float_mul, __float_div
; Arguments: src1 at __float_a (5 bytes), src2 at __float_b (5 bytes)
; Result: stored back to __float_a
; ===========================================================================

.global __float_a
__float_a: .res 5

.global __float_b
__float_b: .res 5

.global __float_add
__float_add:
    lda #<__float_b : ldx #>__float_b : jsr __float_load_fac
    lda #<__float_a : ldx #>__float_a : jsr __float_load_arg
    +map_basic_in
    jsr BJT_FADDT
    +map_basic_out
    lda #<__float_a : ldx #>__float_a : jsr __float_store_fac
    rts

.global __float_sub
__float_sub:
    lda #<__float_b : ldx #>__float_b : jsr __float_load_fac
    lda #<__float_a : ldx #>__float_a : jsr __float_load_arg
    +map_basic_in
    jsr BJT_FSUBT
    +map_basic_out
    lda #<__float_a : ldx #>__float_a : jsr __float_store_fac
    rts

.global __float_mul
__float_mul:
    lda #<__float_b : ldx #>__float_b : jsr __float_load_fac
    lda #<__float_a : ldx #>__float_a : jsr __float_load_arg
    +map_basic_in
    jsr BJT_FMULTT
    +map_basic_out
    lda #<__float_a : ldx #>__float_a : jsr __float_store_fac
    rts

.global __float_div
__float_div:
    lda #<__float_b : ldx #>__float_b : jsr __float_load_fac
    lda #<__float_a : ldx #>__float_a : jsr __float_load_arg
    +map_basic_in
    jsr BJT_FDIVT
    +map_basic_out
    lda #<__float_a : ldx #>__float_a : jsr __float_store_fac
    rts

; ===========================================================================
; Unary: __float_neg
; ===========================================================================
.global __float_neg
__float_neg:
    lda #<__float_a : ldx #>__float_a : jsr __float_load_fac
    +map_basic_in
    jsr BJT_NEGOP
    +map_basic_out
    lda #<__float_a : ldx #>__float_a : jsr __float_store_fac
    rts

; ===========================================================================
; Conversion: __float_itof (int AX → float at __float_a)
; ===========================================================================
.global __float_itof
__float_itof:
    tay             ; Y = low byte
    txa             ; A = high byte (givayf wants A=high, Y=low)
    +map_basic_in
    jsr BJT_GIVAYF
    +map_basic_out
    lda #<__float_a : ldx #>__float_a : jsr __float_store_fac
    rts

; ===========================================================================
; Conversion: __float_ftoi (float at __float_a → int AX)
; ===========================================================================
.global __float_ftoi
__float_ftoi:
    lda #<__float_a : ldx #>__float_a : jsr __float_load_fac
    +map_basic_in
    jsr BJT_AYINT
    +map_basic_out
    lda FAC_M3      ; low byte of result
    ldx FAC_M4      ; high byte
    rts

; ===========================================================================
; Math functions: __float_sin, __float_cos, __float_sqrt, etc.
; Operate on __float_a, result in __float_a
; ===========================================================================

.macro float_unary_func addr
    lda #<__float_a : ldx #>__float_a : jsr __float_load_fac
    +map_basic_in
    jsr addr
    +map_basic_out
    lda #<__float_a : ldx #>__float_a : jsr __float_store_fac
    rts
.endmacro

.global __float_sin
__float_sin: +float_unary_func BJT_SIN

.global __float_cos
__float_cos: +float_unary_func BJT_COS

.global __float_tan
__float_tan: +float_unary_func BJT_TAN

.global __float_atn
__float_atn: +float_unary_func BJT_ATN

.global __float_log
__float_log: +float_unary_func BJT_LOG

.global __float_exp
__float_exp: +float_unary_func BJT_EXP

.global __float_sqr
__float_sqr: +float_unary_func BJT_SQR

.global __float_abs
__float_abs: +float_unary_func BJT_ABS
