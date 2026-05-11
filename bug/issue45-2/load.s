.cpu _45gs02

.global _loadobj
.global _initpxyattic
.global _initangles
.global frac1000q88

.global nvl
.global nvh
.global ax
.global ay
.global az
.global axv
.global ayv
.global azv
.global s88
.global camx0
.global camx1
.global camy0
.global camy1
.global camz0
.global camz1
.global fl880
.global fl881
.global fl882
.global dxw
.global dyw


.code

ptr     = $20
vbufp   = $22
vbufph  = $23
tbufp   = $24
tbufph  = $25
cbufp   = $26
cbufph  = $27

open = $ffc0
close = $ffc3
ckout = $ffc9
bsout = $ffd2
setbnk = $ff6b
setlfs = $ffba
setnam = $ffbd
chkin = $ffc6
readss = $ffb7
basin = $ffcf
load = $ffd5
clrchn = $ffcc
key = $ff9f
getin = $ffe4

_loadobj:

   lda #$00
   sta fs
   sta nvl
   sta nvh
   sta tl
   sta th
   sta vcnt
   sta tcnt
   sta ccnt
   sta skf
   sta objfacesstarted

   lda #<vbuf
   sta vbufp
   lda #>vbuf
   sta vbufph
   lda #0
   sta vcnt

   lda nvl
   sta il
   lda nvh
   sta ih
   jsr mul6itooff

   lda #$80
   sta v20
   lda #$00
   sta vab
   lda offl
   sta val
   lda offh
   sta vah

   lda #<tbuf
   sta tbufp
   lda #>tbuf
   sta tbufph
   lda #0
   sta tcnt

   lda tl
   sta il
   lda th
   sta ih
   jsr mul6itooff

   lda #$81
   sta t20
   lda #$00
   sta taba
   lda offl
   sta tal
   lda offh
   sta tah

   lda nvl
   sta basev0
   lda nvh
   sta basev1

   lda #<cbuf
   sta cbufp
   lda #>cbuf
   sta cbufph
   lda #0
   sta ccnt

   lda nvl
   sta ia0
   lda nvh
   sta ia1
   jsr mul3idxtooff

   lda #$84
   sta c20
   lda #$00
   sta cab
   lda offl
   sta cal
   lda offh
   sta cah
   
   lda #3 : sta $d020

   jsr openobj

@mainloop:
   jsr readobjlinekind
   lda fs
   bne @done

   lda linekind
   cmp #1
   beq @vertex
   cmp #2
   beq @face
   jmp @mainloop

@vertex:
   jsr readobjvertex
   jmp @mainloop

@face:
   lda objfacesstarted
   bne @anon_120
   jsr flushvbuftoattic
   jsr flushcbuftoattic
   lda #1
   sta objfacesstarted
@anon_120:
   jsr readobjface
   jmp @mainloop

@done:
   jsr flushvbuftoattic
   jsr flushcbuftoattic
   jsr flushtbuftoattic
   jsr closeobj
   rts

mul6itooff:
   ldax il; AX = word da il/ih
   mul.16 .ax, #6; AX = AX * 6
   stax offl; offl/offh = AX
   rts

mul3idxtooff:
   ldax ia0; AX = word da ia0/ia1
   mul.16 .ax, #3; AX = AX * 3
   stax offl; offl/offh = AX
   rts

openobj:
   sei

   ; ------------------------------------------------------------
   ; SETNAM
   ; ------------------------------------------------------------
   lda #objname_end - objname   ; lunghezza nome
   ldx #<objname
   ldy #>objname
   jsr setnam

   ; ------------------------------------------------------------
   ; SETLFS
   ; A = file number
   ; X = device (8 = disk drive)
   ; Y = secondary address (2 = read SEQ)
   ; ------------------------------------------------------------
   lda #$01
   ldx #$08
   ldy #$02
   jsr setlfs

   ; ------------------------------------------------------------
   ; SETBNK (MEGA65)
   ; A = 0 → bank default
   ; X = 0 → normale
   ; ------------------------------------------------------------
   lda #$00
   ldx #$00
   jsr setbnk

   ; ------------------------------------------------------------
   ; OPEN
   ; ------------------------------------------------------------
   jsr open
   bcs @err_open

   ; ------------------------------------------------------------
   ; CHKIN (usa file 1 come input)
   ; ------------------------------------------------------------
   ldx #$01
   jsr chkin
   bcs @err_chkin

   lda #$00
   sta fs

   cli
   rts

; ------------------------------------------------------------
; ERROR HANDLING
; ------------------------------------------------------------

