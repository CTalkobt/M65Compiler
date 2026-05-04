; crt_heap_zp.s — CRT module for heap support (ZP calling convention)
;
; Ensures the heap is initialized at startup.

.global _init_heap_crt
.extern _heap_init
.extern _init_features

.segment "code"

_init_heap_crt:
    jsr _heap_init
    rts
