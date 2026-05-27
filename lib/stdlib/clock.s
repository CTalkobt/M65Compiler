; clock.s — Return jiffy clock ticks (16-bit)
;
; clock_t clock(void);
;   Returns low 16 bits of the 24-bit KERNAL jiffy clock.
;   KERNAL RDTIM ($FFDE) returns A=hi, X=mid, Y=lo.

.global _clock

.segment "code"

proc _clock
    jsr $FFDE       ; RDTIM: A=hi, X=mid, Y=lo
    tya             ; A = lo byte
                    ; X = mid byte (already set)
    rtn #0
    endproc