@err_chkin:
   lda #$01
   jsr close

@err_open:
   lda #$ff
   sta fs
   jsr clrchn
   cli
   rts

readobjlinekind:

@loop:
   jsr readchar
   lda fs
   beq @anon_188
   jmp @out
@anon_188:

   lda char
   and #$7f
   sta char

   cmp #$0d
   beq @loop
   cmp #$0a
   beq @loop
   cmp #$20
   beq @loop
   cmp #$09
   beq @loop

   cmp #$23
   beq @skipline

   cmp #$76
   beq @checkv
   cmp #$56
   beq @checkv

   cmp #$66
   beq @checkf
   cmp #$46
   beq @checkf

   jsr skipline
   lda #0
   sta linekind
   rts

@checkv:
   jsr readchar
   lda fs
   bne @out

   lda char
   and #$7f
   sta char

   cmp #$20
   beq @isv
   cmp #$09
   beq @isv

; vn / vt / altro
   jsr skipline
   lda #0
   sta linekind
   rts

@isv:
   lda #1
   sta linekind
   rts

@checkf:
   jsr readchar
   lda fs
   bne @out

   lda char
   and #$7f
   sta char

   cmp #$20
   beq @isf
   cmp #$09
   beq @isf

   jsr skipline
   lda #0
   sta linekind
   rts

@isf:
   lda #2
   sta linekind
   rts

@skipline:
   jsr skipline
   lda #0
   sta linekind
   rts

@out:
   lda #0
   sta linekind
   rts

readchar:
  jsr basin
  sta char
  jsr readss
  and #%11000011
  sta fs
  rts

skipline:

@lp:
 jsr readchar
 lda fs
 bne @out
 lda char
 cmp #$0d
 beq @out
 cmp #$0a
 beq @out
 jmp @lp

@out:
 rts

readobjvertex:
jsr readfixq88
lda qv0
sta vx0
lda qv1

jsr readfixq88
lda qv0
sta vy0
lda qv1
sta vy1

jsr readfixq88
lda qv0
sta vz0
lda qv1
sta vz1

jsr storevertexi

; default = bianco
lda #$ff
sta vr
sta vg
sta vb

; prova a leggere R
jsr readoptionalfixq88
lda optpresent
beq @storec
jsr normfrac3
jsr objfrac_to_u8
sta vr

; prova a leggere G
jsr readoptionalfixq88
lda optpresent
beq @storec
jsr normfrac3
jsr objfrac_to_u8
sta vg

; prova a leggere B
jsr readoptionalfixq88
lda optpresent
beq @storec
jsr normfrac3
jsr objfrac_to_u8
sta vb

@storec:
jsr storecolori

inc nvl
bne @anon_362
inc nvh
@anon_362:
rts

readfixq88:
   lda #0
   sta qv0
   sta qv1
   sta ival0
   sta ival1
   sta fval0
   sta fval1
   sta fdig
   sta sgn2

   jsr seeknum
   tax
   lda fs
   beq @anon_381
   jmp @out
@anon_381:

   txa
   cmp #$2d
   bne @firstok
   lda #1
   sta sgn2
   jsr readchar
   lda fs
   beq @anon_392
   jmp @build
@anon_392:
   lda char

@firstok:
; -------------------------
; parte intera
; -------------------------

@intloop:
   jsr isdigit
   bne @afterint

   pha
   jsr mul1016ival
   pla
   sec
   sbc #$30  
   clc
   adc ival0
   sta ival0
   bcc @anon_414
   inc ival1
@anon_414:
   jsr readchar
   lda fs
   beq @anon_419
   jmp @build
@anon_419:
   lda char
   jmp @intloop

@afterint:
; -------------------------
; parte decimale: fino a 3 cifre
; -------------------------
   cmp #$2e
   bne @chkexp

   jsr readchar
   lda fs
   beq @anon_434
   jmp @build
@anon_434:
   lda char

@fracloop:
   jsr isdigit
   bne @chkexp

   lda fdig
   cmp #3
   bcs @skipfracdigit

   lda char
   pha
   jsr mul1016fval
   pla
   sec
   sbc #$30
   clc
   adc fval0
   sta fval0
   bcc @anon_456
   inc fval1
@anon_456:
   inc fdig

@skipfracdigit:
   jsr readchar
   lda fs
   bne @build
   lda char
   jmp @fracloop

@chkexp:
; -------------------------
; notazione scientifica minima
; se trova e-... => valore piccolissimo -> 0
; -------------------------
   cmp #$65
   beq @exp
   cmp #$45
   bne @build

