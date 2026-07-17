; scroll_hw.s — Hardware-level screen helpers for text scroller
;
; Provides MEGA65-native routines for screen manipulation:
;   _scroll_line_left  — shift a screen row left by one character
;   _set_text_color    — fill the colour RAM row with a chosen colour
;   _wait_vsync        — wait for vertical blank (raster line 0)
;   _clear_screen      — fill screen RAM with spaces, colour RAM with a colour
;
; All functions use proc/endproc for correct C calling convention.

.cpu _45gs02
.segmentOrder code, data, bss

.extern __sp_base

.global _scroll_line_left
.global _set_text_color
.global _wait_vsync
.global _clear_screen

SCREEN  = $0800
COLRAM  = $d800
COLS    = 40
ROWS    = 25
scratch = $02

.code

; void scroll_line_left(int row)
;   Shift row left by 1 char: cols 1..39 move to 0..38, col 39 = space
proc _scroll_line_left, W#_p_row
    ; compute row * 40 as screen offset
    lda.sp _p_row           ; lo byte of row
    asl                     ; *2
    asl                     ; *4
    asl                     ; *8
    sta scratch             ; save *8
    lda.sp _p_row
    asl                     ; *2
    asl                     ; *4
    asl                     ; *8
    asl                     ; *16
    asl                     ; *32
    clc
    adc scratch             ; *32 + *8 = *40
    tax                     ; X = row offset lo

    ; copy bytes: src = SCREEN+offset+1, dst = SCREEN+offset, len = 39
    ldy #0
@copy:
    lda SCREEN+1,x
    sta SCREEN,x
    inx
    iny
    cpy #(COLS-1)
    bne @copy
    ; put space in last column
    lda #$20
    sta SCREEN,x
    rts
    endproc

; void set_text_color(int row, char color)
;   Fill colour RAM for the given row with color byte
proc _set_text_color, W#_p_row, B#_p_color
    lda.sp _p_row           ; lo byte of row
    asl
    asl
    asl
    sta scratch
    lda.sp _p_row
    asl
    asl
    asl
    asl
    asl
    clc
    adc scratch
    tax                     ; X = row * 40

    lda.sp _p_color
    ldy #0
@fill:
    sta COLRAM,x
    inx
    iny
    cpy #COLS
    bne @fill
    rts
    endproc

; void wait_vsync(void)
;   Busy-wait until raster line reaches 0 (top of frame)
proc _wait_vsync
@wait1:
    lda $d012
    bne @wait1              ; wait for line = 0
    lda $d011
    and #$80
    bne @wait1              ; check bit 8 of raster
    rts
    endproc

; void clear_screen(char color)
;   Fill screen with spaces, colour RAM with given colour
proc _clear_screen, B#_p_color
    lda.sp _p_color         ; color
    sta scratch             ; save color

    ldx #0
    lda #$20               ; space
@cls:
    sta SCREEN,x
    sta SCREEN+$0100,x
    sta SCREEN+$0200,x
    sta SCREEN+$0300,x
    dex
    bne @cls

    lda scratch             ; restore color
    ldx #0
@clr:
    sta COLRAM,x
    sta COLRAM+$0100,x
    sta COLRAM+$0200,x
    sta COLRAM+$0300,x
    dex
    bne @clr
    rts
    endproc
