; assert_fail.s — Runtime assertion failure handler
;
; Called by assert() macro when an assertion fails.
; Prints an error message to the console and halts execution.
;
; Parameters (stack convention):
;   - expr: pointer to expression string
;   - file: pointer to filename string
;   - line: line number (16-bit int)

.cpu _45gs02

.global _assert_fail

_assert_fail:
    proc

    ; Parameters: _p_expr (pointer), _p_file (pointer), _p_line (int)
    ; Stack layout:
    ;   [SP+6]  _p_expr (lo/hi)
    ;   [SP+4]  _p_file (lo/hi)
    ;   [SP+2]  _p_line (lo/hi)
    ;   [SP+0]  Return address (lo/hi)

    ; Build printf call: printf("Assertion failed: %s (file %s, line %d)\n",
    ;                           expr, file, line)

    ; Arguments pushed right-to-left for stack convention:
    tsx

    ; Push line number (16-bit)
    lda $0102, x
    pha
    lda $0103, x
    pha

    ; Push file pointer (16-bit)
    lda $0104, x
    pha
    lda $0105, x
    pha

    ; Push expr pointer (16-bit)
    lda $0106, x
    pha
    lda $0107, x
    pha

    ; Push format string pointer
    phw #_assert_format_str

    ; Call printf
    jsr _printf

    ; If printf returns, halt execution
    brk

    endproc

; Format string for assertion failures
.segment "data"
_assert_format_str:
    .ascii "Assertion failed: "
    .ascii "%s"
    .ascii " (file %s, line %d)"
    .ascii $0A
    .byte $00