@exp:
   jsr readchar
   lda fs
   bne @build
   lda char

   cmp #$2d
   bne @skipexp


   lda #0
   sta ival0
   sta ival1
   sta fval0
   sta fval1
   sta fdig
   jmp @skipexp

@skipexp:


@exp_loop:
   jsr readchar
   lda fs
   bne @build
   lda char
   jsr isws
   beq @build
   jmp @exp_loop

@build:
; -------------------------
; clamp parte intera e costruzione Q8.8
; -------------------------

; positivo
   lda sgn2
   bne @negbuild

; clamp > 127.xxx
   lda ival1
   bne @posclamp
   lda ival0
   cmp #128
   bcc @posok

@posclamp:
   lda #$ff
   sta qv0
   lda #$7f
   sta qv1
   jmp @out

@posok:
   lda ival0
   sta qv1

   lda fdig
   beq @out

   jsr normfrac3
   jsr frac3toq88
   lda q0
   sta qv0
   jmp @out

@negbuild:
; clamp modulo >= 128.xxx -> -128.0
   lda ival1
   bne @negclamp
   lda ival0
   cmp #128
   bcc @negok
   bne @negclamp
   lda fdig
   beq @negok
   lda fval0
   ora fval1
   bne @negclamp

@negok:
; costruisce valore positivo in tmp1:tmp0
   lda ival0
   sta tmp1
   lda #0
   sta tmp0

   lda fdig
   beq @maketwos
   jsr normfrac3
   jsr frac3toq88
   lda q0
   sta tmp0

@maketwos:
; two's complement 16 bit
   lda tmp0
   eor #$ff
   sta qv0
   lda tmp1
   eor #$ff
   sta qv1
   clc
   lda qv0
   adc #1
   sta qv0
   lda qv1
   adc #0
   sta qv1
   jmp @out

@negclamp:
   lda #$00
   sta qv0
   lda #$80
   sta qv1

@out:
   rts

seeknum:
  lda skf
  bne @normal

@seekfirst:
  jsr readchar
  lda fs
  and #%11000011
  bne @out
  lda char
  jsr isdigorminus
  beq @got
  jmp @seekfirst

@got:
  tax
  lda #1
  sta skf
  txa
  rts

@normal:

@skipws:
 jsr readchar
 lda fs
 and #%11000011
 bne @out

 lda char
 cmp #$2d
 beq @found

 jsr isdigit
 bne @skipws

@found:
 lda char

@out:
 rts

isdigorminus:
 cmp #$2d      ; '-'
 beq @rts
 cmp #$30      ; '0'
 beq @rts
 cmp #$31      ; '1'
 beq @rts
 cmp #$32      ; '2'
 beq @rts
 cmp #$33      ; '3'
 beq @rts
 cmp #$34      ; '4'
 beq @rts
 cmp #$35      ; '5'
 beq @rts
 cmp #$36      ; '6'
 beq @rts
 cmp #$37      ; '7'
 beq @rts
 cmp #$38      ; '8'
 beq @rts
 cmp #$39      ; '9'
 beq @rts

@rts:
 rts


isdigit:
 cmp #$30      ; '0'
 beq @rts
 cmp #$31      ; '1'
 beq @rts
 cmp #$32      ; '2'
 beq @rts
 cmp #$33      ; '3'
 beq @rts
 cmp #$34      ; '4'
 beq @rts
 cmp #$35      ; '5'
 beq @rts
 cmp #$36      ; '6'
 beq @rts
 cmp #$37      ; '7'
 beq @rts
 cmp #$38      ; '8'
 beq @rts
 cmp #$39      ; '9'
 beq @rts

@rts:
 rts

mul1016ival:
   ldax ival0
   mul.16 .ax, #$000a
   stax ival0
   rts

mul1016fval:
   ldax fval0
   mul.16 .ax, #10
   stax fval0
   rts

normfrac3:
  lda fdig
  cmp #3
  beq @done

  ldax fval0

  cmp #2
  beq @mul10

  mul.16 .ax, #100
  stax fval0
  rts

@mul10:
  mul.16 .ax, #10
  stax fval0

@done:
  rts

frac3toq88:
lda #<frac1000q88
sta ptr
lda #>frac1000q88
sta ptr+1
 
 lda ptr
 sta dump_ptr0
 lda ptr+1
 sta dump_ptr1

 clc
 lda ptr
 adc fval0
 sta ptr
 lda ptr+1
 adc fval1
 sta ptr+1

 ldy #0
 lda (ptr),y
 sta q0
 rts

