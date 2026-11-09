1 rem test_basic_compile21_xemu -- basic45 --compile (#369): run compiled and interpreted (the
2 rem same results; the values are the interpreter's). drive 8: an empty d81. results at $c0c0
3 rem (49344): 1 = pass. [0] sprite save, sprite load (2 pages at $0600); [1] key save, key
4 rem load; [2] mount of a missing image (?file not found); [3] chdir on the sd card (missing);
5 rem [4] pointer (an address: the compiled program's own layout); [5] the errors trapped; [6] 1; [7] $aa
10 trap 900
20 for i=0 to 511:poke 1536+i,i and 255:next:sprite save "spr":for i=0 to 511:poke 1536+i,0:next
25 sprite load "spr":if peek(1536)=0 and peek(1791)=255 and peek(1836)=44 and peek(2047)=255 then poke 49344,1
30 key 1,"zz":key save "k":key 1,"q":key load "k":if peek(4096)=2 then poke 49345,1
40 e=0:mount "nosuchfile.d81":if e=1 and ee=4 then poke 49346,1
50 chdir "nodir",u12:if e=2 and ee=4 then poke 49347,1
60 a%=1234:p=pointer(a%):if p>0 and p<65536 then poke 49348,1
70 if e=2 then poke 49349,1
80 poke 49350,1:poke 49351,170:end
900 e=e+1:ee=er:resume next
