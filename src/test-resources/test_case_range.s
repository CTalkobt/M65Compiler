    .o45
    .org $2000
    .weak __sp_base
    __sp_base = $0101
    .weak __static_chain
    .weak __zp_scratch
    .weak __zp_scratch2
    .weak __zp_scratch3
    .weak __zp_scratch4
    .weak cc45.zeroPageStart
    __static_chain = $06
    __zp_scratch = $08
    __zp_scratch2 = $0A
    __zp_scratch3 = $0C
    __zp_scratch4 = $0E
    cc45.zeroPageStart = $08

    .global _classify_char
    .global _classify_int
    .global _mixed_cases
    .global _main

    .segment "code"

; function _classify_char
; SAC inline storage: 2 bytes
    .global _classify_char__param_c
    _classify_char__param_c: .word 0
    _classify_char__local_0: .word 0
    proc _classify_char, B#@_p_c
    .sac
    .var _fp = 0
    .loc "test_case_range.c", 3
    .var @_p_c = 2
; .debug_var: __classify_char @_p_c offset=2 size=2 type=int8 scope=parameter

@entry:
    .loc "test_case_range.c", 4
    lda _classify_char__param_c
    cmp #97
    bcs @range_upper6
    bra @case_skip5
@range_upper6:
    lda _classify_char__param_c
    cmp #122
    bcc @case1
    beq @case1
@case_skip5:
    lda _classify_char__param_c
    cmp #65
    bcs @range_upper8
    bra @case_skip7
@range_upper8:
    lda _classify_char__param_c
    cmp #90
    bcc @case2
    beq @case2
@case_skip7:
    lda _classify_char__param_c
    cmp #48
    bcs @range_upper10
    bra @default4
@range_upper10:
    lda _classify_char__param_c
    cmp #57
    bcc @case3
    beq @case3
    bra @default4
@case1:
    .loc "test_case_range.c", 6
    lda #1
    ldx #0
    bra @__return
@case2:
    .loc "test_case_range.c", 8
    lda #2
    ldx #0
    bra @__return
@case3:
    .loc "test_case_range.c", 10
    lda #3
    ldx #0
    bra @__return
@default4:
    .loc "test_case_range.c", 12
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _classify_int
; SAC inline storage: 2 bytes
    .global _classify_int__param_val
    _classify_int__param_val: .word 0
    _classify_int__local_0: .word 0
    proc _classify_int, W#@_p_val
    .sac
    .var _fp = 0
    .loc "test_case_range.c", 16
    .var @_p_val = 2
; .debug_var: __classify_int @_p_val offset=2 size=2 type=int16 scope=parameter

@entry:
    .loc "test_case_range.c", 17
    lda _classify_int__param_val
    ldx _classify_int__param_val+1
    cmp.16 .AX, #0
    bcs @range_upper18
    bra @case_skip17
@range_upper18:
    lda _classify_int__param_val
    ldx _classify_int__param_val+1
    cmp.16 .AX, #9
    bcc @case12
    beq @case12
@case_skip17:
    lda _classify_int__param_val
    ldx _classify_int__param_val+1
    cmp.16 .AX, #10
    bcs @range_upper20
    bra @case_skip19
@range_upper20:
    lda _classify_int__param_val
    ldx _classify_int__param_val+1
    cmp.16 .AX, #99
    bcc @case13
    beq @case13
@case_skip19:
    lda _classify_int__param_val
    ldx _classify_int__param_val+1
    cmp.16 .AX, #100
    bcs @range_upper22
    bra @case_skip21
@range_upper22:
    lda _classify_int__param_val
    ldx _classify_int__param_val+1
    cmp.16 .AX, #999
    bcc @case14
    beq @case14
@case_skip21:
    lda _classify_int__param_val
    ldx _classify_int__param_val+1
    cmp.16 .AX, #1000
    beq @case15
    bra @default16
@case12:
    .loc "test_case_range.c", 19
    lda #1
    ldx #0
    bra @__return
@case13:
    .loc "test_case_range.c", 21
    lda #2
    ldx #0
    bra @__return
@case14:
    .loc "test_case_range.c", 23
    lda #3
    ldx #0
    bra @__return
@case15:
    .loc "test_case_range.c", 25
    lda #4
    ldx #0
    bra @__return
@default16:
    .loc "test_case_range.c", 27
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _mixed_cases
; SAC inline storage: 2 bytes
    .global _mixed_cases__param_x
    _mixed_cases__param_x: .word 0
    _mixed_cases__local_0: .word 0
    proc _mixed_cases, W#@_p_x
    .sac
    .var _fp = 0
    .loc "test_case_range.c", 32
    .var @_p_x = 2
; .debug_var: __mixed_cases @_p_x offset=2 size=2 type=int16 scope=parameter

@entry:
    .loc "test_case_range.c", 33
    lda _mixed_cases__param_x
    ldx _mixed_cases__param_x+1
    cmp.16 .AX, #0
    beq @case25
@case_skip30:
    lda _mixed_cases__param_x
    ldx _mixed_cases__param_x+1
    cmp.16 .AX, #1
    bcs @range_upper32
    bra @case_skip31
@range_upper32:
    lda _mixed_cases__param_x
    ldx _mixed_cases__param_x+1
    cmp.16 .AX, #5
    bcc @case26
    beq @case26
