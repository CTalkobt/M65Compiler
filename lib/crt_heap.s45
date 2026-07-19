; crt_heap.s — CRT module for heap support
;
; This module is pulled in by #pragma cc45 heap.
; It ensures the heap is initialized at startup.

.global _init_heap_crt
.extern _heap_init
.extern _init_features

.segment "code"

_init_heap_crt:
    ; This could be called from __init or _init_features.
    ; For now, we'll just make sure the symbol is here so the linker pulls in malloc.o45.
    ; To actually run at startup, we'd need a way to hook into __init.
    jsr _heap_init
    rts
