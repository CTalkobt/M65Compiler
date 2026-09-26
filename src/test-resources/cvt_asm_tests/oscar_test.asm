* Oscar test file
	ORG $0800

TABLE	BYTE 1, 2, 3, 4, 5
	WORD $1000, $2000

MAIN	; Immediate addressing
	LDA #$20
	LDX #$00
	LDY #$FF

	; Zero page
	LDA $80
	STA $81
	CMP $82

	; Absolute addressing
	LDA $D000
	STA $D020
	JMP DONE

	; Indexed addressing
LOOP	LDA TABLE, X
	STA $0400, X
	DEX
	BNE LOOP

	; Indirect addressing
	LDA ($80)
	STA ($81), Y

	; Bit operations
	BIT $82

DONE	RTS