@case_skip31:
    lda _mixed_cases__param_x
    ldx _mixed_cases__param_x+1
    cmp.16 .AX, #6
    beq @case27
@case_skip33:
    lda _mixed_cases__param_x
    ldx _mixed_cases__param_x+1
    cmp.16 .AX, #7
    bcs @range_upper35
    bra @default29
@range_upper35:
    lda _mixed_cases__param_x
    ldx _mixed_cases__param_x+1
    cmp.16 .AX, #10
    bcc @case28
    beq @case28
    bra @default29
@case25:
    .loc "test_case_range.c", 35
    lda #10
    ldx #0
    bra @__return
@case26:
    .loc "test_case_range.c", 37
    lda #20
    ldx #0
    bra @__return
@case27:
    .loc "test_case_range.c", 39
    lda #30
    ldx #0
    bra @__return
@case28:
    .loc "test_case_range.c", 41
    lda #40
    ldx #0
    bra @__return
@default29:
    .loc "test_case_range.c", 43
    lda #50
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _main
; SAC zero-alloc leaf: no storage overhead
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_case_range.c", 47

@entry:
    .loc "test_case_range.c", 49
    lda #97
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda $22
    ldx #0
    sta $20
    .loc "test_case_range.c", 4
    lda $22
    cmp #97
    bcs @range_upper46
    bra @case_skip45
@range_upper46:
    lda $20
    cmp #122
    bcc @case41
    beq @case41
@case_skip45:
    lda $20
    cmp #65
    bcs @range_upper48
    bra @case_skip47
@range_upper48:
    lda $20
    cmp #90
    bcc @case42
    beq @case42
@case_skip47:
    lda $20
    cmp #48
    bcs @range_upper50
    bra @default44
@range_upper50:
    lda $20
    cmp #57
    bcc @case43
    beq @case43
    bra @default44
@case41:
    .loc "test_case_range.c", 6
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end39
@case42:
    .loc "test_case_range.c", 8
    lda #2
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end39
@case43:
    .loc "test_case_range.c", 10
    lda #3
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end39
@default44:
    .loc "test_case_range.c", 12
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end39:
    .loc "test_case_range.c", 49
    lda $22
    ldx $23
    cmp.16 .AX, #1
    bne @if_then36
    bra @if_end38
@if_then36:
    lda #1
    ldx #0
    bra @__return
@if_end38:
    .loc "test_case_range.c", 50
    lda #109
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda $22
    ldx #0
    sta $20
    .loc "test_case_range.c", 4
    lda $22
    cmp #97
    bcs @range_upper65
    bra @case_skip64
@range_upper65:
    lda $20
    cmp #122
    bcc @case60
    beq @case60
@case_skip64:
    lda $20
    cmp #65
    bcs @range_upper67
    bra @case_skip66
@range_upper67:
    lda $20
    cmp #90
    bcc @case61
    beq @case61
@case_skip66:
    lda $20
    cmp #48
    bcs @range_upper69
    bra @default63
@range_upper69:
    lda $20
    cmp #57
    bcc @case62
    beq @case62
    bra @default63
@case60:
    .loc "test_case_range.c", 6
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end58
@case61:
    .loc "test_case_range.c", 8
    lda #2
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end58
@case62:
    .loc "test_case_range.c", 10
    lda #3
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end58
@default63:
    .loc "test_case_range.c", 12
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end58:
    .loc "test_case_range.c", 50
    lda $22
    ldx $23
    cmp.16 .AX, #1
    bne @if_then55
    bra @if_end57
@if_then55:
    lda #2
    ldx #0
    bra @__return
@if_end57:
    .loc "test_case_range.c", 51
    lda #122
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda $22
    ldx #0
    sta $20
    .loc "test_case_range.c", 4
    lda $22
    cmp #97
    bcs @range_upper84
    bra @case_skip83
@range_upper84:
    lda $20
    cmp #122
    bcc @case79
    beq @case79
@case_skip83:
    lda $20
    cmp #65
    bcs @range_upper86
    bra @case_skip85
@range_upper86:
    lda $20
    cmp #90
    bcc @case80
    beq @case80
@case_skip85:
    lda $20
    cmp #48
    bcs @range_upper88
    bra @default82
@range_upper88:
    lda $20
    cmp #57
    bcc @case81
    beq @case81
    bra @default82
@case79:
    .loc "test_case_range.c", 6
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end77
@case80:
    .loc "test_case_range.c", 8
    lda #2
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end77
@case81:
    .loc "test_case_range.c", 10
    lda #3
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end77
@default82:
    .loc "test_case_range.c", 12
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end77:
    .loc "test_case_range.c", 51
    lda $22
    ldx $23
    cmp.16 .AX, #1
    bne @if_then74
    bra @if_end76
@if_then74:
    lda #3
    ldx #0
    bra @__return
@if_end76:
    .loc "test_case_range.c", 52
    lda #65
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda $22
    ldx #0
    sta $20
    .loc "test_case_range.c", 4
    lda $22
    cmp #97
    bcs @range_upper103
    bra @case_skip102
@range_upper103:
    lda $20
    cmp #122
    bcc @case98
    beq @case98
@case_skip102:
    lda $20
    cmp #65
    bcs @range_upper105
    bra @case_skip104
@range_upper105:
    lda $20
    cmp #90
    bcc @case99
    beq @case99
@case_skip104:
    lda $20
    cmp #48
    bcs @range_upper107
    bra @default101
