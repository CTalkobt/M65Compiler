; malloc.s — Dynamic memory allocation for cc45 / MEGA65
; Design: Singly-linked sorted free list with coalescing.
;
; Block Header:
;   [Size] (2 bytes) — bit 0 is 1 if allocated, 0 if free.
;   [Next] (2 bytes) — pointer to next free block (only if free).

.global _malloc
.global _free
.global _calloc
.global _realloc
.global _heap_init
.global __heap_ptr
.weak __bss_end
.weak __bss_start
.extern _memcpy
.extern _memset

.segment "bss"
__heap_ptr: .res 2       ; Head of the sorted free list

; Default weak values for BSS if not provided by the compiler
__bss_start = 0
__bss_end = 0

.segment "code"

; -----------------------------------------------------------------------------
; void *malloc(size_t size)
; -----------------------------------------------------------------------------
proc _malloc, W#_p_size
    .var _fp = 0
    ; Save ZP $02-$0B to stack
    lda $02
    pha
    lda $03
    pha
    lda $04
    pha
    lda $05
    pha
    lda $06
    pha
    lda $07
    pha
    lda $08
    pha
    lda $09
    pha
    lda $0a
    pha
    lda $0b
    pha
    ; Total stack shift for params = 10

    ; 1. Adjust size: add 2-byte header, align to 2 bytes, min 4 bytes.
    ldax _p_size+10, sp
    ora #0
    bne @non_zero
    txa
    bne @non_zero
    jmp @fail           ; malloc(0) -> NULL

@non_zero:
    clc
    adc #2              ; + header
    tax
    lda _p_size+11, sp
    adc #0
    tay
    
    txa
    and #$FE            ; Align 2
    tax
    ; (Y:X) is requested block size.
    tya
    bne @size_ok
    cpx #4
    bcs @size_ok
    ldx #4              ; Min size 4
@size_ok:
    stx $08             ; $08/$09 = req_size
    sty $09

    ; 2. Ensure initialized
    lda __heap_ptr
    ora __heap_ptr+1
    bne @search
    jsr _heap_init
    lda __heap_ptr
    ora __heap_ptr+1
    beq @fail

@search:
    stz $04             ; prev = 0
    stz $05
    lda __heap_ptr
    sta $02             ; curr = head
    lda __heap_ptr+1
    sta $03

@loop:
    ldy #0
    lda ($02),y
    tax                 ; block size low
    iny
    lda ($02),y
    tay                 ; block size high (bit 0 is 0)
    
    ; Compare (Y:X) with req ($09:$08)
    tya
    cmp $09
    bcc @next
    bne @found
    txa
    cmp $08
    bcc @next

@found:
    ; Found a block! (Y:X) = current size.
    ; Check for split: remainder >= 4?
    txa
    sec
    sbc $08
    sta $0A             ; remainder low
    tya
    sbc $09
    sta $0B             ; remainder high
    
    ora $0A
    beq @exact_match
    
    lda $0B
    bne @do_split
    lda $0A
    cmp #4
    bcc @exact_match

@do_split:
    ; Split: curr becomes used, remainder becomes a new free block.
    ; New free block at curr + req_size
    lda $02
    clc
    adc $08
    sta $06
    lda $03
    adc $09
    sta $07             ; $06/$07 = new_free
    
    ; new_free->size = remainder
    ldy #0
    lda $0A
    sta ($06),y
    iny
    lda $0B
    sta ($06),y
    
    ; new_free->next = curr->next
    ldy #2
    lda ($02),y
    sta ($06),y
    iny
    lda ($02),y
    sta ($06),y
    
    ; Update link: prev->next = new_free (or __heap_ptr = new_free)
    lda $04
    ora $05
    bne @update_prev
    lda $06
    sta __heap_ptr
    lda $07
    sta __heap_ptr+1
    bra @mark_used
@update_prev:
    ldy #2
    lda $06
    sta ($04),y
    iny
    lda $07
    sta ($04),y
    bra @mark_used

@exact_match:
    ; Remove curr from free list
    ldy #2
    lda ($02),y
    tax
    iny
    lda ($02),y
    tay                 ; Y:X = curr->next
    
    lda $04
    ora $05
    bne @update_prev_exact
    stx __heap_ptr
    sty __heap_ptr+1
    bra @mark_used
@update_prev_exact:
    ldy #2
    txa
    sta ($04),y
    iny
    tya
    sta ($04),y

@mark_used:
    ; curr->size = req_size | 1
    ldy #0
    lda $08
    ora #1
    sta ($02),y
    iny
    lda $09
    sta ($02),y
    
    ; Return curr + 2
    lda $02
    clc
    adc #2
    tax
    lda $03
    adc #0
    bra @restore

