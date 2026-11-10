1 rem test_basic_compile_tron_xemu -- basic45 --compile (#369): tron / troff print "[line]"
2 rem before each statement while the trace is on, as the interpreter; mem's values taken.
3 rem the screen's first 60 bytes copied to $c0c0 (49344); $aa after them
10 scnclr:mem 0,0
20 tron:a=1:if a=1 then b=2
30 c=3::d=4:if a=2 then b=3:else c=5
40 gosub 1005:troff:e=5
50 for i=0 to 59:poke 49344+i,peek(2048+i):next:poke 49404,170:end
1005 x=1:return