@range_upper107:
    lda $20
    cmp #57
    bcc @case100
    beq @case100
    bra @default101
@case98:
    .loc "test_case_range.c", 6
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end96
@case99:
    .loc "test_case_range.c", 8
    lda #2
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end96
@case100:
    .loc "test_case_range.c", 10
    lda #3
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end96
@default101:
    .loc "test_case_range.c", 12
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end96:
    .loc "test_case_range.c", 52
    lda $22
    ldx $23
    cmp.16 .AX, #2
    bne @if_then93
    bra @if_end95
@if_then93:
    lda #4
    ldx #0
    bra @__return
@if_end95:
    .loc "test_case_range.c", 53
    lda #77
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda $22
    ldx #0
    sta $20
    .loc "test_case_range.c", 4
    lda $22
    cmp #97
    bcs @range_upper122
    bra @case_skip121
@range_upper122:
    lda $20
    cmp #122
    bcc @case117
    beq @case117
@case_skip121:
    lda $20
    cmp #65
    bcs @range_upper124
    bra @case_skip123
@range_upper124:
    lda $20
    cmp #90
    bcc @case118
    beq @case118
@case_skip123:
    lda $20
    cmp #48
    bcs @range_upper126
    bra @default120
@range_upper126:
    lda $20
    cmp #57
    bcc @case119
    beq @case119
    bra @default120
@case117:
    .loc "test_case_range.c", 6
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end115
@case118:
    .loc "test_case_range.c", 8
    lda #2
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end115
@case119:
    .loc "test_case_range.c", 10
    lda #3
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end115
@default120:
    .loc "test_case_range.c", 12
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end115:
    .loc "test_case_range.c", 53
    lda $22
    ldx $23
    cmp.16 .AX, #2
    bne @if_then112
    bra @if_end114
@if_then112:
    lda #5
    ldx #0
    bra @__return
@if_end114:
    .loc "test_case_range.c", 54
    lda #90
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda $22
    ldx #0
    sta $20
    .loc "test_case_range.c", 4
    lda $22
    cmp #97
    bcs @range_upper141
    bra @case_skip140
@range_upper141:
    lda $20
    cmp #122
    bcc @case136
    beq @case136
@case_skip140:
    lda $20
    cmp #65
    bcs @range_upper143
    bra @case_skip142
@range_upper143:
    lda $20
    cmp #90
    bcc @case137
    beq @case137
@case_skip142:
    lda $20
    cmp #48
    bcs @range_upper145
    bra @default139
@range_upper145:
    lda $20
    cmp #57
    bcc @case138
    beq @case138
    bra @default139
@case136:
    .loc "test_case_range.c", 6
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end134
@case137:
    .loc "test_case_range.c", 8
    lda #2
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end134
@case138:
    .loc "test_case_range.c", 10
    lda #3
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end134
@default139:
    .loc "test_case_range.c", 12
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end134:
    .loc "test_case_range.c", 54
    lda $22
    ldx $23
    cmp.16 .AX, #2
    bne @if_then131
    bra @if_end133
@if_then131:
    lda #6
    ldx #0
    bra @__return
@if_end133:
    .loc "test_case_range.c", 55
    lda #48
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda $22
    ldx #0
    sta $20
    .loc "test_case_range.c", 4
    lda $22
    cmp #97
    bcs @range_upper160
    bra @case_skip159
@range_upper160:
    lda $20
    cmp #122
    bcc @case155
    beq @case155
@case_skip159:
    lda $20
    cmp #65
    bcs @range_upper162
    bra @case_skip161
@range_upper162:
    lda $20
    cmp #90
    bcc @case156
    beq @case156
@case_skip161:
    lda $20
    cmp #48
    bcs @range_upper164
    bra @default158
@range_upper164:
    lda $20
    cmp #57
    bcc @case157
    beq @case157
    bra @default158
@case155:
    .loc "test_case_range.c", 6
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end153
@case156:
    .loc "test_case_range.c", 8
    lda #2
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end153
@case157:
    .loc "test_case_range.c", 10
    lda #3
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end153
@default158:
    .loc "test_case_range.c", 12
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end153:
    .loc "test_case_range.c", 55
    lda $22
    ldx $23
    cmp.16 .AX, #3
    bne @if_then150
    bra @if_end152
@if_then150:
    lda #7
    ldx #0
    bra @__return
@if_end152:
    .loc "test_case_range.c", 56
    lda #53
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda $22
    ldx #0
    sta $20
    .loc "test_case_range.c", 4
    lda $22
    cmp #97
    bcs @range_upper179
    bra @case_skip178
@range_upper179:
    lda $20
    cmp #122
    bcc @case174
    beq @case174
@case_skip178:
    lda $20
    cmp #65
    bcs @range_upper181
    bra @case_skip180
@range_upper181:
    lda $20
    cmp #90
    bcc @case175
    beq @case175
@case_skip180:
    lda $20
    cmp #48
    bcs @range_upper183
    bra @default177
@range_upper183:
    lda $20
    cmp #57
    bcc @case176
    beq @case176
    bra @default177
@case174:
    .loc "test_case_range.c", 6
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end172
@case175:
    .loc "test_case_range.c", 8
    lda #2
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end172
@case176:
    .loc "test_case_range.c", 10
    lda #3
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end172
@default177:
    .loc "test_case_range.c", 12
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end172:
    .loc "test_case_range.c", 56
    lda $22
    ldx $23
    cmp.16 .AX, #3
    bne @if_then169
    bra @if_end171
