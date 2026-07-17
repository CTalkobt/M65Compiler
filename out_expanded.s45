    .org    $2000
    ldx     #$00
    .org    $2002
@__zp_save_loop:
    lda     $08,x
    .org    $2004
    sta     $2584,x
    .org    $2007
    inx     
    .org    $2008
    cpx     #$F8
    .org    $200A
    bne     *-8
    .org    $200C
    bsr     *+57
    .org    $200F
    sta     $02
    .org    $2011
    stx     $03
    .org    $2013
    ldx     #$00
    .org    $2015
@__zp_restore_loop:
    lda     $2584,x
    .org    $2018
    sta     $08,x
    .org    $201A
    inx     
    .org    $201B
    cpx     #$F8
    .org    $201D
    bne     *-8
    .org    $201F
    lda     $02
    .org    $2021
    ldx     $03
    .org    $2023
__halt:
    bra     *+0
    .org    $2025
_result:
    .word   $4000
    .org    $2027
_scores:
    .byte   $00
    .org    $2028
    .byte   $00
    .byte   $00
    .byte   $00
    .byte   $00
    .org    $202C
_grid:
    .word   $0000
    .org    $202E
    .byte   $00
    .byte   $00
    .byte   $00
    .byte   $00
    .byte   $00
    .byte   $00
    .byte   $00
    .byte   $00
    .byte   $00
    .byte   $00
    .byte   $00
    .byte   $00
    .byte   $00
    .byte   $00
    .byte   $00
    .byte   $00
    .byte   $00
    .byte   $00
    .byte   $00
    .byte   $00
    .byte   $00
    .byte   $00
    .org    $2044
_main:
    phw     #$0000
    .org    $2047
    phw     #$0000
    .org    $204A
    tsx     
    .org    $204B
    txa     
    .org    $204C
    clc     
    .org    $204D
    adc     #$01
    .org    $204F
    sta     $FD
    .org    $2051
    lda     #$01
    .org    $2053
    adc     #$00
    .org    $2055
    sta     $FE
    .org    $2057
_main:@entry:
    lda     #$00
    .org    $2059
    taz     
    .org    $205A
    tsx     
    sta     $0101,x
    tsx     
    stz     $0102,x
    .org    $2062
_main:@for_cond0:
    tsx     
    lda     $0102,x
    pha     
    lda     $0101,x
    plx     
    .org    $206B
    cpx     #$00
    cmp     #$05
    .org    $2071
    bcs     *+109
    .org    $2073
_main:@for_body1:
    tsx     
    lda     $0102,x
    pha     
    lda     $0101,x
    plx     
    .org    $207C
    clc     
    .org    $207D
    adc     #$01
    .org    $207F
    sta     $26
    .org    $2081
    stx     $27
    .org    $2083
    pha     
    .org    $2084
    tsx     
    lda     $0102,x
    pha     
    lda     $0101,x
    plx     
    .org    $208D
    sta     $0A
    .org    $208F
    stx     $0B
    .org    $2091
    lda     $0A
    ldx     $0B
    sta     $02
    stx     $03
    lda     #$27
    ldx     #$20
    clc     
    adc     $02
    pha     
    txa     
    adc     $03
    tax     
    pla     
    sta     $08
    txa     
    sta     $09
    .org    $20AF
    pla     
    .org    $20B0
    ldy     #$00
    .org    $20B2
    sta     ($08),y
    .org    $20B4
_main:@for_inc2:
    tsx     
    lda     $0102,x
    pha     
    lda     $0101,x
    plx     
    .org    $20BD
    sta     $2A
    .org    $20BF
    stx     $2B
    .org    $20C1
    clc     
    .org    $20C2
    adc     #$01
    .org    $20C4
    sta     $2C
    .org    $20C6
    lda     $2B
    .org    $20C8
    adc     #$00
    .org    $20CA
    sta     $2D
    .org    $20CC
    lda     $2C
    .org    $20CE
    ldx     $2D
    .org    $20D0
    pha     
    txa     
    taz     
    pla     
    tsx     
    sta     $0101,x
    tsx     
    stz     $0102,x
    .org    $20DC
    bra     *-122
    .org    $20DE