storevertexi:
ldy #0
   lda vx0
   sta (vbufp),y
   iny
   lda vx1
   sta (vbufp),y
   iny
   lda vy0
   sta (vbufp),y
   iny
   lda vy1
   sta (vbufp),y
   iny
   lda vz0
   sta (vbufp),y
   iny
   lda vz1
   sta (vbufp),y
clc
lda vbufp
adc #6
sta vbufp
bcc @anon_757
inc vbufph
@anon_757:
inc vcnt
lda vcnt
cmp #255
bne @maybeflush
jsr flushvbuftoattic

@maybeflush:
rts

flushvbuftoattic:
lda vcnt
beq @rts

lda vcnt
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

; ptr = &dmavlist
lda #<dmavlist
sta ptr
lda #>dmavlist
sta ptr+1

; +4/+5 = count
ldy #4
lda offl
sta (ptr),y
iny
lda offh
sta (ptr),y

; +9/+10 = dst addr
ldy #9
lda val
sta (ptr),y
iny
lda vah
sta (ptr),y

; +11 = dst bank
iny
lda vab
sta (ptr),y

; +1 = dst20
ldy #1
lda v20
sta (ptr),y

; +6/+7/+8 = src addr
ldy #6
lda #<vbuf
sta (ptr),y
iny
lda #>vbuf
sta (ptr),y
iny
lda #$00
sta (ptr),y

jmp dmavjob

@rts:
rts

dmavjob:
  lda #$00
  sta $d707

dmavlist:
  .byte $81

  ; +1 dst20
  .byte $00
  .byte $00
  .byte $00

  ; +4 count
  .word 0

  ; +6 src addr
  .byte $00
  .byte $00
  .byte $00

  ; +9 dst addr
  .word 0

  ; +11 dst bank
  .byte $00
  .word $0000

afterdmav:
clc
   lda val
   adc offl
   sta val
   lda vah
   adc offh
   sta vah
   lda vab
   adc #0
   sta vab
cmp #$10
bne @no20
lda #$00
sta vab
inc v20

@no20:
   lda #<vbuf
   sta vbufp
   lda #>vbuf
   sta vbufph
   lda #0
   sta vcnt
rts

readoptionalfixq88:
   lda #0
   sta optpresent

@skipws:
   jsr readchar
   lda fs
   bne @out

   lda char
   cmp #$20
   beq @skipws
   cmp #$09
   beq @skipws

   cmp #$0d
   beq @out
   cmp #$0a
   beq @out

   lda char
   jsr isdigorminus
   bne @out

   lda #1
   sta skf
   lda #1
   sta optpresent
   jsr parsefixfromcurrent
   rts

@out:
   rts

parsefixfromcurrent:
   lda #0
   sta qv0
   sta qv1
   sta ival0
   sta ival1
   sta fval0
   sta fval1
   sta fdig
   sta sgn2

   lda char
   cmp #$2d
   bne @firstok
   lda #1
   sta sgn2
   jsr readchar
   lda fs
   beq @anon_907
   jmp @build
@anon_907:
   lda char

@firstok:

@intloop:
   jsr isdigit
   bne @afterint

   pha
   jsr mul1016ival
   pla
   sec
   sbc #$30
   clc
   adc ival0
   sta ival0
   bcc @anon_926
   inc ival1
@anon_926:
   jsr readchar
   lda fs
   beq @anon_931
   jmp @build
@anon_931:
   lda char
   jmp @intloop

@afterint:
   cmp #$2e
   bne @chkexp

   jsr readchar
   lda fs
   beq @anon_943
   jmp @build
@anon_943:
   lda char

@fracloop:
   jsr isdigit
   bne @chkexp

   lda fdig
   cmp #3
   bcs @skipfracdigit

   lda char
   pha
   jsr mul1016fval
   pla
   sec
   sbc #$30
   clc
   adc fval0
   sta fval0
   bcc @anon_965
   inc fval1
@anon_965:
   inc fdig

@skipfracdigit:
   jsr readchar
   lda fs
   bne @build
   lda char
   jmp @fracloop

@chkexp:
   cmp #$65 
   beq @exp
   cmp #$45
   bne @build

@exp:
   jsr readchar
   lda fs
   bne @build
   lda char

   cmp #$2d
   bne @skipexp

   lda #0
   sta ival0
   sta ival1
   sta fval0
   sta fval1
   sta fdig
   jmp @skipexp

@skipexp:

@exp_loop:
   jsr readchar
   lda fs
   bne @build
   lda char
   jsr isws
   beq @build
   jmp @exp_loop

@build:
   lda sgn2
   bne @negbuild

   lda ival1
   bne @posclamp
   lda ival0
   cmp #128
   bcc @posok

@posclamp:
   lda #$ff
   sta qv0
   lda #$7f
   sta qv1
   jmp @out

@posok:
   lda ival0
   sta qv1

   lda fdig
   beq @out

   jsr normfrac3
   jsr frac3toq88
   lda q0
   sta qv0
   jmp @out

@negbuild:
   lda ival1
   bne @negclamp
   lda ival0
   cmp #128
   bcc @negok
   bne @negclamp
   lda fdig
   beq @negok
   lda fval0
   ora fval1
   bne @negclamp

@negok:
   lda ival0
   sta tmp1
   lda #0
   sta tmp0

   lda fdig
   beq @maketwos
   jsr normfrac3
   jsr frac3toq88
   lda q0
   sta tmp0

@maketwos:
   lda tmp0
   eor #$ff
   sta qv0
   lda tmp1
   eor #$ff
   sta qv1
   clc
   lda qv0
   adc #1
   sta qv0
   lda qv1
   adc #0
   sta qv1
   jmp @out

@negclamp:
   lda #$00
   sta qv0
   lda #$80
   sta qv1

@out:
   rts

objfrac_to_u8:
; se parte intera != 0 -> clamp a 255
   lda ival1
   bne @ff
   lda ival0
   beq @frac

@ff:
   lda #$ff
   rts

@frac:
; P = fval * 256  (24 bit in p2:p1:p0)
   lda #$00
   sta p0
   lda fval0
   sta p1
   lda fval1
   sta p2

   lda #$00
   sta q0

@loop:
; se p2 != 0 allora P >= 65536 > 1000, quindi sottrai
   lda p2
   bne @sub

; confronto p1:p0 con 1000 ($03E8)
   lda p1
   cmp #$03
   bcc @done
   bne @sub
   lda p0
   cmp #$E8
   bcc @done

@sub:
   sec
   lda p0
   sbc #$E8
   sta p0
   lda p1
   sbc #$03
   sta p1
   lda p2
   sbc #$00
   sta p2

   inc q0
   bne @loop

@done:
   lda q0
   rts

storecolori:
  ldy #0
  lda vr
  sta (cbufp),y
  iny
  lda vg
  sta (cbufp),y
  iny
  lda vb
  sta (cbufp),y

  clc
  lda cbufp
  adc #3
  sta cbufp
  bcc @anon_1163
  inc cbufph
@anon_1163:
  inc ccnt
  lda ccnt
  cmp #255
  bne @rts
  jsr flushcbuftoattic

@rts:
  rts

flushcbuftoattic:
  lda ccnt
  beq @rts

  lda ccnt
  sta offl
  lda #0
  sta offh

; off = count * 3
  lda offl
  sta t0
  lda offh
  sta t1

  asl offl
  rol offh

  clc
  lda offl
  adc t0
  sta offl
  lda offh
  adc t1
  sta offh

  ; ptr = &dmaclist
  lda #<dmaclist
  sta ptr
  lda #>dmaclist
  sta ptr+1

  ; +4/+5 = count
  ldy #4
  lda offl
  sta (ptr),y
  iny
  lda offh
  sta (ptr),y

  ; +9/+10 = dst addr
  ldy #9
  lda cal
  sta (ptr),y
  iny
  lda cah
  sta (ptr),y

  ; +11 = dst bank
  iny
  lda cab
  sta (ptr),y

  ; +1 = dst20
  ldy #1
  lda c20
  sta (ptr),y

  ; +6/+7/+8 = src addr
  ldy #6
  lda #<cbuf
  sta (ptr),y
  iny
  lda #>cbuf
  sta (ptr),y
  iny
  lda #$00
  sta (ptr),y
  
  jmp dmacjob

@rts:
  rts

dmacjob:
lda #$00
sta $d707

dmaclist:
.byte $81

; +1 dst20
.byte $00
.byte $00
.byte $00

; +4 count
.word 0

; +6 src addr
.byte $00
.byte $00
.byte $00

; +9 dst addr
.word 0

; +11 dst bank
.byte $00
.word $0000

afterdmac:
  clc
  lda cal
  adc offl
  sta cal
  lda cah
  adc offh
  sta cah
  lda cab
  adc #0
  sta cab
  cmp #$10
  bne @no20
  lda #$00
  sta cab
  inc c20

@no20:
  lda #<cbuf
  sta cbufp
  lda #>cbuf
  sta cbufph
  lda #0
  sta ccnt
  rts

q88color_to_u8:
  lda qv1
  beq @anon_1269
  lda #$ff
  rts
@anon_1269:
  lda qv0
  rts

readobjface:
; primo indice -> ia
 jsr readobjfaceindex
 lda fs
 bne @done
 lda objtok
 beq @done

 jsr objindex_to_tmp
 lda tmp0
 sta ia0
 lda tmp1
 sta ia1

; secondo indice -> ib
 jsr readobjfaceindex
 lda fs
 bne @done
 lda objtok
 beq @done

 jsr objindex_to_tmp
 lda tmp0
 sta ib0
 lda tmp1
 sta ib1

@fanloop:
; prossimo indice -> ic
 jsr readobjfaceindex

 lda fs
 bne @done

 lda objtok
 beq @done

 jsr objindex_to_tmp
 lda tmp0
 sta ic0
 lda tmp1
 sta ic1

 jsr storetrit

 lda ic0
 sta ib0
 lda ic1
 sta ib1

 lda char
 cmp #$0d
 beq @done
 cmp #$0a
 beq @done

 jmp @fanloop

@done:
 rts

objindex_to_tmp:
; OBJ index 1-based -> interno 0-based + basev

 sec
 lda v0
 sbc #1
 sta tmp0
 lda v1
 sbc #0
 sta tmp1

 clc
 lda tmp0
 adc basev0
 sta tmp0
 lda tmp1
 adc basev1
 sta tmp1

 rts

readobjfaceindex:
lda #0
sta v0
sta v1
sta objtok

@seek:
jsr readchar
lda fs
bne @out

lda char
cmp #$20
beq @seek
cmp #$09
beq @seek

cmp #$0d
beq @out
cmp #$0a
beq @out

; deve partire con cifra
jsr isdigit
bne @out

@digits:
lda #1
sta objtok

lda char
pha
jsr mul1016v
pla
sec
sbc #$30
clc
adc v0
sta v0
bcc @anon_1396
inc v1
@anon_1396:
jsr readchar
lda fs
bne @out

lda char
jsr isdigit
beq @digits

; se è slash, salto il resto del token fino al separatore finale
lda char
cmp #$2f
bne @out

@skiprest:
jsr readchar
lda fs
bne @out
lda char
cmp #$20
beq @out
cmp #$09
beq @out
cmp #$0d
beq @out
cmp #$0a
beq @out
jmp @skiprest

@out:
rts

mul1016v:
   ldax v0; AX = v1:v0
   mul.16 .ax, #10; AX = AX * 10
   stax v0; v1:v0 = AX
   rts

storetrit:
  ldy #0
   lda ia0
   sta (tbufp),y
   iny
   lda ia1
   sta (tbufp),y
   iny
   lda ib0
   sta (tbufp),y
   iny
   lda ib1
   sta (tbufp),y
   iny
   lda ic0
   sta (tbufp),y
   iny
   lda ic1
   sta (tbufp),y

  clc
  lda tbufp
  adc #6
  sta tbufp
  bcc @anon_1449
  inc tbufph
@anon_1449:
  inc tcnt

  lda tcnt
  cmp #255
  bne @cont
  jsr flushtbuftoattic

@cont:
  inc tl
  bne @anon_1461
  inc th
@anon_1461:
  rts

flushtbuftoattic:
 lda tcnt
 beq @rts

 lda tcnt
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

 ; ptr = &dmatlist
 lda #<dmatlist
 sta ptr
 lda #>dmatlist
 sta ptr+1

 ; +4/+5 = count
 ldy #4
 lda offl
 sta (ptr),y
 iny
 lda offh
 sta (ptr),y

 ; +9/+10 = dst addr
 ldy #9
 lda tal
 sta (ptr),y
 iny
 lda tah
 sta (ptr),y

 ; +11 = dst bank
 iny
 lda taba
 sta (ptr),y

 ; +1 = dst20
 ldy #1
 lda t20
 sta (ptr),y

 ; +6/+7/+8 = src addr
 ldy #6
 lda #<tbuf
 sta (ptr),y
 iny
 lda #>tbuf
 sta (ptr),y
 iny
 lda #$00
 sta (ptr),y
 
 jmp dmatjob

@rts:
 rts

 dmatjob:
   lda #$00
   sta $d707

 dmatlist:
   .byte $81

   ; +1 dst20
   .byte $00
   .byte $00
   .byte $00

   ; +4 count
   .word 0

   ; +6 src addr
   .byte $00
   .byte $00
   .byte $00

   ; +9 dst addr
   .word 0

   ; +11 dst bank
   .byte $00
   .word $0000

