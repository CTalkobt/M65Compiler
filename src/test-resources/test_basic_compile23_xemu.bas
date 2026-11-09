1 rem test_basic_compile23_xemu -- basic45 --compile (#369): run compiled and interpreted (the
2 rem same results; the values are the interpreter's). boot sys: drive 8's home sector (track 1,
3 rem sector 0) loaded to $0400 and called. results at $c0c0 (49344): [0] what the sector's code
4 rem stored at $0600 ($2a: jmp $0403 / lda #$2a / sta $0600 / rts; 0 on a disk without it);
5 rem [1] the error (36 ?bad disk; 0); [2] 1: the program goes on; [3] $aa
10 poke 1536,0:e=0:trap 100
20 boot sys
30 poke 49344,peek(1536):poke 49345,e:poke 49346,1:poke 49347,170:end
100 e=er:resume next
