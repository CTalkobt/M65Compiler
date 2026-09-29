; ca65 test file - CC65 format
.import extern_func
.export main

.segment "CODE"

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
  jmp @done

  ; Indexed addressing
@loop:
  lda table, X
  sta $0400, X
  dex
  bne @loop

  ; Indirect addressing
  lda ($80)
  sta ($81), Y

  ; Bit operations
  bit $82
  bit #$20

@done:
  rts

.segment "DATA"
table:
  .byte 1, 2, 3, 4, 5
  .word $1000, $2000
  .asciiz "HELLO"