@next:
    ; prev = curr
    lda $02
    sta $04
    lda $03
    sta $05
    ; curr = curr->next
    ldy #2
    lda ($02),y
    tax
    iny
    lda ($02),y
    stx $02
    sta $03
    txa
    ora $03
    bne @loop

@fail:
    lda #0
    tax
@restore:
    ; Return AX preserved; PLZ only clobbers Z/flags.
    plz
    stz $0b
    plz
    stz $0a
    plz
    stz $09
    plz
    stz $08
    plz
    stz $07
    plz
    stz $06
    plz
    stz $05
    plz
    stz $04
    plz
    stz $03
    plz
    stz $02
    rts
    endproc

; -----------------------------------------------------------------------------
; void free(void *ptr)
; -----------------------------------------------------------------------------
proc _free, W#_p_ptr
    .var _fp = 0
    ; Save ZP $02-$0B
    lda $02
    pha
    lda $03
    pha
    lda $04
    pha
    lda $05
    pha
    lda $06
    pha
    lda $07
    pha
    lda $08
    pha
    lda $09
    pha
    lda $0a
    pha
    lda $0b
    pha

    ldax _p_ptr+10, sp
    ora #0
    bne @not_null
    txa
    beq @done
@not_null:
    ; curr = ptr - 2
    sec
    sbc #2
    bcs @no_dec
    dex
@no_dec:
    sta $02             ; $02/$03 = block to free (curr)
    stx $03
    
    ; Mark free: curr->size &= ~1
    ldy #0
    lda ($02),y
    and #$FE
    sta ($02),y
    tax                 ; size low
    iny
    lda ($02),y
    tay                 ; size high
    stx $08             ; $08/$09 = size
    sty $09
    
    ; Find insertion point in sorted list (prev < curr < next_p)
    stz $04             ; prev = 0
    stz $05
    lda __heap_ptr
    sta $06             ; next_p = head
    lda __heap_ptr+1
    sta $07

@search:
    lda $06
    ora $07
    beq @insert         ; reached end of list
    
    ; if (curr < next_p) break
    lda $03
    cmp $07
    bcc @insert
    bne @search_next
    lda $02
    cmp $06
    bcc @insert

@search_next:
    lda $06
    sta $04             ; prev = next_p
    lda $07
    sta $05
    ldy #2
    lda ($06),y
    tax
    iny
    lda ($06),y
    stx $06             ; next_p = next_p->next
    sta $07
    bra @search

@insert:
    ; Insert curr between prev and next_p
    ldy #2
    lda $06
    sta ($02),y         ; curr->next = next_p
    iny
    lda $07
    sta ($02),y
    
    lda $04
    ora $05
    bne @link_prev
    lda $02
    sta __heap_ptr      ; head = curr
    lda $03
    sta __heap_ptr+1
    bra @coalesce_next
@link_prev:
    ldy #2
    lda $02
    sta ($04),y         ; prev->next = curr
    iny
    lda $03
    sta ($04),y

@coalesce_next:
    ; if (curr + curr->size == next_p)
    lda $06
    ora $07
    beq @coalesce_prev  ; next_p is NULL
    
    lda $02
    clc
    adc $08
    cmp $06
    bne @coalesce_prev
    lda $03
    adc $09
    cmp $07
    bne @coalesce_prev
    
    ; curr->size += next_p->size
    ldy #0
    lda ($06),y
    clc
    adc $08
    sta ($02),y
    iny
    lda ($06),y
    adc $09
    sta ($02),y
    ; curr->next = next_p->next
    ldy #2
    lda ($06),y
    sta ($02),y
    iny
    lda ($06),y
    sta ($02),y
    ; Update local size for prev-coalesce
    ldy #0
    lda ($02),y
    sta $08
    iny
    lda ($02),y
    sta $09
    
@coalesce_prev:
    ; if (prev + prev->size == curr)
    lda $04
    ora $05
    beq @done
    
    ldy #0
    lda ($04),y
    tax                 ; prev->size low
    iny
    lda ($04),y
    tay                 ; prev->size high
    
    txa                 ; low size
    clc
    adc $04             ; prev address low
    cmp $02             ; curr address low
    bne @done
    tya                 ; high size
    adc $05             ; prev address high
    cmp $07             ; curr address high (was error, should be 17? no, ZP $03 is curr high)
    ; wait, curr address high is $03
    cmp $03
    bne @done
    
    ; prev->size += curr->size
    ldy #0
    lda ($02),y
    clc
    adc ($04),y
    sta ($04),y
    iny
    lda ($02),y
    adc ($04),y
    sta ($04),y
    
    ; prev->next = curr->next
    ldy #2
    lda ($02),y
    sta ($04),y
    iny
    lda ($02),y
    sta ($04),y

