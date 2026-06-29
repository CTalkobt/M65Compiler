; float_rom.s — CBM 40-bit float runtime via BASIC 65 ROM
;
; Uses MAP instruction to bank in BASIC ROM at $2000-$7FFF,
; calls float routines via jump table at $7F00, then restores MAP.
;
; FAC (Floating Point Accumulator) at $63-$68
; ARG (secondary accumulator) at $6A-$6F
;
; Float values are 5 bytes: exponent (1) + mantissa (4), big-endian.
; Stored in memory as: [exponent, mantissa_msb, ..., mantissa_lsb]

    .cpu 45gs02

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

; FAC/ARG ZP locations
FAC_EX = $63
FAC_M1 = $64
FAC_M2 = $65
FAC_M3 = $66
FAC_M4 = $67
FAC_SI = $68
ARG_EX = $6A

; ===========================================================================
; Load 5-byte float from memory (AX = pointer) into FAC
; ===========================================================================
.global __float_load_fac
__float_load_fac:
    sta $02
    stx $03
    ldy #0
    lda ($02),y
    sta FAC_EX
    iny
    lda ($02),y
    sta FAC_M1
    iny
    lda ($02),y
    sta FAC_M2
    iny
    lda ($02),y
    sta FAC_M3
    iny
    lda ($02),y
    sta FAC_M4
    lda #0
    sta FAC_SI
    rts

; ===========================================================================
; Load 5-byte float from memory (AX = pointer) into ARG
; ===========================================================================
.global __float_load_arg
__float_load_arg:
    sta $02
    stx $03
    ldy #0
    lda ($02),y
    sta ARG_EX
    iny
    lda ($02),y
    sta $6B
    iny
    lda ($02),y
    sta $6C
    iny
    lda ($02),y
    sta $6D
    iny
    lda ($02),y
    sta $6E
    lda #0
    sta $6F
    rts

; ===========================================================================
; Store FAC to memory (AX = destination, 5 bytes)
; ===========================================================================
.global __float_store_fac
__float_store_fac:
    sta $02
    stx $03
    ldy #0
    lda FAC_EX
    sta ($02),y
    iny
    lda FAC_M1
    sta ($02),y
    iny
    lda FAC_M2
    sta ($02),y
    iny
    lda FAC_M3
    sta ($02),y
    iny
    lda FAC_M4
    sta ($02),y
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
    ldz #$83
    ldy #$00
    ldx #$e0
    lda #$00
    map
    eom
    jsr BJT_FADDT
    ldz #$b3
    ldy #$00
    ldx #$e3
    lda #$00
    map
    eom
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
    ldz #$83
    ldy #$00
    ldx #$e0
    lda #$00
    map
    eom
    jsr BJT_FSUBT
    ldz #$b3
    ldy #$00
    ldx #$e3
    lda #$00
    map
    eom
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
    ldz #$83
    ldy #$00
    ldx #$e0
    lda #$00
    map
    eom
    jsr BJT_FMULTT
    ldz #$b3
    ldy #$00
    ldx #$e3
    lda #$00
    map
    eom
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
    ldz #$83
    ldy #$00
    ldx #$e0
    lda #$00
    map
    eom
    jsr BJT_FDIVT
    ldz #$b3
    ldy #$00
    ldx #$e3
    lda #$00
    map
    eom
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
    ldz #$83
    ldy #$00
    ldx #$e0
    lda #$00
    map
    eom
    jsr BJT_NEGOP
    ldz #$b3
    ldy #$00
    ldx #$e3
    lda #$00
    map
    eom
    lda #<__float_a
    ldx #>__float_a
    jsr __float_store_fac
    rts

; ===========================================================================
; Compare: __float_cmp  (a vs b → A = $FF/-1, $00, $01)
; ===========================================================================
.global __float_cmp
__float_cmp:
    ; Load b into FAC, a into ARG
    lda #<__float_b
    ldx #>__float_b
    jsr __float_load_fac
    lda #<__float_a
    ldx #>__float_a
    jsr __float_load_arg
    ; FCOMP compares ARG with FAC: A=$FF if ARG<FAC, $00 if equal, $01 if ARG>FAC
    ; But FCOMP takes a memory pointer in AY, not ARG... use different approach:
    ; Store FAC to temp, load a into FAC, then compare FAC with temp
    ; Actually, BJT_FCOMP expects: compare mem (pointed by AY) with FAC
    ; So: load b into FAC, then compare __float_a (mem) with FAC
    ldz #$83
    ldy #$00
    ldx #$e0
    lda #$00
    map
    eom
    ; ARG already has 'a', FAC has 'b'
    ; MOVFA: FAC = ARG (now FAC = a)
    jsr BJT_MOVFA
    ldz #$b3
    ldy #$00
    ldx #$e3
    lda #$00
    map
    eom
    ; Now FAC = a, we want to compare with b (in __float_b memory)
    ; FCOMP(AY): compare FAC with memory at AY
    ; Result: A = $FF if FAC < mem, $00 if equal, $01 if FAC > mem
    lda #<__float_b
    ldy #>__float_b
    ldz #$83
    ldx #$e0
    ; Need to preserve AY across MAP...
    pha
    phy
    lda #$00
    map
    eom
    ply
    pla
    jsr BJT_FCOMP
    pha
    ldz #$b3
    ldy #$00
    ldx #$e3
    lda #$00
    map
    eom
    pla
    ; A = comparison result: $FF (<), $00 (=), $01 (>)
    rts

