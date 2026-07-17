.cpu _45gs02

.code

.global _projectallvertices
.global il
.global ih
.global rbufp
.global pbufp
.global rbuf
.global pbuf
.global sin256
.global pvert_vars


_projectallvertices:
   jsr stepangles
   jsr calctrig
   jsr initrvxyzattic
   jsr initpxyattic
   lda #0
   sta il
   lda #0
   sta ih

@loop:
   lda ih
   cmp nvh
   bne @cont
   lda il
   cmp nvl
   beq @done

@cont:
   jsr loadvfromattic
   jsr copyvxyztoq88
   jsr scaleq88
   jsr rotatepointq88
   jsr addviewoffset
   jsr storervxyzi
   jsr projectpointq88
   jsr storepxyi
   inc il
   bne @loop
   inc ih
   jmp @loop

@done:
   jsr flushrbuftoattic
   jsr flushpbuftoattic
   rts

stepangles:
   clc
   lda ax
   adc axv
   sta ax
   lda ay
   adc ayv
   sta ay
   lda az
   adc azv
   sta az
   rts

calctrig:
   lda ax
   jsr getsin
   sta s1l
   stx s1h
   lda ax
   clc
   adc #64
   jsr getsin
   sta c1l
   stx c1h
   lda ay
   jsr getsin
   sta s2l
   stx s2h
   lda ay
   clc
   adc #64
   jsr getsin
   sta c2l
   stx c2h
   lda az
   jsr getsin
   sta s3l
   stx s3h
   lda az
   clc
   adc #64
   jsr getsin
   sta c3l
   stx c3h
   rts

getsin:
   tax
   txa
   asl
   tay
   lda sin256,y
   ldx sin256+1,y
   rts

initrvxyzattic:
   lda #<rbuf
   sta rbufp
   lda #>rbuf
   sta rbufph
   lda #0
   sta rcnt
   lda #$83
   sta r20
   lda #$00
   sta rab
   sta ral
   sta rah
   rts

initpxyattic:
   lda #<pbuf
   sta pbufp
   lda #>pbuf
   sta pbufph
   lda #0
   sta pcnt
   lda #$82
   sta p20
   lda #$00
   sta pab
   sta pal
   sta pah
   rts

loadvfromattic:
   jsr mul6itooff
   lda #$00
   sta lptr
   lda #$00
   sta lptr+1
   lda #$00
   sta lptr+2
   lda #$08
   sta lptr+3

   clc
   lda lptr
   adc offl
   sta lptr
   lda lptr+1
   adc offh
   sta lptr+1
   bcc @anon_131
   inc lptr+2
   bne @anon_131
   inc lptr+3
@anon_131:

   ldz #0
   lda [lptr],z
   sta vx0
   inz
   lda [lptr],z
   sta vx1
   inz
   lda [lptr],z
   sta vy0
   inz
   lda [lptr],z
   sta vy1
   inz
   lda [lptr],z
   sta vz0
   inz
   lda [lptr],z
   sta vz1
   rts

mul6itooff:
   lda il
   sta offl
   lda ih
   sta offh

; off = i * 2
   asl offl
   rol offh

   lda offl
   sta t0
   lda offh
   sta t1

; off = i * 4
   asl offl
   rol offh

; off = i*4 + i*2 = i*6
   clc
   lda offl
   adc t0
   sta offl
   lda offh
   adc t1
   sta offh
   rts

copyvxyztoq88:
   lda vx0
   sta x88
   lda vx1
   sta x88+1
   lda vy0
   sta y88
   lda vy1
   sta y88+1
   lda vz0
   sta z88
   lda vz1
   sta z88+1
   rts

scaleq88:
   lda x88
   sta mula0
   lda x88+1
   sta mula1
   lda s88
   sta mulb0
   lda s88+1
   sta mulb1
   jsr mulq88q88toq88
   lda tmp0
   sta x88
   lda tmp1
   sta x88+1
   lda y88
   sta mula0
   lda y88+1
   sta mula1
   lda s88
   sta mulb0
   lda s88+1
   sta mulb1
   jsr mulq88q88toq88
   lda tmp0
   sta y88
   lda tmp1
   sta y88+1
   lda z88
   sta mula0
   lda z88+1
   sta mula1
   lda s88
   sta mulb0
   lda s88+1
   sta mulb1
   jsr mulq88q88toq88
   lda tmp0
   sta z88
   lda tmp1
   sta z88+1
   rts