@done:
    plz
    stz $0b
    plz
    stz $0a
    plz
    stz $09
    plz
    stz $08
    plz
    stz $07
    plz
    stz $06
    plz
    stz $05
    plz
    stz $04
    plz
    stz $03
    plz
    stz $02
    rts
    endproc

; -----------------------------------------------------------------------------
; void *calloc(size_t nmemb, size_t size)
; -----------------------------------------------------------------------------
proc _calloc, W#_p_nmemb, W#_p_size
    .var _fp = 0
    ldax _p_nmemb, sp
    stax $02
    ldax _p_size, sp
    mul.16 .ax, $02
    stax $0C            ; total
    phx
    pha
    jsr _malloc
    stax $02            ; result
    ora $03
    beq @fail
    
    ; memset(result, 0, total)
    lda $0D
    pha
    lda $0C
    pha                 ; n
    phw #0              ; c
    lda $03
    pha
    lda $02
    pha                 ; ptr
    jsr _memset
    ldax $02
    rtn #0
@fail:
    lda #0
    tax
    rtn #0
    endproc

; -----------------------------------------------------------------------------
; void *realloc(void *ptr, size_t size)
; -----------------------------------------------------------------------------
proc _realloc, W#_p_ptr, W#_p_size
    .var _fp = 0
    ; 1. If ptr is NULL, realloc is malloc
    ldax _p_ptr, sp
    ora #0
    bne @not_null
    txa
    bne @not_null
    ldax _p_size, sp
    phx
    pha
    jsr _malloc
    rtn #0

@not_null:
    ; 2. If size is 0, realloc is free
    ldax _p_size, sp
    ora #0
    bne @do_realloc
    txa
    bne @do_realloc
    ldax _p_ptr, sp
    phx
    pha
    jsr _free
    lda #0
    tax
    rtn #0

@do_realloc:
    ; 3. Normal realloc
    ldax _p_size, sp
    phx
    pha
    jsr _malloc
    stax $06            ; new_ptr (use $06/$07, $02 is clobbered by ldax)
    ora $07
    beq @fail
    
    ; memcpy(new_ptr, old_ptr, min(old_size, new_size))
    ldax _p_ptr, sp
    sec
    sbc #2
    bcs @no_dec
    dex
@no_dec:
    sta $04
    stx $05
    ldy #0
    lda ($04),y
    and #$FE
    sec
    sbc #2
    tax
    iny
    lda ($04),y
    sbc #0
    tay                 ; Y:X = old_data_size
    
    ; count = min(old_data_size, new_size)
    lda _p_size+1, sp
    sta $0A
    tya
    cmp $0A
    bcc @use_old
    bne @use_new
    lda _p_size, sp
    sta $0A
    txa
    cmp $0A
    bcc @use_old
@use_new:
    ldax _p_size, sp
    bra @do_copy
@use_old:
    txa
    ; high byte already in Y
@do_copy:
    phy
    phx                 ; count (Y:X)
    ldax _p_ptr, sp
    phx
    pha                 ; src (.AX)
    lda $07
    pha
    lda $06
    pha                 ; dest ($06/$07)
    jsr _memcpy
    
    ldax _p_ptr, sp
    phx
    pha
    jsr _free
    
    ldax $06
    rtn #0
@fail:
    lda #0
    tax
    rtn #0
    endproc

; -----------------------------------------------------------------------------
; Internal: heap initialization
; -----------------------------------------------------------------------------
.weak __heap_start
.weak __heap_end

_heap_init:
    lda #<__heap_start
    sta $02
    lda #>__heap_start
    sta $03
    
    lda $02
    ora $03
    bne @check_end
    lda #<__bss_end
    sta $02
    lda #>__bss_end
    sta $03
    
@check_end:
    lda $02
    and #1
    beq @aligned
    inc $02
    bne @aligned
    inc $03
@aligned:

    lda #<__heap_end
    sta $04
    lda #>__heap_end
    sta $05
    
    lda $04
    ora $05
    bne @calc_size
    lda #$00
    sta $04
    lda #$D0
    sta $05
    
@calc_size:
    lda $04
    sec
    sbc $02
    tax             ; size low
    lda $05
    sbc $03
    tay             ; size high
    
    ; Ensure size >= 4
    tya
    bne @init_ok
    cpx #4
    bcs @init_ok
    rts             ; Fail to init
    
@init_ok:
    ; head = __heap_start
    lda $02
    sta __heap_ptr
    lda $03
    sta __heap_ptr+1
    
    ; head->size = (Y:X)
    ldy #0
    txa
    sta ($02),y
    iny
    tya             ; Y was high size
    sta ($02),y
    
    ; head->next = 0
    lda #0
    ldy #2
    sta ($02),y
    iny
    sta ($02),y
    
    rts

; Default weak values
__heap_start = 0
__heap_end = 0