@if_then169:
    lda #8
    ldx #0
    bra @__return
@if_end171:
    .loc "test_case_range.c", 57
    lda #57
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda $22
    ldx #0
    sta $20
    .loc "test_case_range.c", 4
    lda $22
    cmp #97
    bcs @range_upper198
    bra @case_skip197
@range_upper198:
    lda $20
    cmp #122
    bcc @case193
    beq @case193
@case_skip197:
    lda $20
    cmp #65
    bcs @range_upper200
    bra @case_skip199
@range_upper200:
    lda $20
    cmp #90
    bcc @case194
    beq @case194
@case_skip199:
    lda $20
    cmp #48
    bcs @range_upper202
    bra @default196
@range_upper202:
    lda $20
    cmp #57
    bcc @case195
    beq @case195
    bra @default196
@case193:
    .loc "test_case_range.c", 6
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end191
@case194:
    .loc "test_case_range.c", 8
    lda #2
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end191
@case195:
    .loc "test_case_range.c", 10
    lda #3
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end191
@default196:
    .loc "test_case_range.c", 12
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end191:
    .loc "test_case_range.c", 57
    lda $22
    ldx $23
    cmp.16 .AX, #3
    bne @if_then188
    bra @if_end190
@if_then188:
    lda #9
    ldx #0
    bra @__return
@if_end190:
    .loc "test_case_range.c", 58
    lda #32
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda $22
    ldx #0
    sta $20
    .loc "test_case_range.c", 4
    lda $22
    cmp #97
    bcs @range_upper217
    bra @case_skip216
@range_upper217:
    lda $20
    cmp #122
    bcc @case212
    beq @case212
@case_skip216:
    lda $20
    cmp #65
    bcs @range_upper219
    bra @case_skip218
@range_upper219:
    lda $20
    cmp #90
    bcc @case213
    beq @case213
@case_skip218:
    lda $20
    cmp #48
    bcs @range_upper221
    bra @default215
@range_upper221:
    lda $20
    cmp #57
    bcc @case214
    beq @case214
    bra @default215
@case212:
    .loc "test_case_range.c", 6
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end210
@case213:
    .loc "test_case_range.c", 8
    lda #2
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end210
@case214:
    .loc "test_case_range.c", 10
    lda #3
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end210
@default215:
    .loc "test_case_range.c", 12
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end210:
    .loc "test_case_range.c", 58
    lda $22
    ora $23
    bne @if_then207
    bra @if_end209
@if_then207:
    lda #10
    ldx #0
    bra @__return
@if_end209:
    .loc "test_case_range.c", 61
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_case_range.c", 17
    lda $20
    ldx $21
    cmp.16 .AX, #0
    bcs @range_upper237
    bra @case_skip236
@range_upper237:
    lda $22
    ldx $23
    cmp.16 .AX, #9
    bcc @case231
    beq @case231
@case_skip236:
    lda $22
    ldx $23
    cmp.16 .AX, #10
    bcs @range_upper239
    bra @case_skip238
@range_upper239:
    lda $22
    ldx $23
    cmp.16 .AX, #99
    bcc @case232
    beq @case232
@case_skip238:
    lda $22
    ldx $23
    cmp.16 .AX, #100
    bcs @range_upper241
    bra @case_skip240
@range_upper241:
    lda $22
    ldx $23
    cmp.16 .AX, #999
    bcc @case233
    beq @case233
@case_skip240:
    lda $22
    ldx $23
    cmp.16 .AX, #1000
    beq @case234
    bra @default235
@case231:
    .loc "test_case_range.c", 19
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end229
@case232:
    .loc "test_case_range.c", 21
    lda #2
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end229
@case233:
    .loc "test_case_range.c", 23
    lda #3
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end229
@case234:
    .loc "test_case_range.c", 25
    lda #4
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end229
@default235:
    .loc "test_case_range.c", 27
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end229:
    .loc "test_case_range.c", 61
    lda $22
    ldx $23
    cmp.16 .AX, #1
    bne @if_then226
    bra @if_end228
@if_then226:
    lda #11
    ldx #0
    bra @__return
@if_end228:
    .loc "test_case_range.c", 62
    lda #9
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_case_range.c", 17
    lda $20
    ldx $21
    cmp.16 .AX, #0
    bcs @range_upper259
    bra @case_skip258
@range_upper259:
    lda $22
    ldx $23
    cmp.16 .AX, #9
    bcc @case253
    beq @case253
@case_skip258:
    lda $22
    ldx $23
    cmp.16 .AX, #10
    bcs @range_upper261
    bra @case_skip260
@range_upper261:
    lda $22
    ldx $23
    cmp.16 .AX, #99
    bcc @case254
    beq @case254
@case_skip260:
    lda $22
    ldx $23
    cmp.16 .AX, #100
    bcs @range_upper263
    bra @case_skip262
@range_upper263:
    lda $22
    ldx $23
    cmp.16 .AX, #999
    bcc @case255
    beq @case255
@case_skip262:
    lda $22
    ldx $23
    cmp.16 .AX, #1000
    beq @case256
    bra @default257
