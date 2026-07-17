; __builtin_aliases.s — Aliases for GCC builtins to standard library functions
.global ___builtin_malloc
.global ___builtin_free
.global ___builtin_strncpy
.global ___builtin_strchr
.global ___builtin_sprintf
.global ___builtin_vprintf
.global ___builtin_printf
.global ___builtin_memset
.global ___builtin_setjmp
.global ___builtin_longjmp
.global ___builtin_abort
.global ___builtin_exit
.global ___builtin_apply
.global ___builtin_apply_args
.global ___builtin_shuffle
.global ___builtin_choose_expr
.global ___alignof__

.extern _malloc
.extern _free
.extern _strncpy
.extern _strchr
.extern _sprintf
.extern _vprintf
.extern _printf
.extern _memset
.extern _setjmp
.extern _longjmp
.extern _abort
.extern _exit

.segment "code"

___builtin_malloc:  jmp _malloc
___builtin_free:    jmp _free
___builtin_strncpy: jmp _strncpy
___builtin_strchr:  jmp _strchr
___builtin_sprintf: jmp _sprintf
___builtin_vprintf: jmp _vprintf
___builtin_printf:  jmp _printf
___builtin_memset:  jmp _memset
___builtin_setjmp:  jmp _setjmp
___builtin_longjmp: jmp _longjmp
___builtin_abort:   jmp _abort
___builtin_exit:    jmp _exit

___builtin_apply:   ; void* __builtin_apply(void (*fn)(), void *args, size_t size)
___builtin_apply_args: ; void* __builtin_apply_args()
___builtin_shuffle: ; void __builtin_shuffle(...)
___builtin_choose_expr: ; should be handled by parser, but just in case
    lda #0
    ldx #0
    rts

___alignof__:
    lda #1
    ldx #0
    rts