mulq88q88toq88:
   jsr muls16s1632
   ldx #8

@shr:
   lda p3
   and #$80
   beq @c0
   sec
   bcs @go

@c0:
   clc

@go:
   ror p3
   ror p2
   ror p1
   ror p0
   dex
   bne @shr
   lda p0
   sta tmp0
   lda p1
   sta tmp1
   rts

muls16s1632:
   lda mula1
   eor mulb1
   and #$80
   sta mulsign
   lda mula1
   bpl @aok
   lda mula0
   eor #$ff
   sta mula0
   lda mula1
   eor #$ff
   sta mula1
   clc
   lda mula0
   adc #1
   sta mula0
   lda mula1
   adc #0
   sta mula1

@aok:
   lda mulb1
   bpl @bok
   lda mulb0
   eor #$ff
   sta mulb0
   lda mulb1
   eor #$ff
   sta mulb1
   clc
   lda mulb0
   adc #1
   sta mulb0
   lda mulb1
   adc #0
   sta mulb1

@bok:
   lda #0
   sta p0
   sta p1
   sta p2
   sta p3
   lda mula0
   sta tmp0
   lda mula1
   sta tmp1
   lda #0
   sta r0
   sta r1
   ldx #16

@ml:
   lda mulb0
   and #1
   beq @noadd
   clc
   lda p0
   adc tmp0
   sta p0
   lda p1
   adc tmp1
   sta p1
   lda p2
   adc r0
   sta p2
   lda p3
   adc r1
   sta p3

@noadd:
   asl tmp0
   rol tmp1
   rol r0
   rol r1
   lsr mulb1
   ror mulb0
   dex
   bne @ml
   lda mulsign
   beq @out
   jsr neg32p

@out:
   rts

neg32p:
   lda p0
   eor #$ff
   sta p0
   lda p1
   eor #$ff
   sta p1
   lda p2
   eor #$ff
   sta p2
   lda p3
   eor #$ff
   sta p3
   clc
   lda p0
   adc #1
   sta p0
   lda p1
   adc #0
   sta p1
   lda p2
   adc #0
   sta p2
   lda p3
   adc #0
   sta p3
   rts

rotatepointq88:
   lda y88
   sta ty88
   lda y88+1
   sta ty88+1
   lda c1l
   sta mula0
   lda c1h
   sta mula1
   lda ty88
   sta mulb0
   lda ty88+1
   sta mulb1
   jsr mulq15q88toq88
   lda tmp0
   sta tx88
   lda tmp1
   sta tx88+1
   lda s1l
   sta mula0
   lda s1h
   sta mula1
   lda z88
   sta mulb0
   lda z88+1
   sta mulb1
   jsr mulq15q88toq88
   sec
   lda tx88
   sbc tmp0
   sta y88
   lda tx88+1
   sbc tmp1
   sta y88+1
   lda s1l
   sta mula0
   lda s1h
   sta mula1
   lda ty88
   sta mulb0
   lda ty88+1
   sta mulb1
   jsr mulq15q88toq88
   lda tmp0
   sta tx88
   lda tmp1
   sta tx88+1
   lda c1l
   sta mula0
   lda c1h
   sta mula1
   lda z88
   sta mulb0
   lda z88+1
   sta mulb1
   jsr mulq15q88toq88
   clc
   lda tx88
   adc tmp0
   sta z88
   lda tx88+1
   adc tmp1
   sta z88+1
   lda x88
   sta tx88
   lda x88+1
   sta tx88+1
   lda c2l
   sta mula0
   lda c2h
   sta mula1
   lda tx88
   sta mulb0
   lda tx88+1
   sta mulb1
   jsr mulq15q88toq88
   lda tmp0
   sta ty88
   lda tmp1
   sta ty88+1
   lda s2l
   sta mula0
   lda s2h
   sta mula1
   lda z88
   sta mulb0
   lda z88+1
   sta mulb1
   jsr mulq15q88toq88
   clc
   lda ty88
   adc tmp0
   sta x88
   lda ty88+1
   adc tmp1
   sta x88+1
   lda s2l
   sta mula0
   lda s2h
   sta mula1
   lda tx88
   sta mulb0
   lda tx88+1
   sta mulb1
   jsr mulq15q88toq88
   lda tmp0
   eor #$ff
   sta ty88
   lda tmp1
   eor #$ff
   sta ty88+1
   clc
   lda ty88
   adc #1
   sta ty88
   lda ty88+1
   adc #0
   sta ty88+1
   lda c2l
   sta mula0
   lda c2h
   sta mula1
   lda z88
   sta mulb0
   lda z88+1
   sta mulb1
   jsr mulq15q88toq88
   clc
   lda ty88
   adc tmp0
   sta z88
   lda ty88+1
   adc tmp1
   sta z88+1
   lda x88
   sta tx88
   lda x88+1
   sta tx88+1
   lda c3l
   sta mula0
   lda c3h
   sta mula1
   lda tx88
   sta mulb0
   lda tx88+1
   sta mulb1
   jsr mulq15q88toq88
   lda tmp0
   sta ty88
   lda tmp1
   sta ty88+1
   lda s3l
   sta mula0
   lda s3h
   sta mula1
   lda y88
   sta mulb0
   lda y88+1
   sta mulb1
   jsr mulq15q88toq88
   sec
   lda ty88
   sbc tmp0
   sta x88
   lda ty88+1
   sbc tmp1
   sta x88+1
   lda s3l
   sta mula0
   lda s3h
   sta mula1
   lda tx88
   sta mulb0
   lda tx88+1
   sta mulb1
   jsr mulq15q88toq88
   lda tmp0
   sta ty88
   lda tmp1
   sta ty88+1
   lda c3l
   sta mula0
   lda c3h
   sta mula1
   lda y88
   sta mulb0
   lda y88+1
   sta mulb1
   jsr mulq15q88toq88
   clc
   lda ty88
   adc tmp0
   sta y88
   lda ty88+1
   adc tmp1
   sta y88+1
   rts

