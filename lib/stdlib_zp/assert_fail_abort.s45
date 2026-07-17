; assert_fail_abort.s — Simple halt function for assert_msg (ZP convention)
; Called after printf has printed assertion message.
; No return: halts execution with BRK.

.global _assert_fail_abort

_assert_fail_abort:
    proc
    brk
    .zp_clobbers
    .reg_clobbers
    .flag_clobbers
    endproc
