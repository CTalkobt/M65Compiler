; MEGA65 Serial UART Implementation
; Low-level UART hardware access for serial communication
;
; Uses standard cc45 calling conventions

.cpu 45GS02

; UART Hardware Register Addresses
UART_DATA_REG   = $D600
UART_STATUS_REG = $D601
UART_RXRDY      = $01        ; Receive ready flag (bit 0)
UART_TXRDY      = $02        ; Transmit ready flag (bit 1)

.global _uart_putchar
.global _uart_puts
.global _uart_getchar
.global _uart_rx_ready
.global _uart_tx_ready
.extern __sp_base

.segment "code"

; void uart_putchar(unsigned char c)
proc _uart_putchar, W#_p_c
    .var _fp = 0
    ; Get character from parameter
    lda.sp _p_c

    ; Wait for TX ready
@wait:
    pha                         ; Save char
    lda UART_STATUS_REG
    and #UART_TXRDY
    beq @wait

    pla                         ; Restore char
    sta UART_DATA_REG           ; Write to UART
    rtn #0
endproc

; void uart_puts(const char *str)
proc _uart_puts, W#_p_str
    .var _fp = 0
    ; Save ZP locations
    lda $21
    pha
    lda $22
    pha

    ; Load string pointer
    ldax _p_str+2, sp
    sta $21
    stx $22

    ; Loop through string
    ldy #0
@loop:
    lda ($21),y
    beq @done

    ; Wait for TX ready
@wait:
    pha                         ; Save char
    lda UART_STATUS_REG
    and #UART_TXRDY
    beq @wait

    pla                         ; Restore char
    sta UART_DATA_REG           ; Write byte

    ; Next char
    iny
    cpy #0
    bne @loop

    ; Page boundary: increment high byte
    inc $22
    jmp @loop

@done:
    ; Restore ZP
    pla
    sta $22
    pla
    sta $21

    ldax #0
    rtn #0
endproc

; unsigned char uart_getchar(void)
proc _uart_getchar
    .var _fp = 0
    ; Wait for RX ready
@wait:
    lda UART_STATUS_REG
    and #UART_RXRDY
    beq @wait

    ; Read byte
    lda UART_DATA_REG
    rtn #0
endproc

; unsigned char uart_rx_ready(void)
proc _uart_rx_ready
    .var _fp = 0
    lda UART_STATUS_REG
    and #UART_RXRDY
    rtn #0
endproc

; unsigned char uart_tx_ready(void)
proc _uart_tx_ready
    .var _fp = 0
    lda UART_STATUS_REG
    and #UART_TXRDY
    rtn #0
endproc
