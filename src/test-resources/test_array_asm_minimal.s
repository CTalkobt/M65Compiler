; Test array initialization and access in pure assembly
; This isolates the issue from compiler codegen

* = $2000

main:
    ; === Phase 1: Initialize array on simulated frame ===
    ; Frame array at $2003 (after 3 bytes of frame header)
    ; Offsets: 0,2,4 for arr[0], arr[1], arr[2]

    ; Store arr[0] = 100
    lda #$64
    sta $2003           ; arr[0] lo
    lda #$00
    sta $2004           ; arr[0] hi

    ; Store arr[1] = 200
    lda #$C8
    sta $2005           ; arr[1] lo
    lda #$00
    sta $2006           ; arr[1] hi

    ; Store arr[2] = 300 ($012C)
    lda #$2C
    sta $2007           ; arr[2] lo
    lda #$01
    sta $2008           ; arr[2] hi

    ; === Phase 2: Verify initialization (direct read) ===
    lda $2003           ; Read arr[0] lo
    sta $4000           ; Output
    lda $2004           ; Read arr[0] hi
    sta $4001

    lda $2005           ; Read arr[1] lo
    sta $4002           ; Should be $C8
    lda $2006           ; Read arr[1] hi
    sta $4003           ; Should be 0

    ; === Phase 3: Read arr[1] via indirect addressing ===
    ; Setup address pair to $2005
    lda #$05            ; Address lo
    sta $08
    lda #$20            ; Address hi
    sta $09

    ldy #0
    lda ($08),y         ; Load from ($2005),Y
    sta $4004           ; Should be $C8
    iny
    lda ($08),y         ; Load high byte
    sta $4005           ; Should be 0

    ; Mark complete
    lda #$AA
    sta $4006

    brk