@case253:
    .loc "test_case_range.c", 19
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end251
@case254:
    .loc "test_case_range.c", 21
    lda #2
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end251
@case255:
    .loc "test_case_range.c", 23
    lda #3
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end251
@case256:
    .loc "test_case_range.c", 25
    lda #4
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end251
@default257:
    .loc "test_case_range.c", 27
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end251:
    .loc "test_case_range.c", 62
    lda $22
    ldx $23
    cmp.16 .AX, #1
    bne @if_then248
    bra @if_end250
@if_then248:
    lda #12
    ldx #0
    bra @__return
@if_end250:
    .loc "test_case_range.c", 63
    lda #10
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_case_range.c", 17
    lda $20
    ldx $21
    cmp.16 .AX, #0
    bcs @range_upper281
    bra @case_skip280
@range_upper281:
    lda $22
    ldx $23
    cmp.16 .AX, #9
    bcc @case275
    beq @case275
@case_skip280:
    lda $22
    ldx $23
    cmp.16 .AX, #10
    bcs @range_upper283
    bra @case_skip282
@range_upper283:
    lda $22
    ldx $23
    cmp.16 .AX, #99
    bcc @case276
    beq @case276
@case_skip282:
    lda $22
    ldx $23
    cmp.16 .AX, #100
    bcs @range_upper285
    bra @case_skip284
@range_upper285:
    lda $22
    ldx $23
    cmp.16 .AX, #999
    bcc @case277
    beq @case277
@case_skip284:
    lda $22
    ldx $23
    cmp.16 .AX, #1000
    beq @case278
    bra @default279
@case275:
    .loc "test_case_range.c", 19
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end273
@case276:
    .loc "test_case_range.c", 21
    lda #2
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end273
@case277:
    .loc "test_case_range.c", 23
    lda #3
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end273
@case278:
    .loc "test_case_range.c", 25
    lda #4
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end273
@default279:
    .loc "test_case_range.c", 27
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end273:
    .loc "test_case_range.c", 63
    lda $22
    ldx $23
    cmp.16 .AX, #2
    bne @if_then270
    bra @if_end272
@if_then270:
    lda #13
    ldx #0
    bra @__return
@if_end272:
    .loc "test_case_range.c", 64
    lda #99
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_case_range.c", 17
    lda $20
    ldx $21
    cmp.16 .AX, #0
    bcs @range_upper303
    bra @case_skip302
@range_upper303:
    lda $22
    ldx $23
    cmp.16 .AX, #9
    bcc @case297
    beq @case297
@case_skip302:
    lda $22
    ldx $23
    cmp.16 .AX, #10
    bcs @range_upper305
    bra @case_skip304
@range_upper305:
    lda $22
    ldx $23
    cmp.16 .AX, #99
    bcc @case298
    beq @case298
@case_skip304:
    lda $22
    ldx $23
    cmp.16 .AX, #100
    bcs @range_upper307
    bra @case_skip306
@range_upper307:
    lda $22
    ldx $23
    cmp.16 .AX, #999
    bcc @case299
    beq @case299
@case_skip306:
    lda $22
    ldx $23
    cmp.16 .AX, #1000
    beq @case300
    bra @default301
@case297:
    .loc "test_case_range.c", 19
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end295
@case298:
    .loc "test_case_range.c", 21
    lda #2
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end295
@case299:
    .loc "test_case_range.c", 23
    lda #3
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end295
@case300:
    .loc "test_case_range.c", 25
    lda #4
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end295
@default301:
    .loc "test_case_range.c", 27
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end295:
    .loc "test_case_range.c", 64
    lda $22
    ldx $23
    cmp.16 .AX, #2
    bne @if_then292
    bra @if_end294
@if_then292:
    lda #14
    ldx #0
    bra @__return
@if_end294:
    .loc "test_case_range.c", 65
    lda #100
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_case_range.c", 17
    lda $20
    ldx $21
    cmp.16 .AX, #0
    bcs @range_upper325
    bra @case_skip324
@range_upper325:
    lda $22
    ldx $23
    cmp.16 .AX, #9
    bcc @case319
    beq @case319
@case_skip324:
    lda $22
    ldx $23
    cmp.16 .AX, #10
    bcs @range_upper327
    bra @case_skip326
@range_upper327:
    lda $22
    ldx $23
    cmp.16 .AX, #99
    bcc @case320
    beq @case320
@case_skip326:
    lda $22
    ldx $23
    cmp.16 .AX, #100
    bcs @range_upper329
    bra @case_skip328
@range_upper329:
    lda $22
    ldx $23
    cmp.16 .AX, #999
    bcc @case321
    beq @case321
@case_skip328:
    lda $22
    ldx $23
    cmp.16 .AX, #1000
    beq @case322
    bra @default323
@case319:
    .loc "test_case_range.c", 19
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end317
@case320:
    .loc "test_case_range.c", 21
    lda #2
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end317
@case321:
    .loc "test_case_range.c", 23
    lda #3
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end317
@case322:
    .loc "test_case_range.c", 25
    lda #4
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end317
@default323:
    .loc "test_case_range.c", 27
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end317:
    .loc "test_case_range.c", 65
    lda $22
    ldx $23
    cmp.16 .AX, #3
    bne @if_then314
    bra @if_end316
@if_then314:
    lda #15
    ldx #0
    bra @__return
@if_end316:
    .loc "test_case_range.c", 66
    lda #231
    ldx #3
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_case_range.c", 17
    lda $20
    ldx $21
    cmp.16 .AX, #0
    bcs @range_upper347
    bra @case_skip346
