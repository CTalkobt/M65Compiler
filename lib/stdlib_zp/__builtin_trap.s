; __builtin_trap — unconditional program abort
.global ___builtin_trap
.extern __abort
.segment "code"

___builtin_trap:
    jmp __abort
