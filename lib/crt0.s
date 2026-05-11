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

    jsr _init_features
    jsr _main

    ; Fall through to __exit
__exit:
    ; Restore caller's stack pointer and return
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