@range_upper347:
    lda $22
    ldx $23
    cmp.16 .AX, #9
    bcc @case341
    beq @case341
@case_skip346:
    lda $22
    ldx $23
    cmp.16 .AX, #10
    bcs @range_upper349
    bra @case_skip348
@range_upper349:
    lda $22
    ldx $23
    cmp.16 .AX, #99
    bcc @case342
    beq @case342
@case_skip348:
    lda $22
    ldx $23
    cmp.16 .AX, #100
    bcs @range_upper351
    bra @case_skip350
@range_upper351:
    lda $22
    ldx $23
    cmp.16 .AX, #999
    bcc @case343
    beq @case343
@case_skip350:
    lda $22
    ldx $23
    cmp.16 .AX, #1000
    beq @case344
    bra @default345
@case341:
    .loc "test_case_range.c", 19
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end339
@case342:
    .loc "test_case_range.c", 21
    lda #2
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end339
@case343:
    .loc "test_case_range.c", 23
    lda #3
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end339
@case344:
    .loc "test_case_range.c", 25
    lda #4
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end339
@default345:
    .loc "test_case_range.c", 27
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end339:
    .loc "test_case_range.c", 66
    lda $22
    ldx $23
    cmp.16 .AX, #3
    bne @if_then336
    bra @if_end338
@if_then336:
    lda #16
    ldx #0
    bra @__return
@if_end338:
    .loc "test_case_range.c", 67
    lda #232
    ldx #3
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_case_range.c", 17
    lda $20
    ldx $21
    cmp.16 .AX, #0
    bcs @range_upper369
    bra @case_skip368
@range_upper369:
    lda $22
    ldx $23
    cmp.16 .AX, #9
    bcc @case363
    beq @case363
@case_skip368:
    lda $22
    ldx $23
    cmp.16 .AX, #10
    bcs @range_upper371
    bra @case_skip370
@range_upper371:
    lda $22
    ldx $23
    cmp.16 .AX, #99
    bcc @case364
    beq @case364
@case_skip370:
    lda $22
    ldx $23
    cmp.16 .AX, #100
    bcs @range_upper373
    bra @case_skip372
@range_upper373:
    lda $22
    ldx $23
    cmp.16 .AX, #999
    bcc @case365
    beq @case365
@case_skip372:
    lda $22
    ldx $23
    cmp.16 .AX, #1000
    beq @case366
    bra @default367
@case363:
    .loc "test_case_range.c", 19
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end361
@case364:
    .loc "test_case_range.c", 21
    lda #2
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end361
@case365:
    .loc "test_case_range.c", 23
    lda #3
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end361
@case366:
    .loc "test_case_range.c", 25
    lda #4
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end361
@default367:
    .loc "test_case_range.c", 27
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end361:
    .loc "test_case_range.c", 67
    lda $22
    ldx $23
    cmp.16 .AX, #4
    bne @if_then358
    bra @if_end360
@if_then358:
    lda #17
    ldx #0
    bra @__return
@if_end360:
    .loc "test_case_range.c", 68
    lda #233
    ldx #3
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_case_range.c", 17
    lda $20
    ldx $21
    cmp.16 .AX, #0
    bcs @range_upper391
    bra @case_skip390
@range_upper391:
    lda $22
    ldx $23
    cmp.16 .AX, #9
    bcc @case385
    beq @case385
@case_skip390:
    lda $22
    ldx $23
    cmp.16 .AX, #10
    bcs @range_upper393
    bra @case_skip392
@range_upper393:
    lda $22
    ldx $23
    cmp.16 .AX, #99
    bcc @case386
    beq @case386
@case_skip392:
    lda $22
    ldx $23
    cmp.16 .AX, #100
    bcs @range_upper395
    bra @case_skip394
@range_upper395:
    lda $22
    ldx $23
    cmp.16 .AX, #999
    bcc @case387
    beq @case387
@case_skip394:
    lda $22
    ldx $23
    cmp.16 .AX, #1000
    beq @case388
    bra @default389
@case385:
    .loc "test_case_range.c", 19
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end383
@case386:
    .loc "test_case_range.c", 21
    lda #2
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end383
@case387:
    .loc "test_case_range.c", 23
    lda #3
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end383
@case388:
    .loc "test_case_range.c", 25
    lda #4
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end383
@default389:
    .loc "test_case_range.c", 27
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end383:
    .loc "test_case_range.c", 68
    lda $22
    ora $23
    bne @if_then380
    bra @if_end382
@if_then380:
    lda #18
    ldx #0
    bra @__return
@if_end382:
    .loc "test_case_range.c", 71
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_case_range.c", 33
    lda $20
    ldx $21
    cmp.16 .AX, #0
    beq @case407
@case_skip412:
    lda $22
    ldx $23
    cmp.16 .AX, #1
    bcs @range_upper414
    bra @case_skip413
@range_upper414:
    lda $22
    ldx $23
    cmp.16 .AX, #5
    bcc @case408
    beq @case408
@case_skip413:
    lda $22
    ldx $23
    cmp.16 .AX, #6
    beq @case409
@case_skip415:
    lda $22
    ldx $23
    cmp.16 .AX, #7
    bcs @range_upper417
    bra @default411
@range_upper417:
    lda $22
    ldx $23
    cmp.16 .AX, #10
    bcc @case410
    beq @case410
    bra @default411
