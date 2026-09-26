; clock.s — Return jiffy clock ticks (16-bit, ZP calling convention)
;
; clock_t clock(void);
;   Returns low 16 bits of the 24-bit KERNAL jiffy clock.

.global _clock

.segment "code"

proc _clock
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z

    jsr $FFDE       ; RDTIM: A=hi, X=mid, Y=lo
    tya             ; A = lo byte
                    ; X = mid byte
    rts
    endproc
