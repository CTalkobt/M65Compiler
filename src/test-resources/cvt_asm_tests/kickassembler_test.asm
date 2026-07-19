// KickAssembler test file

.pc = $0800

table:
  !byte 1, 2, 3, 4, 5
  !word $1000, $2000

main:
  // Immediate addressing
  lda #$20
  ldx #$00
  ldy #$FF

  // Zero page
  lda $80
  sta $81
  cmp $82

  // Absolute addressing
  lda $D000
  sta $D020
  jmp done

  // Indexed addressing
loop:
  lda table, x
  sta $0400, x
  dex
  bne loop

  // Indirect addressing
  lda ($80)
  sta ($81), y

  // Bit operations
  bit $82

done:
  rts