_main:@for_end3:
    lda     #$00
    .org    $20E0
    taz     
    .org    $20E1
    tsx     
    sta     $0101,x
    tsx     
    stz     $0102,x
    .org    $20E9
_main:@for_cond4:
    tsx     
    lda     $0102,x
    pha     
    lda     $0101,x
    plx     
    .org    $20F2
    cpx     #$00
    cmp     #$03
    .org    $20F8
    bcs     *+344
    .org    $20FB
_main:@for_body5:
    lda     #$00
    .org    $20FD
    taz     
    .org    $20FE
    tsx     
    sta     $0103,x
    tsx     
    stz     $0104,x
    .org    $2106
_main:@for_cond8_ph:
    lda     #$0A
    .org    $2108
    ldx     #$00
    .org    $210A
    sta     $26
    .org    $210C
    stx     $27
    .org    $210E
    tsx     
    lda     $0102,x
    pha     
    lda     $0101,x
    plx     
    .org    $2117
    sta     $D770
    txa     
    sta     $D771
    lda     #$0A
    sta     $D774
    lda     #$00
    sta     $D775
    lda     $D779
    tax     
    lda     $D778
    .org    $212F
    sta     $28
    .org    $2131
    stx     $29
    .org    $2133
    tsx     
    lda     $0102,x
    pha     
    lda     $0101,x
    plx     
    .org    $213C
    sta     $0A
    .org    $213E
    stx     $0B
    .org    $2140
    lda     $0A
    sta     $D770
    ldx     $0B
    stx     $D771
    lda     #$08
    sta     $D774
    lda     #$00
    sta     $D775
    stz     $D772
    stz     $D773
    stz     $D776
    stz     $D777
    lda     #$2C
    ldx     #$20
    clc     
    adc     $D778
    pha     
    txa     
    adc     $D779
    tax     
    pla     
    sta     $2A
    txa     
    sta     $2B
    .org    $2178
_main:@for_cond8:
    tsx     
    lda     $0104,x
    pha     
    lda     $0103,x
    plx     
    .org    $2181
    cpx     #$00
    cmp     #$04
    .org    $2187
    bcs     *+158
    .org    $218A
_main:@for_body9:
    lda     #$0A
    .org    $218C
    ldx     #$00
    .org    $218E
    sta     $30
    .org    $2190
    stx     $31
    .org    $2192
    tsx     
    lda     $0104,x
    pha     
    lda     $0103,x
    plx     
    .org    $219B
    clc     
    adc     $28
    pha     
    txa     
    adc     $29
    tax     
    pla     
    .org    $21A4
    sta     $32
    .org    $21A6
    stx     $33
    .org    $21A8
    pha     
    .org    $21A9
    phx     
    .org    $21AA
    tsx     
    lda     $0104,x
    pha     
    lda     $0103,x
    plx     
    .org    $21B3
    sta     $0A
    .org    $21B5
    stx     $0B
    .org    $21B7
    lda     $0A
    sta     $D770
    ldx     $0B
    stx     $D771
    lda     #$02
    sta     $D774
    lda     #$00
    sta     $D775
    stz     $D772
    stz     $D773
    stz     $D776
    stz     $D777
    lda     $2A
    ldx     $2B
    clc     
    adc     $D778
    pha     
    txa     
    adc     $D779
    tax     
    pla     
    sta     $08
    txa     
    sta     $09
    .org    $21EF
    plx     
    .org    $21F0
    pla     
    .org    $21F1
    ldy     #$00
    .org    $21F3
    sta     ($08),y
    .org    $21F5
    txa     
    .org    $21F6
    iny     
    .org    $21F7
    sta     ($08),y
    .org    $21F9