afterdmat:
 clc
   lda tal
   adc offl
   sta tal
   lda tah
   adc offh
   sta tah
   lda taba
   adc #0
   sta taba
 cmp #$10
 bne @no20
 lda #$00
 sta taba
 inc t20

@no20:
   lda #<tbuf
   sta tbufp
   lda #>tbuf
   sta tbufph
   lda #0
   sta tcnt
 rts

isws:
	cmp #$20
	beq @rts
	cmp #$0d
	beq @rts
	cmp #$0a
	beq @rts
	cmp #$09
	beq @rts

@rts:
	rts

closeobj:
	jsr clrchn
	lda #$01
	jsr close
	rts
	
objname:
    .text "c,s"
objname_end:
	
.data

frac1000q88:
   .byte 0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4
   .byte 4,4,5,5,5,5,6,6,6,6,7,7,7,7,8,8
   .byte 8,8,9,9,9,9,10,10,10,10,11,11,11,12,12,12
   .byte 12,13,13,13,13,14,14,14,14,15,15,15,15,16,16,16
   .byte 16,17,17,17,17,18,18,18,18,19,19,19,19,20,20,20
   .byte 20,21,21,21,22,22,22,22,23,23,23,23,24,24,24,24
   .byte 25,25,25,25,26,26,26,26,27,27,27,27,28,28,28,28
   .byte 29,29,29,29,30,30,30,30,31,31,31,31,32,32,32,33
   .byte 33,33,33,34,34,34,34,35,35,35,35,36,36,36,36,37
   .byte 37,37,37,38,38,38,38,39,39,39,39,40,40,40,40,41
   .byte 41,41,41,42,42,42,42,43,43,43,44,44,44,44,45,45
   .byte 45,45,46,46,46,46,47,47,47,47,48,48,48,48,49,49
   .byte 49,49,50,50,50,50,51,51,51,51,52,52,52,52,53,53
   .byte 53,54,54,54,54,55,55,55,55,56,56,56,56,57,57,57
   .byte 57,58,58,58,58,59,59,59,59,60,60,60,60,61,61,61
   .byte 61,62,62,62,62,63,63,63,63,64,64,64,65,65,65,65
   .byte 66,66,66,66,67,67,67,67,68,68,68,68,69,69,69,69
   .byte 70,70,70,70,71,71,71,71,72,72,72,72,73,73,73,73
   .byte 74,74,74,74,75,75,75,76,76,76,76,77,77,77,77,78
   .byte 78,78,78,79,79,79,79,80,80,80,80,81,81,81,81,82
   .byte 82,82,82,83,83,83,83,84,84,84,84,85,85,85,86,86
   .byte 86,86,87,87,87,87,88,88,88,88,89,89,89,89,90,90
   .byte 90,90,91,91,91,91,92,92,92,92,93,93,93,93,94,94
   .byte 94,94,95,95,95,95,96,96,96,97,97,97,97,98,98,98
   .byte 98,99,99,99,99,100,100,100,100,101,101,101,101,102,102,102
   .byte 102,103,103,103,103,104,104,104,104,105,105,105,105,106,106,106
   .byte 106,107,107,107,108,108,108,108,109,109,109,109,110,110,110,110
   .byte 111,111,111,111,112,112,112,112,113,113,113,113,114,114,114,114
   .byte 115,115,115,115,116,116,116,116,117,117,117,118,118,118,118,119
   .byte 119,119,119,120,120,120,120,121,121,121,121,122,122,122,122,123
   .byte 123,123,123,124,124,124,124,125,125,125,125,126,126,126,126,127
   .byte 127,127,127,128,128,128,129,129,129,129,130,130,130,130,131,131
   .byte 131,131,132,132,132,132,133,133,133,133,134,134,134,134,135,135
   .byte 135,135,136,136,136,136,137,137,137,137,138,138,138,138,139,139
   .byte 139,140,140,140,140,141,141,141,141,142,142,142,142,143,143,143
   .byte 143,144,144,144,144,145,145,145,145,146,146,146,146,147,147,147
   .byte 147,148,148,148,148,149,149,149,150,150,150,150,151,151,151,151
   .byte 152,152,152,152,153,153,153,153,154,154,154,154,155,155,155,155
   .byte 156,156,156,156,157,157,157,157,158,158,158,158,159,159,159,159
   .byte 160,160,160,161,161,161,161,162,162,162,162,163,163,163,163,164
   .byte 164,164,164,165,165,165,165,166,166,166,166,167,167,167,167,168
   .byte 168,168,168,169,169,169,169,170,170,170,170,171,171,171,172,172
   .byte 172,172,173,173,173,173,174,174,174,174,175,175,175,175,176,176
   .byte 176,176,177,177,177,177,178,178,178,178,179,179,179,179,180,180
   .byte 180,180,181,181,181,182,182,182,182,183,183,183,183,184,184,184
   .byte 184,185,185,185,185,186,186,186,186,187,187,187,187,188,188,188
   .byte 188,189,189,189,189,190,190,190,190,191,191,191,191,192,192,192
   .byte 193,193,193,193,194,194,194,194,195,195,195,195,196,196,196,196
   .byte 197,197,197,197,198,198,198,198,199,199,199,199,200,200,200,200
   .byte 201,201,201,201,202,202,202,202,203,203,203,204,204,204,204,205
   .byte 205,205,205,206,206,206,206,207,207,207,207,208,208,208,208,209
   .byte 209,209,209,210,210,210,210,211,211,211,211,212,212,212,212,213
   .byte 213,213,214,214,214,214,215,215,215,215,216,216,216,216,217,217
   .byte 217,217,218,218,218,218,219,219,219,219,220,220,220,220,221,221
   .byte 221,221,222,222,222,222,223,223,223,223,224,224,224,225,225,225
   .byte 225,226,226,226,226,227,227,227,227,228,228,228,228,229,229,229
   .byte 229,230,230,230,230,231,231,231,231,232,232,232,232,233,233,233
   .byte 233,234,234,234,234,235,235,235,236,236,236,236,237,237,237,237
   .byte 238,238,238,238,239,239,239,239,240,240,240,240,241,241,241,241
   .byte 242,242,242,242,243,243,243,243,244,244,244,244,245,245,245,246
   .byte 246,246,246,247,247,247,247,248,248,248,248,249,249,249,249,250
   .byte 250,250,250,251,251,251,251,252,252,252,252,253,253,253,253,254
   .byte 254,254,254,255,255,255,255,255

