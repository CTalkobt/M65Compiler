; ca45 test file - comprehensive instruction coverage
.cpu 45GS02

; Symbols
.global main
.extern extern_func
.weak weak_symbol

; Data section
.org $0800

CONST_VAL = $42

TABLE:
  .byte 1, 2, 3, 4, 5
  .word $1000, $2000, $3000

; Code section
main:
  ; Immediate addressing
  lda #$20
  ldx #$00
  ldy #$FF

  ; Zero page
  lda $80
  sta $81
  cmp $82

  ; Absolute addressing
  lda $D000
  sta $D020
  jmp done

  ; Indexed addressing
loop:
  lda table, X
  sta $0400, X
  dex
  bne loop

  ; Indirect addressing
  lda ($80)
  sta ($81), Y

  ; Stack relative (45GS02)
  lda $10, S
  sta $20, S

  ; Frame relative (45GS02)
  lda .fp local_var
  sta .fp other_var

done:
  rts

extern_symbol = $CAFE