; ===========================================================================
; Conversion: __float_itof (int AX → float at __float_a)
; ===========================================================================
.global __float_itof
__float_itof:
    tay             ; Y = low byte
    txa             ; A = high byte (GIVAYF wants A=high, Y=low)
    ldz #$83
    ldy #$00
    ldx #$e0
    lda #$00
    map
    eom
    jsr BJT_GIVAYF
    ldz #$b3
    ldy #$00
    ldx #$e3
    lda #$00
    map
    eom
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
    ldz #$83
    ldy #$00
    ldx #$e0
    lda #$00
    map
    eom
    jsr BJT_AYINT
    ldz #$b3
    ldy #$00
    ldx #$e3
    lda #$00
    map
    eom
    lda FAC_M3      ; low byte of result
    ldx FAC_M4      ; high byte
    rts

; ===========================================================================
; Math: __float_sin, __float_cos, etc.
; ===========================================================================
.global __float_sin
__float_sin:
    lda #<__float_a
    ldx #>__float_a
    jsr __float_load_fac
    ldz #$83
    ldy #$00
    ldx #$e0
    lda #$00
    map
    eom
    jsr BJT_SIN
    ldz #$b3
    ldy #$00
    ldx #$e3
    lda #$00
    map
    eom
    lda #<__float_a
    ldx #>__float_a
    jsr __float_store_fac
    rts

.global __float_cos
__float_cos:
    lda #<__float_a
    ldx #>__float_a
    jsr __float_load_fac
    ldz #$83
    ldy #$00
    ldx #$e0
    lda #$00
    map
    eom
    jsr BJT_COS
    ldz #$b3
    ldy #$00
    ldx #$e3
    lda #$00
    map
    eom
    lda #<__float_a
    ldx #>__float_a
    jsr __float_store_fac
    rts

.global __float_tan
__float_tan:
    lda #<__float_a
    ldx #>__float_a
    jsr __float_load_fac
    ldz #$83
    ldy #$00
    ldx #$e0
    lda #$00
    map
    eom
    jsr BJT_TAN
    ldz #$b3
    ldy #$00
    ldx #$e3
    lda #$00
    map
    eom
    lda #<__float_a
    ldx #>__float_a
    jsr __float_store_fac
    rts

.global __float_atn
__float_atn:
    lda #<__float_a
    ldx #>__float_a
    jsr __float_load_fac
    ldz #$83
    ldy #$00
    ldx #$e0
    lda #$00
    map
    eom
    jsr BJT_ATN
    ldz #$b3
    ldy #$00
    ldx #$e3
    lda #$00
    map
    eom
    lda #<__float_a
    ldx #>__float_a
    jsr __float_store_fac
    rts

.global __float_log
__float_log:
    lda #<__float_a
    ldx #>__float_a
    jsr __float_load_fac
    ldz #$83
    ldy #$00
    ldx #$e0
    lda #$00
    map
    eom
    jsr BJT_LOG
    ldz #$b3
    ldy #$00
    ldx #$e3
    lda #$00
    map
    eom
    lda #<__float_a
    ldx #>__float_a
    jsr __float_store_fac
    rts

.global __float_exp
__float_exp:
    lda #<__float_a
    ldx #>__float_a
    jsr __float_load_fac
    ldz #$83
    ldy #$00
    ldx #$e0
    lda #$00
    map
    eom
    jsr BJT_EXP
    ldz #$b3
    ldy #$00
    ldx #$e3
    lda #$00
    map
    eom
    lda #<__float_a
    ldx #>__float_a
    jsr __float_store_fac
    rts

.global __float_sqr
__float_sqr:
    lda #<__float_a
    ldx #>__float_a
    jsr __float_load_fac
    ldz #$83
    ldy #$00
    ldx #$e0
    lda #$00
    map
    eom
    jsr BJT_SQR
    ldz #$b3
    ldy #$00
    ldx #$e3
    lda #$00
    map
    eom
    lda #<__float_a
    ldx #>__float_a
    jsr __float_store_fac
    rts

.global __float_abs
__float_abs:
    lda #<__float_a
    ldx #>__float_a
    jsr __float_load_fac
    ldz #$83
    ldy #$00
    ldx #$e0
    lda #$00
    map
    eom
    jsr BJT_ABS
    ldz #$b3
    ldy #$00
    ldx #$e3
    lda #$00
    map
    eom
    lda #<__float_a
    ldx #>__float_a
    jsr __float_store_fac
    rts
