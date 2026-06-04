; crt0.s — Default C Runtime Startup for cc45
;
; This is the entry point for all C programs. The linker's -basic
; stub (or direct-compile wrapper) calls __init, which:
;   1. Saves the stack pointer (for clean return to caller/BASIC)
;   2. Calls _init_features (weak — overridable for platform setup)
;   3. Calls _main
;   4. Restores SP and returns to caller
;
; To override the init hook, provide a strong _init_features in another
; object file (e.g., crt0_mega65.s) and link it before or after crt0.
;
; Usage (relocatable):
;   ln45 -prg crt0.o45 main.o45 -o program.prg

.global __init
.global __exit
.global __sp_base
.weak _init_features
.extern _main

.segment "init"

__sp_base = $0101

__init:
    ; Save caller's stack pointer for clean return
    tsx
    stx __saved_spl + 1
    tsy
    sty __saved_sph + 1

    ; Enable MEGA65 I/O before DMA — GS knock must precede $D700 access
    lda #$47            ; 'G'
    sta $D02F
    lda #$53            ; 'S'
    sta $D02F

    ; Save ZP $08-$FF to BSS buffer via static DMA job
    ldz #0              ; Z must be 0 for stz bank clears below
    stz $D704           ; megabyte bank = 0 (clear stale KERNAL value)
    stz $D702           ; bank = 0
    lda #>__dma_save
    sta $D701           ; DMA list address MSB
    lda #<__dma_save
    sta $D700           ; DMA list address LSB — triggers DMA

    jsr _init_features
    jsr _main

    ; Fall through to __exit
__exit:
    ; Restore ZP $08-$FF from BSS buffer via static DMA job
    ldz #0              ; Z may have changed during program execution
    stz $D704           ; megabyte bank = 0
    stz $D702           ; bank = 0
    lda #>__dma_restore
    sta $D701           ; DMA list address MSB
    lda #<__dma_restore
    sta $D700           ; DMA list address LSB — triggers DMA

    ; Restore caller's stack pointer and return.
__saved_spl:
    ldx #$FF
    txs
__saved_sph:
    ldy #$01
    tys
    rts

; Default init_features — does nothing. Override with a strong definition
; to set up hardware (16-bit stack, I/O mapping, DMA, etc.)
_init_features:
    rts

; Static DMA command blocks (12 bytes each, linker patches BSS address)
.segment "data"
__dma_save:
    .byte $00                           ; command: COPY
    .word 248                           ; count
    .word $08                           ; source: ZP $08
    .byte $00                           ; source bank
    .word __zp_save_buf                 ; dest: BSS buffer
    .byte $00                           ; dest bank
    .byte $00                           ; command MSB
    .byte $00                           ; modulo

__dma_restore:
    .byte $00                           ; command: COPY
    .word 248                           ; count
    .word __zp_save_buf                 ; source: BSS buffer
    .byte $00                           ; source bank
    .word $08                           ; dest: ZP $08
    .byte $00                           ; dest bank
    .byte $00                           ; command MSB
    .byte $00                           ; modulo

.segment "bss"
__zp_save_buf:
    .res 248