; ------------------------------------------------------------
; Variables / buffers used by loadobj.s
; ------------------------------------------------------------

fs: .byte 0
skf: .byte 0
objfacesstarted: .byte 0
linekind: .byte 0
char: .byte 0
objsel: .byte 0
objtok: .byte 0
optpresent: .byte 0

ax: .byte 0
ay: .byte 0
az: .byte 0
axv: .byte 0
ayv: .byte 0
azv: .byte 0
s88: .byte 0
camx0: .byte 0
camx1: .byte 0
camy0: .byte 0
camy1: .byte 0
camz0: .byte 0
camz1: .byte 0

fl880: .byte 0
fl881: .byte 0
fl882: .byte 0

dxw: .byte 0
dyw: .byte 0

nvl: .byte 0
nvh: .byte 0
tl: .byte 0
th: .byte 0
il: .byte 0
ih: .byte 0
vcnt: .byte 0
tcnt: .byte 0
ccnt: .byte 0
dump_byte: .byte 0
dump_ptr0: .byte 0
dump_ptr1: .byte 0

basev0: .byte 0
basev1: .byte 0

offl: .byte 0
offh: .byte 0
t0: .byte 0
t1: .byte 0
tmp0: .byte 0
tmp1: .byte 0

v0: .byte 0
v1: .byte 0
ia0: .byte 0
ia1: .byte 0
ib0: .byte 0
ib1: .byte 0
ic0: .byte 0
ic1: .byte 0

ival0: .byte 0
ival1: .byte 0
fval0: .byte 0
fval1: .byte 0
fdig: .byte 0
sgn2: .byte 0
q0: .byte 0
qv0: .byte 0
qv1: .byte 0

vx0: .byte 0
vx1: .byte 0
vy0: .byte 0
vy1: .byte 0
vz0: .byte 0
vz1: .byte 0

vr: .byte 0
vg: .byte 0
vb: .byte 0

p0: .byte 0
p1: .byte 0
p2: .byte 0

v20: .byte 0
vab: .byte 0
val: .byte 0
vah: .byte 0

t20: .byte 0
taba: .byte 0
tal: .byte 0
tah: .byte 0

c20: .byte 0
cab: .byte 0
cal: .byte 0
cah: .byte 0

.bss

start_bss_buffers:
vbuf: .res 1530
tbuf: .res 1530
cbuf: .res 765