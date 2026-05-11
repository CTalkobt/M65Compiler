; errno.s — Error number support
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

; The CBM/MEGA65 kernal status byte lives at $0090.
; This equate lets C code access it as `extern char _errnoc`.
__errnoc = $90
