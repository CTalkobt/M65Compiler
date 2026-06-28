; float_rom.s — CBM 40-bit float runtime via BASIC 65 ROM
;
; Uses JSRFAR ($FF6E) to call BASIC ROM float routines.
; JSRFAR handles MAP banking automatically — no overlay conflicts.
;
; JSRFAR convention: Bank=$02, PCH=$03, PCL=$04
; These ZP locations are scratch (used by DMA ops and startup code)
; and are free at the point float routines are called.
;
; FAC (Floating Point Accumulator) at $63-$68
; ARG (secondary accumulator) at $6A-$6F
;
; Float values are 5 bytes: exponent (1) + mantissa (4), big-endian.

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

; FAC/ARG ZP locations
FAC_EX = $63
FAC_M1 = $64
FAC_M2 = $65
FAC_M3 = $66
FAC_M4 = $67
FAC_SI = $68
ARG_EX = $6A

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
    ; Load a into FAC, b into ARG
    lda #<__float_a
    ldx #>__float_a
    jsr __float_load_fac
    lda #<__float_b
    ldx #>__float_b
    jsr __float_load_arg
    ; FCOMP: compare MEM (AY) with FAC → A = -1/0/1
    ; But we need FAC vs ARG. Use MOVFA to get a into FAC,
    ; then compare FAC with __float_b in memory.
    ; Actually: load b into FAC first, then a into ARG, then MOVFA (FAC=ARG=a)
    ; Then FCOMP(__float_b) compares a(FAC) with b(mem)
    lda #<BJT_MOVFA
    ldx #>BJT_MOVFA
    jsr __jsrfar_basic
    ; Now FAC = a. Compare with __float_b in memory.
    ; FCOMP takes pointer in AY, compares (AY) with FAC
    ; Result: A=$FF if FAC < (AY), $00 if equal, $01 if FAC > (AY)
    lda #<__float_b
    ldy #>__float_b
    sta $04             ; PCL for FCOMP
    sty $03             ; PCH
    ; Wait — FCOMP takes MEM pointer in AY, not via JSRFAR
    ; We need FCOMP to read from our RAM, but via JSRFAR it reads from ROM bank
    ; Use direct FCOMP instead — but that needs BASIC ROM mapped in
    ; For now: use subtraction and check sign
    lda #<BJT_FSUBT
    ldx #>BJT_FSUBT
    jsr __jsrfar_basic
    ; FAC = a - b. Check sign.
    lda FAC_EX
    beq @zero           ; exponent 0 = value is 0 (equal)
    lda FAC_SI
    bne @negative        ; sign byte nonzero = negative
    lda #$01            ; positive: a > b
    rts
@negative:
    lda #$FF            ; negative: a < b
    rts
@zero:
    lda #$00            ; equal
    rts

; ===========================================================================
; Conversion: __float_itof (int AX → float at __float_a)
; ===========================================================================
.global __float_itof
__float_itof:
    tay             ; Y = low byte
    txa             ; A = high byte (GIVAYF wants A=high, Y=low)
    sta $06         ; save A (high) — JSRFAR uses $02-$04
    sty $09         ; save Y (low)
    lda #<BJT_GIVAYF
    ldx #>BJT_GIVAYF
    ; Set up JSRFAR: AC=$06, YR=$09 will be loaded by JSRFAR
    lda $06
    sta $06         ; AC register for JSRFAR
    lda $09
    sta $09         ; YR register for JSRFAR
    ; Actually JSRFAR loads A/X/Y/Z from $06-$09 before calling
    ; So we need: $06=A(high byte), $09=Y(low byte)
    ; But we also need to set $02=bank, $03=PCH, $04=PCL
    lda #BASIC_BANK
    sta $02
    lda #>BJT_GIVAYF
    sta $03
    lda #<BJT_GIVAYF
    sta $04
    jsr JSRFAR
    ; FAC now has the float value
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
    lda FAC_M3      ; low byte of result
    ldx FAC_M4      ; high byte
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
