.global _initvideomode
.global _returntobasic
.global _getkey

.cpu _45gs02
.segmentOrder code, data, bss
.code

t0        = $20
t1        = $21
cbp       = $22

getin   = $ffe4
clrchn  = $ffcc
ioinit  = $ff84
cint    = $ff81     

COUNT_1000 = 1000

_getkey:
    jsr $ffe4
    cmp #$51      ; Q PETSCII
    beq yes_key
    lda #$00
    ldx #$00
    rts

yes_key:
    lda #$01
    ldx #$00
    rts

_initvideomode:
    sei

    lda #65
    sta $00

    lda #$47
    sta $d02f
    lda #$53
    sta $d02f

    lda #0
    sta $d020
    sta $d021

    lda $d05d
    ora #%10000000
    sta $d05d

    lda #<$c000
    sta $fb
    lda #>$c000
    sta $fc

    lda #$00
    sta $fd
    lda #$10
    sta $fe

    lda #$e8
    sta t0
    lda #$03
    sta t1

@fillloop:
    .byte $a0,$00  
    lda $fd
    sta ($fb),y
    iny
    lda $fe
    sta ($fb),y

    inc $fd
    bne @noincfe
    inc $fe

@noincfe:
    clc
    lda $fb
    adc #2
    sta $fb
    bcc @nofbcarry
    inc $fc

@nofbcarry:
    sec
    lda t0
    sbc #1
    sta t0
    lda t1
    sbc #0
    sta t1
    lda t0
    ora t1
    bne @fillloop

    lda $d031
    and #%01010111
    sta $d031

    lda #%00000001
    tsb $d054        ; CHR16 ON

    lda #%00000100
    tsb $d054        ; FCLRHI ON / FCM color mode

    lda #%00000010
    trb $d054        ; NCM OFF

    lda #$50
    sta $d058
    lda #$00
    sta $d059

    lda #40
    sta $d05e

    lda $d060
    sta screenptr
    lda $d061
    sta screenptr+1
    lda $d062
    sta screenptr+2

    lda #$00
    sta $d060
    lda #$c0
    sta $d061
    lda #$00
    sta $d062
	
    ; CHARPTR = $040000 per FCM
    lda #$00
    sta $d068
    lda #$00
    sta $d069
    lda #$04
    sta $d06a
    lda #$00
    sta $d06b

    ; ------------------------------------------------------------
    ; clear/init color RAM $0FF80000
    ; 1000 celle * 2 byte = 2000 byte
    ; byte basso / byte alto separati
    ; ------------------------------------------------------------

    lda #$00
    sta cbp
    lda #$00
    sta cbp+1
    lda #$f8
    sta cbp+2
    lda #$0f
    sta cbp+3

    ; 8 pagine = 2048 byte, quindi copre tutte le 1000 celle
    ldx #$08

@clear_color_page:
    ldz #$00

@clear_color_byte:
    lda #$00          ; byte basso Color RAM
    sta [cbp],z

    inz

    lda #$ff          ; byte alto Color RAM
    sta [cbp],z

    inz
    bne @clear_color_byte

    inc cbp+1
    bne @clear_color_next
    inc cbp+2
    bne @clear_color_next
    inc cbp+3

@clear_color_next:
    dex
    bne @clear_color_page


    ; ------------------------------------------------------------
    ; clear FCM char/pixel RAM $040000, 64 KB
    ; ------------------------------------------------------------

    lda #$00
    sta cbp
    lda #$00
    sta cbp+1
    lda #$04
    sta cbp+2
    lda #$00
    sta cbp+3

    ; 64 KB = 256 pagine da 256 byte
    ldx #$00

@clear_screen_page:
    ldz #$00

@clear_screen_byte:
    lda #$00
    sta [cbp],z

    inz
    bne @clear_screen_byte

    inc cbp+1
    bne @clear_screen_next
    inc cbp+2
    bne @clear_screen_next
    inc cbp+3

@clear_screen_next:
    inx
    bne @clear_screen_page

    rts
	
_returntobasic:

@clrcolor:
    lda #$00
    sta cbp
    lda #$00
    sta cbp+1
    lda #$f8
    sta cbp+2
    lda #$0f
    sta cbp+3

    ; 8 pagine = 2048 byte, quindi copre tutte le 1000 celle
    ldx #$08

@clear_color_page:
    ldz #$00

@clear_color_byte:
    lda #$00          ; byte basso Color RAM
    sta [cbp],z

    inz

    lda #$00          ; byte alto Color RAM
    sta [cbp],z

    inz
    bne @clear_color_byte

    inc cbp+1
    bne @clear_color_next
    inc cbp+2
    bne @clear_color_next
    inc cbp+3

@clear_color_next:
    dex
    bne @clear_color_page

   lda screenptr
   sta $d060
   lda screenptr+1
   sta $d061
   lda screenptr+2
   sta $d062
   lda #<80
   sta $d058
   lda #>80
   sta $d059
   lda #40
   sta $d05e
   lda #$80
   tsb $d031
   lda #$20
   tsb $d031
   lda #$08
   trb $d031
   lda #$01
   trb $d054
   lda #$04
   trb $d054
   
   jsr clrchn
   jsr ioinit
   jsr cint
   
   cli

   rts
   
.data

screenptr:
   .byte $00,$00,$00

	
	