_main:@for_inc10:
    tsx     
    lda     $0104,x
    pha     
    lda     $0103,x
    plx     
    .org    $2202
    sta     $36
    .org    $2204
    stx     $37
    .org    $2206
    clc     
    .org    $2207
    adc     #$01
    .org    $2209
    sta     $38
    .org    $220B
    lda     $37
    .org    $220D
    adc     #$00
    .org    $220F
    sta     $39
    .org    $2211
    lda     $38
    .org    $2213
    ldx     $39
    .org    $2215
    pha     
    txa     
    taz     
    pla     
    tsx     
    sta     $0103,x
    tsx     
    stz     $0104,x
    .org    $2221
    bra     *-168
    .org    $2224
_main:@for_end11:
_main:@for_inc6:
    tsx     
    lda     $0102,x
    pha     
    lda     $0101,x
    plx     
    .org    $222D
    sta     $3A
    .org    $222F
    stx     $3B
    .org    $2231
    clc     
    .org    $2232
    adc     #$01
    .org    $2234
    sta     $3C
    .org    $2236
    lda     $3B
    .org    $2238
    adc     #$00
    .org    $223A
    sta     $3D
    .org    $223C
    lda     $3C
    .org    $223E
    ldx     $3D
    .org    $2240
    pha     
    txa     
    taz     
    pla     
    tsx     
    sta     $0101,x
    tsx     
    stz     $0102,x
    .org    $224C
    bra     *-354
    .org    $224F
_main:@for_end7:
    lda     #$00
    .org    $2251
    sta     $20
    .org    $2253
    sta     $21
    .org    $2255
    lda     $20
    ldx     $21
    sta     $02
    stx     $03
    lda     #$27
    ldx     #$20
    clc     
    adc     $02
    pha     
    txa     
    adc     $03
    tax     
    pla     
    sta     $08
    txa     
    sta     $09
    .org    $2271
    ldy     #$00
    .org    $2273
    lda     ($08),y
    .org    $2275
    ldx     #$00
    .org    $2277
    sta     $24
    .org    $2279
    lda     $2025
    .org    $227C
    ldx     $2026
    .org    $227F
    sta     $20
    .org    $2281
    stx     $21
    .org    $2283
    lda     #$00
    .org    $2285
    sta     $22
    .org    $2287
    sta     $23
    .org    $2289
    lda     $24
    .org    $228B
    ldx     #$00
    .org    $228D
    pha     
    .org    $228E
    lda     $22
    ldx     $23
    sta     $02
    stx     $03
    lda     $20
    ldx     $21
    clc     
    adc     $02
    pha     
    txa     
    adc     $03
    tax     
    pla     
    sta     $08
    txa     
    sta     $09
    .org    $22AA
    pla     
    .org    $22AB
    sta     ($08),y
    .org    $22AD
    lda     #$04
    .org    $22AF
    ldx     #$00
    .org    $22B1
    sta     $20
    .org    $22B3
    stx     $21
    .org    $22B5
    lda     $20
    ldx     $21
    sta     $02
    stx     $03
    lda     #$27
    ldx     #$20
    clc     
    adc     $02
    pha     
    txa     
    adc     $03
    tax     
    pla     
    sta     $08
    txa     
    sta     $09
    .org    $22D1
    lda     ($08),y
    .org    $22D3
    ldx     #$00
    .org    $22D5
    sta     $24
    .org    $22D7
    lda     $2025
    .org    $22DA
    ldx     $2026
    .org    $22DD
    sta     $20
    .org    $22DF
    stx     $21
    .org    $22E1
    lda     #$01
    .org    $22E3
    ldx     #$00
    .org    $22E5
    sta     $22
    .org    $22E7
    stx     $23
    .org    $22E9
    lda     $24
    .org    $22EB
    pha     
    .org    $22EC
    lda     $22
    ldx     $23
    sta     $02
    stx     $03
    lda     $20
    ldx     $21
    clc     
    adc     $02
    pha     
    txa     
    adc     $03
    tax     
    pla     
    sta     $08
    txa     
    sta     $09
    .org    $2308
    pla     
    .org    $2309
    sta     ($08),y
    .org    $230B
    lda     #$00
    .org    $230D
    sta     $20
    .org    $230F
    sta     $21
    .org    $2311
    lda     $20
    sta     $D770
    ldx     $21
    stx     $D771
    lda     #$08
    sta     $D774
    lda     #$00
    sta     $D775
    stz     $D772
    stz     $D773
    stz     $D776
    stz     $D777
    lda     #$2C
    ldx     #$20
    clc     
    adc     $D778
    pha     
    txa     
    adc     $D779
    tax     
    pla     
    sta     $22
    txa     
    sta     $23
    .org    $2347
    lda     #$00
    .org    $2349
    sta     $20
    .org    $234B
    sta     $21
    .org    $234D
    lda     $20
    sta     $D770
    ldx     $21
    stx     $D771
    lda     #$02
    sta     $D774
    lda     #$00
    sta     $D775
    stz     $D772
    stz     $D773
    stz     $D776
    stz     $D777
    lda     $22
    ldx     $23
    clc     
    adc     $D778
    pha     
    txa     
    adc     $D779
    tax     
    pla     
    sta     $08
    txa     
    sta     $09
    .org    $2383
    lda     ($08),y
    .org    $2385
    pha     
    .org    $2386
    iny     
    .org    $2387
    lda     ($08),y
    .org    $2389
    tax     
    .org    $238A
    pla     
    .org    $238B
    sta     $26
    .org    $238D
    stx     $27
    .org    $238F
    sta     $20
    .org    $2391
    lda     $2025
    .org    $2394
    ldx     $2026
    .org    $2397
    sta     $22
    .org    $2399
    stx     $23
    .org    $239B
    lda     #$02
    .org    $239D
    ldx     #$00
    .org    $239F
    sta     $24
    .org    $23A1
    stx     $25
    .org    $23A3
    lda     $20
    .org    $23A5
    pha     
    .org    $23A6
    lda     $24
    ldx     $25
    sta     $02
    stx     $03
    lda     $22
    ldx     $23
    clc     
    adc     $02
    pha     
    txa     
    adc     $03
    tax     
    pla     
    sta     $08
    txa     
    sta     $09
    .org    $23C2
    pla     
    .org    $23C3
    ldy     #$00
    .org    $23C5
    sta     ($08),y
    .org    $23C7
    lda     #$01
    .org    $23C9
    ldx     #$00
    .org    $23CB
    sta     $20
    .org    $23CD
    stx     $21
    .org    $23CF
    lda     $20
    sta     $D770
    ldx     $21
    stx     $D771
    lda     #$08
    sta     $D774
    lda     #$00
    sta     $D775
    stz     $D772
    stz     $D773
    stz     $D776
    stz     $D777
    lda     #$2C
    ldx     #$20
    clc     
    adc     $D778
    pha     
    txa     
    adc     $D779
    tax     
    pla     
    sta     $22
    txa     
    sta     $23
    .org    $2405
    lda     #$02
    .org    $2407
    ldx     #$00
    .org    $2409
    sta     $20
    .org    $240B
    stx     $21
    .org    $240D
    lda     $20
    sta     $D770
    ldx     $21
    stx     $D771
    lda     #$02
    sta     $D774
    lda     #$00
    sta     $D775
    stz     $D772
    stz     $D773
    stz     $D776
    stz     $D777
    lda     $22
    ldx     $23
    clc     
    adc     $D778
    pha     
    txa     
    adc     $D779
    tax     
    pla     
    sta     $08
    txa     
    sta     $09
    .org    $2443
    lda     ($08),y
    .org    $2445
    pha     
    .org    $2446
    iny     
    .org    $2447
    lda     ($08),y
    .org    $2449
    tax     
    .org    $244A
    pla     
    .org    $244B
    sta     $26
    .org    $244D
    stx     $27
    .org    $244F
    sta     $20
    .org    $2451
    lda     $2025
    .org    $2454
    ldx     $2026
    .org    $2457
    sta     $22
    .org    $2459
    stx     $23
    .org    $245B
    lda     #$03
    .org    $245D
    ldx     #$00
    .org    $245F
    sta     $24
    .org    $2461
    stx     $25
    .org    $2463
    lda     $20
    .org    $2465
    pha     
    .org    $2466
    lda     $24
    ldx     $25
    sta     $02
    stx     $03
    lda     $22
    ldx     $23
    clc     
    adc     $02
    pha     
    txa     
    adc     $03
    tax     
    pla     
    sta     $08
    txa     
    sta     $09
    .org    $2482
    pla     
    .org    $2483
    ldy     #$00
    .org    $2485
    sta     ($08),y
    .org    $2487
    lda     #$02
    .org    $2489
    ldx     #$00
    .org    $248B
    sta     $20
    .org    $248D
    stx     $21
    .org    $248F
    lda     $20
    sta     $D770
    ldx     $21
    stx     $D771
    lda     #$08
    sta     $D774
    lda     #$00
    sta     $D775
    stz     $D772
    stz     $D773
    stz     $D776
    stz     $D777
    lda     #$2C
    ldx     #$20
    clc     
    adc     $D778
    pha     
    txa     
    adc     $D779
    tax     
    pla     
    sta     $22
    txa     
    sta     $23
    .org    $24C5
    lda     #$03
    .org    $24C7
    ldx     #$00
    .org    $24C9
    sta     $20
    .org    $24CB
    stx     $21
    .org    $24CD
    lda     $20
    sta     $D770
    ldx     $21
    stx     $D771
    lda     #$02
    sta     $D774
    lda     #$00
    sta     $D775
    stz     $D772
    stz     $D773
    stz     $D776
    stz     $D777
    lda     $22
    ldx     $23
    clc     
    adc     $D778
    pha     
    txa     
    adc     $D779
    tax     
    pla     
    sta     $08
    txa     
    sta     $09
    .org    $2503
    lda     ($08),y
    .org    $2505
    pha     
    .org    $2506
    iny     
    .org    $2507
    lda     ($08),y
    .org    $2509
    tax     
    .org    $250A
    pla     
    .org    $250B
    sta     $26
    .org    $250D
    stx     $27
    .org    $250F
    sta     $20
    .org    $2511
    lda     $2025
    .org    $2514
    ldx     $2026
    .org    $2517
    sta     $22
    .org    $2519
    stx     $23
    .org    $251B
    lda     #$04
    .org    $251D
    ldx     #$00
    .org    $251F
    sta     $24
    .org    $2521
    stx     $25
    .org    $2523
    lda     $20
    .org    $2525
    pha     
    .org    $2526
    lda     $24
    ldx     $25
    sta     $02
    stx     $03
    lda     $22
    ldx     $23
    clc     
    adc     $02
    pha     
    txa     
    adc     $03
    tax     
    pla     
    sta     $08
    txa     
    sta     $09
    .org    $2542
    pla     
    .org    $2543
    ldy     #$00
    .org    $2545
    sta     ($08),y
    .org    $2547
    lda     #$AA
    .org    $2549
    sta     $20
    .org    $254B
    lda     $2025
    .org    $254E
    ldx     $2026
    .org    $2551
    sta     $22
    .org    $2553
    stx     $23
    .org    $2555
    lda     #$05
    .org    $2557
    ldx     #$00
    .org    $2559
    sta     $24
    .org    $255B
    stx     $25
    .org    $255D
    lda     $20
    .org    $255F
    pha     
    .org    $2560
    lda     $24
    ldx     $25
    sta     $02
    stx     $03
    lda     $22
    ldx     $23
    clc     
    adc     $02
    pha     
    txa     
    adc     $03
    tax     
    pla     
    sta     $08
    txa     
    sta     $09
    .org    $257C
    pla     
    .org    $257D
    sta     ($08),y
    .org    $257F
_main:@__return:
    plz     
    .org    $2580
    plz     
    .org    $2581
    plz     
    .org    $2582
    plz     
    .org    $2583
    rts     
    .org    $2584
__zp_save_buf:
