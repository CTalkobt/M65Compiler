; malloc.s — Dynamic memory allocation for cc45 / MEGA65 (ZP calling convention)
; Design: Singly-linked sorted free list with coalescing.
;
; Block Header:
;   [Size] (2 bytes) — bit 0 is 1 if allocated, 0 if free.
;   [Next] (2 bytes) — pointer to next free block (only if free).
;
; Internal ZP usage: $03-$0B (after reading params from ZP block)

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
;   size in $03/$04
; -----------------------------------------------------------------------------
proc _malloc
    .zp_uses $03, $04
    .zp_clobbers $03, $04, $05, $06, $07, $08, $09, $0A
    .zp_release $03, $04
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z

    ; Read size param and move to $09/$0A for working storage
    lda $03
    sta $09
    lda $04
    sta $0A

    ; 1. Check for malloc(0)
    lda $09
    ora $0A
    bne @non_zero
    jmp @fail

@non_zero:
    ; Adjust: add 2-byte header, align to 2 bytes, min 4
    lda $09
    clc
    adc #2
    tax
    lda $0A
    adc #0
    tay
    txa
    and #$FE
    tax
    tya
    bne @size_ok
    cpx #4
    bcs @size_ok
    ldx #4
@size_ok:
    stx $09             ; Req size low
    sty $0A             ; Req size high

    ; 2. Ensure initialized
    lda __heap_ptr
    ora __heap_ptr+1
    bne @search
    jsr _heap_init
    lda __heap_ptr
    ora __heap_ptr+1
    beq @fail

@search:
    stz $05             ; prev = 0
    stz $06
    lda __heap_ptr
    sta $03             ; curr = head
    lda __heap_ptr+1
    sta $04

@loop:
    ldy #0
    lda ($03),y
    tax                 ; block size low
    iny
    lda ($03),y
    tay                 ; block size high

    ; Compare (Y:X) with req ($0A:$09)
    tya
    cmp $0A
    bcc @next
    bne @found
    txa
    cmp $09
    bcc @next

@found:
    ; Found! (Y:X) = current size, ($0A:$09) = req size
    ; Check for split: remainder >= 4?
    txa
    sec
    sbc $09
    sta $0B             ; remainder low (using extra ZP)
    tya
    sbc $0A
    pha                 ; remainder high (on stack temporarily)

    ora $0B
    beq @exact_match_pop

    pla                 ; remainder high
    pha
    bne @do_split_pop
    lda $0B
    cmp #4
    bcc @exact_match_pop

@do_split_pop:
    pla                 ; remainder high -> use in split
    sta $02             ; stash remainder high in scratch

    ; Split: new free block at curr + req_size
    lda $03
    clc
    adc $09
    sta $07
    lda $04
    adc $0A
    sta $08

    ; new_free->size = remainder
    ldy #0
    lda $0B
    sta ($07),y
    iny
    lda $02             ; remainder high
    sta ($07),y

    ; new_free->next = curr->next
    ldy #2
    lda ($03),y
    sta ($07),y
    iny
    lda ($03),y
    sta ($07),y

    ; Update link: prev->next = new_free (or __heap_ptr = new_free)
    lda $05
    ora $06
    bne @update_prev
    lda $07
    sta __heap_ptr
    lda $08
    sta __heap_ptr+1
    bra @mark_used
@update_prev:
    ldy #2
    lda $07
    sta ($05),y
    iny
    lda $08
    sta ($05),y
    bra @mark_used

@exact_match_pop:
    pla                 ; discard remainder high from stack
    ; Remove curr from free list
    ldy #2
    lda ($03),y
    tax
    iny
    lda ($03),y
    tay                 ; Y:X = curr->next

    lda $05
    ora $06
    bne @update_prev_exact
    stx __heap_ptr
    sty __heap_ptr+1
    bra @mark_used
@update_prev_exact:
    ldy #2
    txa
    sta ($05),y
    iny
    tya
    sta ($05),y

@mark_used:
    ; curr->size = req_size | 1
    ldy #0
    lda $09
    ora #1
    sta ($03),y
    iny
    lda $0A
    sta ($03),y

    ; Return curr + 2
    lda $03
    clc
    adc #2
    tax
    lda $04
    adc #0
    rts

@next:
    ; prev = curr
    lda $03
    sta $05
    lda $04
    sta $06
    ; curr = curr->next
    ldy #2
    lda ($03),y
    tax
    iny
    lda ($03),y
    stx $03
    sta $04
    ora $03
    bne @loop

@fail:
    lda #0
    tax
    rts
    endproc

; -----------------------------------------------------------------------------
; void free(void *ptr)
;   ptr in $03/$04
; -----------------------------------------------------------------------------
proc _free
    .zp_uses $03, $04
    .zp_clobbers $03, $04, $05, $06, $07, $08, $09, $0A
    .zp_release $03, $04
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z

    lda $03
    ora $04
    beq @done

    ; curr = ptr - 2
    lda $03
    sec
    sbc #2
    sta $03
    lda $04
    sbc #0
    sta $04

    ; Mark free: curr->size &= ~1
    ldy #0
    lda ($03),y
    and #$FE
    sta ($03),y
    tax                 ; block size low
    iny
    lda ($03),y
    tay                 ; block size high
    stx $09             ; block size low
    sty $0A             ; block size high

    ; Find insertion point in sorted list
    stz $05             ; prev = 0
    stz $06
    lda __heap_ptr
    sta $07             ; next_p = head
    lda __heap_ptr+1
    sta $08

@search:
    lda $07
    ora $08
    beq @insert

    ; if (curr < next_p) break
    lda $04
    cmp $08
    bcc @insert
    bne @search_next
    lda $03
    cmp $07
    bcc @insert

@search_next:
    lda $07
    sta $05             ; prev = next_p
    lda $08
    sta $06
    ldy #2
    lda ($07),y
    tax
    iny
    lda ($07),y
    stx $07
    sta $08
    bra @search

@insert:
    ; curr->next = next_p
    ldy #2
    lda $07
    sta ($03),y
    iny
    lda $08
    sta ($03),y

    lda $05
    ora $06
    bne @link_prev
    lda $03
    sta __heap_ptr
    lda $04
    sta __heap_ptr+1
    bra @coalesce_next
@link_prev:
    ldy #2
    lda $03
    sta ($05),y
    iny
    lda $04
    sta ($05),y

@coalesce_next:
    ; if (curr + curr->size == next_p)
    lda $07
    ora $08
    beq @coalesce_prev

    lda $03
    clc
    adc $09
    cmp $07
    bne @coalesce_prev
    lda $04
    adc $0A
    cmp $08
    bne @coalesce_prev

    ; curr->size += next_p->size
    ldy #0
    lda ($07),y
    clc
    adc $09
    sta ($03),y
    iny
    lda ($07),y
    adc $0A
    sta ($03),y
    ; curr->next = next_p->next
    ldy #2
    lda ($07),y
    sta ($03),y
    iny
    lda ($07),y
    sta ($03),y
    ; Update local size
    ldy #0
    lda ($03),y
    sta $09
    iny
    lda ($03),y
    sta $0A

@coalesce_prev:
    ; if (prev + prev->size == curr)
    lda $05
    ora $06
    beq @done

    ldy #0
    lda ($05),y
    tax                 ; prev->size low
    iny
    lda ($05),y
    tay                 ; prev->size high

    txa
    clc
    adc $05
    cmp $03
    bne @done
    tya
    adc $06
    cmp $04
    bne @done

    ; prev->size += curr->size
    ldy #0
    lda ($03),y
    clc
    adc ($05),y
    sta ($05),y
    iny
    lda ($03),y
    adc ($05),y
    sta ($05),y

    ; prev->next = curr->next
    ldy #2
    lda ($03),y
    sta ($05),y
    iny
    lda ($03),y
    sta ($05),y

@done:
    rts
    endproc

; -----------------------------------------------------------------------------
; void *calloc(size_t nmemb, size_t size)
;   nmemb=$03/$04, size=$05/$06
; -----------------------------------------------------------------------------
proc _calloc
    .zp_uses $03, $04, $05, $06
    .zp_clobbers $03, $04, $05, $06, $07, $08, $09, $0A
    .zp_release $03, $04, $05, $06
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z

    ; total = nmemb * size
    ldax $03
    mul.16 .ax, $05
    stax $09            ; save total in $09/$0A

    ; malloc(total)
    sta $03
    stx $04
    jsr _malloc
    sta $07             ; save result lo
    stx $08             ; save result hi
    ora $08
    beq @fail

    ; memset(result, 0, total)
    lda $07
    sta $03
    lda $08
    sta $04             ; dest = result
    stz $05             ; c = 0
    stz $06
    lda $09
    sta $07
    lda $0A
    sta $08             ; n = total
    jsr _memset

    lda $03             ; memset returns dest
    ldx $04
    rts
@fail:
    lda #0
    tax
    rts
    endproc

