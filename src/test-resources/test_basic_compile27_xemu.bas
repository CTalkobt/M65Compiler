1 rem test_basic_compile27_xemu -- basic45 --compile --checks (#580): the run time's gosub
2 rem and return raise basic's errors. results at $c0c0 (49344): 1 = pass. [0] return
3 rem without gosub (12) in its line; [1] gosubs without end: out of memory (16; the rom of
4 rem release 920413 says 25, formula too complex, and has no room for more in the handler); [2] $aa
10 trap 900
20 p=1:n=0:gosub 500
30 return
40 if e=12 and l=30 then poke 49344,1
50 p=2:gosub 600
60 end
500 n=n+1:return
600 n=n+1:gosub 600
900 e=er:l=el:if p=1 then resume next
905 if e=25 then e=16
910 if e=16 then poke 49345,1
920 poke 49346,170
930 end
