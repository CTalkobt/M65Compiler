; errno.s — Error number support (ZP calling convention)
;
; Provides:
;   _errno   — 16-bit global error number (int), in BSS
;   __errnoc — CBM kernal status byte at $0090 (read-only alias)

.global _errno
.global __errnoc

.segment "bss"

_errno:
    .space 2

.segment "data"

__errnoc = $90