; -----------------------------------------------------------------------------
; void *realloc(void *ptr, size_t size)
;   ptr=$03/$04, size=$05/$06
; -----------------------------------------------------------------------------
proc _realloc
    .zp_uses $03, $04, $05, $06
    .zp_clobbers $03, $04, $05, $06, $07, $08, $09, $0A
    .zp_release $03, $04, $05, $06
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z

    ; Save params
    lda $03
    sta $09             ; ptr lo
    lda $04
    sta $0A             ; ptr hi
    lda $05
    sta $07             ; size lo
    lda $06
    sta $08             ; size hi

    ; If ptr is NULL, realloc is malloc
    lda $09
    ora $0A
    bne @not_null
    lda $07
    sta $03
    lda $08
    sta $04
    jmp _malloc

@not_null:
    ; If size is 0, realloc is free
    lda $07
    ora $08
    bne @do_realloc
    lda $09
    sta $03
    lda $0A
    sta $04
    jmp _free

@do_realloc:
    ; malloc(size)
    lda $07
    sta $03
    lda $08
    sta $04
    jsr _malloc
    sta $05             ; new_ptr lo (reuse $05/$06 for new ptr)
    stx $06
    ora $06
    beq @fail

    ; Get old block size: read header at (ptr - 2)
    lda $09
    sec
    sbc #2
    sta $03
    lda $0A
    sbc #0
    sta $04
    ldy #0
    lda ($03),y
    and #$FE
    sec
    sbc #2              ; old_data_size low
    tax
    iny
    lda ($03),y
    sbc #0              ; old_data_size high
    tay                 ; Y:X = old_data_size

    ; count = min(old_data_size, new_size)
    tya
    cmp $08
    bcc @use_old
    bne @use_new
    txa
    cmp $07
    bcc @use_old
@use_new:
    lda $07
    sta $03             ; count lo (reuse $03/$04 for count... wait)
    lda $08
    bra @do_copy_setup
@use_old:
    txa
    sta $03
    tya
@do_copy_setup:
    sta $04             ; count hi in $04, count lo in $03
    ; Now: new_ptr=$05/$06, old_ptr=$09/$0A, count=$03/$04
    ; Set up memcpy(new_ptr, old_ptr, count): dest=$03/$04, src=$05/$06, n=$07/$08
    lda $03
    pha                 ; save count lo
    lda $04
    pha                 ; save count hi
    lda $05
    sta $03             ; dest = new_ptr
    lda $06
    sta $04
    lda $09
    sta $05             ; src = old_ptr
    lda $0A
    sta $06
    pla
    sta $08             ; n hi
    pla
    sta $07             ; n lo
    ; Save new_ptr before memcpy clobbers $03/$04
    lda $03
    pha
    lda $04
    pha
    jsr _memcpy

    ; free(old_ptr)
    lda $09
    sta $03
    lda $0A
    sta $04
    jsr _free

    ; Return new_ptr
    pla
    tax                 ; hi
    pla                 ; lo
    rts
@fail:
    lda #0
    tax
    rts
    endproc

; -----------------------------------------------------------------------------
; Internal: heap initialization
; -----------------------------------------------------------------------------
.weak __heap_start
.weak __heap_end

_heap_init:
    lda #<__heap_start
    sta $03
    lda #>__heap_start
    sta $04

    lda $03
    ora $04
    bne @check_end
    lda #<__bss_end
    sta $03
    lda #>__bss_end
    sta $04

@check_end:
    lda $03
    and #1
    beq @aligned
    inc $03
    bne @aligned
    inc $04
@aligned:

    lda #<__heap_end
    sta $05
    lda #>__heap_end
    sta $06

    lda $05
    ora $06
    bne @calc_size
    lda #$00
    sta $05
    lda #$D0
    sta $06

@calc_size:
    lda $05
    sec
    sbc $03
    tax             ; size low
    lda $06
    sbc $04
    tay             ; size high

    ; Ensure size >= 4
    tya
    bne @init_ok
    cpx #4
    bcs @init_ok
    rts             ; Fail to init

@init_ok:
    ; head = __heap_start
    lda $03
    sta __heap_ptr
    lda $04
    sta __heap_ptr+1

    ; head->size = (Y:X)
    ldy #0
    txa
    sta ($03),y
    iny
    tya
    sta ($03),y

    ; head->next = 0
    lda #0
    ldy #2
    sta ($03),y
    iny
    sta ($03),y

    rts

; Default weak values
__heap_start = 0
__heap_end = 0