@case407:
    .loc "test_case_range.c", 35
    lda #10
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end405
@case408:
    .loc "test_case_range.c", 37
    lda #20
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end405
@case409:
    .loc "test_case_range.c", 39
    lda #30
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end405
@case410:
    .loc "test_case_range.c", 41
    lda #40
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end405
@default411:
    .loc "test_case_range.c", 43
    lda #50
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end405:
    .loc "test_case_range.c", 71
    lda $22
    ldx $23
    cmp.16 .AX, #10
    bne @if_then402
    bra @if_end404
@if_then402:
    lda #19
    ldx #0
    bra @__return
@if_end404:
    .loc "test_case_range.c", 72
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_case_range.c", 33
    lda $20
    ldx $21
    cmp.16 .AX, #0
    beq @case428
@case_skip433:
    lda $22
    ldx $23
    cmp.16 .AX, #1
    bcs @range_upper435
    bra @case_skip434
@range_upper435:
    lda $22
    ldx $23
    cmp.16 .AX, #5
    bcc @case429
    beq @case429
@case_skip434:
    lda $22
    ldx $23
    cmp.16 .AX, #6
    beq @case430
@case_skip436:
    lda $22
    ldx $23
    cmp.16 .AX, #7
    bcs @range_upper438
    bra @default432
@range_upper438:
    lda $22
    ldx $23
    cmp.16 .AX, #10
    bcc @case431
    beq @case431
    bra @default432
@case428:
    .loc "test_case_range.c", 35
    lda #10
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end426
@case429:
    .loc "test_case_range.c", 37
    lda #20
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end426
@case430:
    .loc "test_case_range.c", 39
    lda #30
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end426
@case431:
    .loc "test_case_range.c", 41
    lda #40
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end426
@default432:
    .loc "test_case_range.c", 43
    lda #50
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end426:
    .loc "test_case_range.c", 72
    lda $22
    ldx $23
    cmp.16 .AX, #20
    bne @if_then423
    bra @if_end425
@if_then423:
    lda #20
    ldx #0
    bra @__return
@if_end425:
    .loc "test_case_range.c", 73
    lda #3
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_case_range.c", 33
    lda $20
    ldx $21
    cmp.16 .AX, #0
    beq @case449
@case_skip454:
    lda $22
    ldx $23
    cmp.16 .AX, #1
    bcs @range_upper456
    bra @case_skip455
@range_upper456:
    lda $22
    ldx $23
    cmp.16 .AX, #5
    bcc @case450
    beq @case450
@case_skip455:
    lda $22
    ldx $23
    cmp.16 .AX, #6
    beq @case451
@case_skip457:
    lda $22
    ldx $23
    cmp.16 .AX, #7
    bcs @range_upper459
    bra @default453
@range_upper459:
    lda $22
    ldx $23
    cmp.16 .AX, #10
    bcc @case452
    beq @case452
    bra @default453
@case449:
    .loc "test_case_range.c", 35
    lda #10
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end447
@case450:
    .loc "test_case_range.c", 37
    lda #20
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end447
@case451:
    .loc "test_case_range.c", 39
    lda #30
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end447
@case452:
    .loc "test_case_range.c", 41
    lda #40
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end447
@default453:
    .loc "test_case_range.c", 43
    lda #50
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end447:
    .loc "test_case_range.c", 73
    lda $22
    ldx $23
    cmp.16 .AX, #20
    bne @if_then444
    bra @if_end446
@if_then444:
    lda #21
    ldx #0
    bra @__return
@if_end446:
    .loc "test_case_range.c", 74
    lda #5
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_case_range.c", 33
    lda $20
    ldx $21
    cmp.16 .AX, #0
    beq @case470
@case_skip475:
    lda $22
    ldx $23
    cmp.16 .AX, #1
    bcs @range_upper477
    bra @case_skip476
@range_upper477:
    lda $22
    ldx $23
    cmp.16 .AX, #5
    bcc @case471
    beq @case471
@case_skip476:
    lda $22
    ldx $23
    cmp.16 .AX, #6
    beq @case472
@case_skip478:
    lda $22
    ldx $23
    cmp.16 .AX, #7
    bcs @range_upper480
    bra @default474
@range_upper480:
    lda $22
    ldx $23
    cmp.16 .AX, #10
    bcc @case473
    beq @case473
    bra @default474
@case470:
    .loc "test_case_range.c", 35
    lda #10
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end468
@case471:
    .loc "test_case_range.c", 37
    lda #20
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end468
@case472:
    .loc "test_case_range.c", 39
    lda #30
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end468
@case473:
    .loc "test_case_range.c", 41
    lda #40
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end468
@default474:
    .loc "test_case_range.c", 43
    lda #50
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end468:
    .loc "test_case_range.c", 74
    lda $22
    ldx $23
    cmp.16 .AX, #20
    bne @if_then465
    bra @if_end467
@if_then465:
    lda #22
    ldx #0
    bra @__return
@if_end467:
    .loc "test_case_range.c", 75
    lda #6
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_case_range.c", 33
    lda $20
    ldx $21
    cmp.16 .AX, #0
    beq @case491
@case_skip496:
    lda $22
    ldx $23
    cmp.16 .AX, #1
    bcs @range_upper498
    bra @case_skip497
@range_upper498:
    lda $22
    ldx $23
    cmp.16 .AX, #5
    bcc @case492
    beq @case492
@case_skip497:
    lda $22
    ldx $23
    cmp.16 .AX, #6
    beq @case493