mulq15q88toq88:
   jsr muls16s1632
   ldx #15

@shr:
   lda p3
   and #$80
   beq @c0
   sec
   bcs @go

@c0:
   clc

@go:
   ror p3
   ror p2
   ror p1
   ror p0
   dex
   bne @shr
   lda p0
   sta tmp0
   lda p1
   sta tmp1
   rts

addviewoffset:
   clc
   lda x88
   adc camx0
   sta x88
   lda x88+1
   adc camx1
   sta x88+1
   clc
   lda y88
   adc camy0
   sta y88
   lda y88+1
   adc camy1
   sta y88+1
   clc
   lda z88
   adc camz0
   sta z88
   lda z88+1
   adc camz1
   sta z88+1
   rts

storervxyzi:
   ldy #0
   lda x88
   sta (rbufp),y
   iny
   lda x88+1
   sta (rbufp),y
   iny
   lda y88
   sta (rbufp),y
   iny
   lda y88+1
   sta (rbufp),y
   iny
   lda z88
   sta (rbufp),y
   iny
   lda z88+1
   sta (rbufp),y
   clc
   lda rbufp
   adc #6
   sta rbufp
   bcc @anon_475
   inc rbufph
@anon_475:
   inc rcnt
   lda rcnt
   cmp #255
   bne @rts
   jsr flushrbuftoattic

@rts:
   rts

flushrbuftoattic:
   lda rcnt
   beq @rts

   lda rcnt
   sta offl
   lda #0
   sta offh

; off = count * 2
   asl offl
   rol offh
   lda offl
   sta t0
   lda offh
   sta t1

; off = count * 4
   asl offl
   rol offh

; off = count*4 + count*2 = count*6
   clc
   lda offl
   adc t0
   sta offl
   lda offh
   adc t1
   sta offh

   lda offl
   sta dmarcount
   lda offh
   sta dmarcount+1
   lda ral
   sta dmardstaddr
   lda rah
   sta dmardstaddr+1
   lda rab
   sta dmardstbank
   lda r20
   sta dmardst20
   lda #<rbuf
   sta dmarsrcaddr
   lda #>rbuf
   sta dmarsrcaddr+1
   lda #0
   sta dmarsrcbank
   jmp dmarjob

@rts:
   rts

dmarjob:
   lda #$00
   sta $d707
   .byte $81

dmardst20:
   .byte $00
   .byte $00
   .byte $00

dmarcount:
  .word 0

dmarsrcaddr:
  .word 0

dmarsrcbank:
  .byte $00

dmardstaddr:
   .word 0

dmardstbank:
   .byte $00
   .word $0000

afterdmar:
   clc
   lda ral
   adc offl
   sta ral
   lda rah
   adc offh
   sta rah
   lda rab
   adc #0
   sta rab
   cmp #$10
   bne @no20
   lda #$00
   sta rab
   inc r20

@no20:
   lda #<rbuf
   sta rbufp
   lda #>rbuf
   sta rbufph
   lda #0
   sta rcnt
   rts

projectpointq88:
   lda z88
   sta den0
   lda z88+1
   sta den1
   lda z88+1
   and #$80
   beq @zpos
   lda #$ff
   bne @zset

@zpos:
   lda #$00

@zset:
   sta den2
   clc
   lda den0
   adc fl880
   sta den0
   lda den1
   adc fl881
   sta den1
   lda den2
   adc fl882
   sta den2
   lda den2
   bmi @clamp
   bne @denok
   lda den1
   bne @denok
   lda den0
   bne @denok

@clamp:
   lda #1
   sta den0
   lda #0
   sta den1
   sta den2

@denok:
   lda x88
   sta mula0
   lda x88+1
   sta mula1
   lda fl880
   sta mulb0
   lda fl881
   sta mulb1
   lda fl882
   sta mulb2
   jsr muls16u2440
   lda #0
   sta mulsign
   lda n4
   bmi @negx
   jmp @divx

@negx:
   lda #1
   sta mulsign
   jsr neg40n

@divx:
   jsr udiv40by24q16
   lda mulsign
   beq @roundx
   lda q0
   eor #$ff
   sta q0
   lda q1
   eor #$ff
   sta q1
   clc
   lda q0
   adc #1
   sta q0
   lda q1
   adc #0
   sta q1

@roundx:
   clc
   lda q0
   adc #$80
   sta q0
   lda q1
   adc #0
   sta q1
   lda q1
   sta tmp0
   lda q1
   and #$80
   beq @sxpos
   lda #$ff
   sta tmp1
   jmp @addx

@sxpos:
   lda #$00
   sta tmp1

@addx:
   clc
   lda dxw
   adc tmp0
   sta ux0
   lda dxw+1
   adc tmp1
   sta ux1
   lda y88
   sta mula0
   lda y88+1
   sta mula1
   lda fl880
   sta mulb0
   lda fl881
   sta mulb1
   lda fl882
   sta mulb2
   jsr muls16u2440
   lda #0
   sta mulsign
   lda n4
   bmi @negy
   jmp @divy

@negy:
   lda #1
   sta mulsign
   jsr neg40n

@divy:
   jsr udiv40by24q16
   lda mulsign
   beq @roundy
   lda q0
   eor #$ff
   sta q0
   lda q1
   eor #$ff
   sta q1
   clc
   lda q0
   adc #1
   sta q0
   lda q1
   adc #0
   sta q1

@roundy:
   clc
   lda q0
   adc #$80
   sta q0
   lda q1
   adc #0
   sta q1
   lda q1
   sta tmp0
   lda q1
   and #$80
   beq @sypos
   lda #$ff
   sta tmp1
   jmp @addy

@sypos:
   lda #$00
   sta tmp1

@addy:
   sec
   lda dyw
   sbc tmp0
   sta uy0
   lda dyw+1
   sbc tmp1
   sta uy1
; ux <= 319
   lda ux1
   bmi @uxneg
   cmp #$01
   bcc @uxok
   bne @uxhi
   lda ux0
   cmp #$40
   bcc @uxok

@uxhi:
   lda #$3f
   sta ux0
   lda #$01
   sta ux1
   jmp @uxok

@uxneg:
   lda #$00
   sta ux0
   sta ux1

@uxok:
   lda uy1
   bmi @uyneg
   beq @anon_720
   jmp @uyhi
@anon_720:
   lda uy0
   cmp #$c8
   bcc @uyok

@uyhi:
   lda #$c7
   sta uy0
   lda #$00
   sta uy1
   jmp @uyok

@uyneg:
   lda #$00
   sta uy0
   sta uy1

@uyok:
   rts

muls16u2440:
   lda mula1
   and #$80
   sta mulsign
   lda mulsign
   beq @aok
   lda mula0
   eor #$ff
   sta mula0
   lda mula1
   eor #$ff
   sta mula1
   clc
   lda mula0
   adc #1
   sta mula0
   lda mula1
   adc #0
   sta mula1

@aok:
   lda #0
   sta n0
   sta n1
   sta n2
   sta n3
   sta n4
   lda mula0
   sta tmp0
   lda mula1
   sta tmp1
   lda #0
   sta r0
   sta r1
   sta r2
   ldx #24

@loop:
   lda mulb0
   and #1
   beq @noadd
   clc
   lda n0
   adc tmp0
   sta n0
   lda n1
   adc tmp1
   sta n1
   lda n2
   adc r0
   sta n2
   lda n3
   adc r1
   sta n3
   lda n4
   adc r2
   sta n4

@noadd:
   asl tmp0
   rol tmp1
   rol r0
   rol r1
   rol r2
   lsr mulb2
   ror mulb1
   ror mulb0
   dex
   bne @loop
   lda mulsign
   beq @out
   jsr neg40n

@out:
   rts

neg40n:
   lda n0
   eor #$ff
   sta n0
   lda n1
   eor #$ff
   sta n1
   lda n2
   eor #$ff
   sta n2
   lda n3
   eor #$ff
   sta n3
   lda n4
   eor #$ff
   sta n4
   clc
   lda n0
   adc #1
   sta n0
   lda n1
   adc #0
   sta n1
   lda n2
   adc #0
   sta n2
   lda n3
   adc #0
   sta n3
   lda n4
   adc #0
   sta n4
   rts

udiv40by24q16:
   lda #0
   sta q0
   sta q1
   lda #0
   sta r0
   sta r1
   sta r2
   ldx #24

@pre:
   asl n0
   rol n1
   rol n2
   rol n3
   rol n4
   rol r0
   rol r1
   rol r2
   lda r2
   cmp den2
   bcc @prenext
   bne @prege
   lda r1
   cmp den1
   bcc @prenext
   bne @prege
   lda r0
   cmp den0
   bcc @prenext

@prege:
   sec
   lda r0
   sbc den0
   sta r0
   lda r1
   sbc den1
   sta r1
   lda r2
   sbc den2
   sta r2

@prenext:
   dex
   bne @pre
   ldx #16

@loop:
   asl n0
   rol n1
   rol n2
   rol n3
   rol n4
   rol r0
   rol r1
   rol r2
   asl q0
   rol q1
   lda r2
   cmp den2
   bcc @next
   bne @ge
   lda r1
   cmp den1
   bcc @next
   bne @ge
   lda r0
   cmp den0
   bcc @next

@ge:
   sec
   lda r0
   sbc den0
   sta r0
   lda r1
   sbc den1
   sta r1
   lda r2
   sbc den2
   sta r2
   lda q0
   ora #1
   sta q0

@next:
   dex
   bne @loop
   rts

storepxyi:
   ldy #0
   lda ux0
   sta (pbufp),y
   iny
   lda ux1
   sta (pbufp),y
   iny
   lda uy0
   sta (pbufp),y
   iny
   lda uy1
   sta (pbufp),y
   clc
   lda pbufp
   adc #4
   sta pbufp
   bcc @anon_886
   inc pbufph
@anon_886:
   inc pcnt
   lda pcnt
   cmp #255
   bne @maybe
   jsr flushpbuftoattic

@maybe:
   rts

flushpbuftoattic:
   lda pcnt
   beq @rts

   lda pcnt
   sta offl
   lda #0
   sta offh

   asl offl
   rol offh
   asl offl
   rol offh

   lda offl
   sta dmapcount
   lda offh
   sta dmapcount+1
   lda pal
   sta dmapdstaddr
   lda pah
   sta dmapdstaddr+1
   lda pab
   sta dmapdstbank
   lda p20
   sta dmapdst20
   lda #<pbuf
   sta dmapsrcaddr
   lda #>pbuf
   sta dmapsrcaddr+1
   lda #0
   sta dmapsrcbank
   jmp dmapjob

@rts:
   rts

dmapjob:
   lda #$00
   sta $d707
   .byte $81

dmapdst20:
   .byte $00
   .byte $00
   .byte $00

dmapcount:
  .word 0

dmapsrcaddr:
  .word 0

dmapsrcbank:
  .byte $00

dmapdstaddr:
   .word 0

dmapdstbank:
   .byte $00
   .word $0000

afterdmap:
   clc
   lda pal
   adc offl
   sta pal
   lda pah
   adc offh
   sta pah
   lda pab
   adc #0
   sta pab
   cmp #$10
   bne @no20
   lda #$00
   sta pab
   inc p20

@no20:
   lda #<pbuf
   sta pbufp
   lda #>pbuf
   sta pbufph
   lda #0
   sta pcnt
   rts



.data

il: .byte 0
ih: .byte 0

offl: .byte 0
offh: .byte 0

t0: .byte 0
t1: .byte 0

tmp0: .byte 0
tmp1: .byte 0

; loaded vertex
vx0: .byte 0
vx1: .byte 0
vy0: .byte 0
vy1: .byte 0
vz0: .byte 0
vz1: .byte 0

; q8.8 working coords
x88: .byte 0,0
y88: .byte 0,0
z88: .byte 0,0
tx88: .byte 0,0
ty88: .byte 0,0

; trig cache
s1l: .byte 0
s1h: .byte 0
c1l: .byte 0
c1h: .byte 0

s2l: .byte 0
s2h: .byte 0
c2l: .byte 0
c2h: .byte 0

s3l: .byte 0
s3h: .byte 0
c3l: .byte 0
c3h: .byte 0

; multiply inputs
mula0: .byte 0
mula1: .byte 0

mulb0: .byte 0
mulb1: .byte 0
mulb2: .byte 0

mulsign: .byte 0

; 32-bit product
p0: .byte 0
p1: .byte 0
p2: .byte 0
p3: .byte 0

; temp registers
r0: .byte 0
r1: .byte 0
r2: .byte 0

; 40-bit numerator
n0: .byte 0
n1: .byte 0
n2: .byte 0
n3: .byte 0
n4: .byte 0

; division denominator
den0: .byte 0
den1: .byte 0
den2: .byte 0

; quotient
q0: .byte 0
q1: .byte 0

; projected coords
ux0: .byte 0
ux1: .byte 0

uy0: .byte 0
uy1: .byte 0

; rotated vertex buffer ptr
rbufp: .byte 0
rbufph: .byte 0

rcnt: .byte 0

r20: .byte 0
rab: .byte 0
ral: .byte 0
rah: .byte 0

; projected xy buffer ptr
pbufp: .byte 0
pbufph: .byte 0

pcnt: .byte 0

p20: .byte 0
pab: .byte 0
pal: .byte 0
pah: .byte 0

; 28-bit attic pointer
lptr: .byte 0,0,0,0

sin256:
   .word 0,804,1608,2410,3212,4011,4808,5602
   .word 6393,7180,7962,8739,9512,10278,11039,11793
   .word 12539,13279,14010,14732,15446,16150,16845,17530
   .word 18204,18868,19519,20159,20787,21403,22005,22594
   .word 23170,23731,24279,24811,25329,25832,26319,26790
   .word 27245,27683,28105,28510,28898,29269,29622,29957
   .word 30274,30572,30852,31113,31356,31579,31784,31968
   .word 32134,32279,32405,32510,32596,32661,32706,32730
   .word 32767,32730,32706,32661,32596,32510,32405,32279
   .word 32134,31968,31784,31579,31356,31113,30852,30572
   .word 30274,29957,29622,29269,28898,28510,28105,27683
   .word 27245,26790,26319,25832,25329,24811,24279,23731
   .word 23170,22594,22005,21403,20787,20159,19519,18868
   .word 18204,17530,16845,16150,15446,14732,14010,13279
   .word 12539,11793,11039,10278,9512,8739,7962,7180
   .word 6393,5602,4808,4011,3212,2410,1608,804
   .word 0,-804,-1608,-2410,-3212,-4011,-4808,-5602
   .word -6393,-7180,-7962,-8739,-9512,-10278,-11039,-11793
   .word -12539,-13279,-14010,-14732,-15446,-16150,-16845,-17530
   .word -18204,-18868,-19519,-20159,-20787,-21403,-22005,-22594
   .word -23170,-23731,-24279,-24811,-25329,-25832,-26319,-26790
   .word -27245,-27683,-28105,-28510,-28898,-29269,-29622,-29957
   .word -30274,-30572,-30852,-31113,-31356,-31579,-31784,-31968
   .word -32134,-32279,-32405,-32510,-32596,-32661,-32706,-32730
   .word -32767,-32730,-32706,-32661,-32596,-32510,-32405,-32279
   .word -32134,-31968,-31784,-31579,-31356,-31113,-30852,-30572
   .word -30274,-29957,-29622,-29269,-28898,-28510,-28105,-27683
   .word -27245,-26790,-26319,-25832,-25329,-24811,-24279,-23731
   .word -23170,-22594,-22005,-21403,-20787,-20159,-19519,-18868
   .word -18204,-17530,-16845,-16150,-15446,-14732,-14010,-13279
   .word -12539,-11793,-11039,-10278,-9512,-8739,-7962,-7180
   .word -6393,-5602,-4808,-4011,-3212,-2410,-1608,-804
   
.bss


rbuf: .res 1530
pbuf: .res 1020