@case_skip499:
    lda $22
    ldx $23
    cmp.16 .AX, #7
    bcs @range_upper501
    bra @default495
@range_upper501:
    lda $22
    ldx $23
    cmp.16 .AX, #10
    bcc @case494
    beq @case494
    bra @default495
@case491:
    .loc "test_case_range.c", 35
    lda #10
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end489
@case492:
    .loc "test_case_range.c", 37
    lda #20
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end489
@case493:
    .loc "test_case_range.c", 39
    lda #30
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end489
@case494:
    .loc "test_case_range.c", 41
    lda #40
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end489
@default495:
    .loc "test_case_range.c", 43
    lda #50
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end489:
    .loc "test_case_range.c", 75
    lda $22
    ldx $23
    cmp.16 .AX, #30
    bne @if_then486
    bra @if_end488
@if_then486:
    lda #23
    ldx #0
    bra @__return
@if_end488:
    .loc "test_case_range.c", 76
    lda #7
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_case_range.c", 33
    lda $20
    ldx $21
    cmp.16 .AX, #0
    beq @case512
@case_skip517:
    lda $22
    ldx $23
    cmp.16 .AX, #1
    bcs @range_upper519
    bra @case_skip518
@range_upper519:
    lda $22
    ldx $23
    cmp.16 .AX, #5
    bcc @case513
    beq @case513
@case_skip518:
    lda $22
    ldx $23
    cmp.16 .AX, #6
    beq @case514
@case_skip520:
    lda $22
    ldx $23
    cmp.16 .AX, #7
    bcs @range_upper522
    bra @default516
@range_upper522:
    lda $22
    ldx $23
    cmp.16 .AX, #10
    bcc @case515
    beq @case515
    bra @default516
@case512:
    .loc "test_case_range.c", 35
    lda #10
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end510
@case513:
    .loc "test_case_range.c", 37
    lda #20
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end510
@case514:
    .loc "test_case_range.c", 39
    lda #30
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end510
@case515:
    .loc "test_case_range.c", 41
    lda #40
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end510
@default516:
    .loc "test_case_range.c", 43
    lda #50
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end510:
    .loc "test_case_range.c", 76
    lda $22
    ldx $23
    cmp.16 .AX, #40
    bne @if_then507
    bra @if_end509
@if_then507:
    lda #24
    ldx #0
    bra @__return
@if_end509:
    .loc "test_case_range.c", 77
    lda #10
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_case_range.c", 33
    lda $20
    ldx $21
    cmp.16 .AX, #0
    beq @case533
@case_skip538:
    lda $22
    ldx $23
    cmp.16 .AX, #1
    bcs @range_upper540
    bra @case_skip539
@range_upper540:
    lda $22
    ldx $23
    cmp.16 .AX, #5
    bcc @case534
    beq @case534
@case_skip539:
    lda $22
    ldx $23
    cmp.16 .AX, #6
    beq @case535
@case_skip541:
    lda $22
    ldx $23
    cmp.16 .AX, #7
    bcs @range_upper543
    bra @default537
@range_upper543:
    lda $22
    ldx $23
    cmp.16 .AX, #10
    bcc @case536
    beq @case536
    bra @default537
@case533:
    .loc "test_case_range.c", 35
    lda #10
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end531
@case534:
    .loc "test_case_range.c", 37
    lda #20
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end531
@case535:
    .loc "test_case_range.c", 39
    lda #30
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end531
@case536:
    .loc "test_case_range.c", 41
    lda #40
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end531
@default537:
    .loc "test_case_range.c", 43
    lda #50
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end531:
    .loc "test_case_range.c", 77
    lda $22
    ldx $23
    cmp.16 .AX, #40
    bne @if_then528
    bra @if_end530
@if_then528:
    lda #25
    ldx #0
    bra @__return
@if_end530:
    .loc "test_case_range.c", 78
    lda #11
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_case_range.c", 33
    lda $20
    ldx $21
    cmp.16 .AX, #0
    beq @case554
@case_skip559:
    lda $22
    ldx $23
    cmp.16 .AX, #1
    bcs @range_upper561
    bra @case_skip560
@range_upper561:
    lda $22
    ldx $23
    cmp.16 .AX, #5
    bcc @case555
    beq @case555
@case_skip560:
    lda $22
    ldx $23
    cmp.16 .AX, #6
    beq @case556
@case_skip562:
    lda $22
    ldx $23
    cmp.16 .AX, #7
    bcs @range_upper564
    bra @default558
@range_upper564:
    lda $22
    ldx $23
    cmp.16 .AX, #10
    bcc @case557
    beq @case557
    bra @default558
@case554:
    .loc "test_case_range.c", 35
    lda #10
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end552
@case555:
    .loc "test_case_range.c", 37
    lda #20
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end552
@case556:
    .loc "test_case_range.c", 39
    lda #30
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end552
@case557:
    .loc "test_case_range.c", 41
    lda #40
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end552
@default558:
    .loc "test_case_range.c", 43
    lda #50
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end552:
    .loc "test_case_range.c", 78
    lda $22
    ldx $23
    cmp.16 .AX, #50
    bne @if_then549
    bra @if_end551
@if_then549:
    lda #26
    ldx #0
    bra @__return
@if_end551:
    .loc "test_case_range.c", 80
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, zeroalloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 0
    endproc


__zp_save_buf:
