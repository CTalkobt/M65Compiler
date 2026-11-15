1 rem test_basic_compile25_xemu -- basic45 --compile (#369): a for loop whose variable has a
2 rem loop on basic's stack already (left by goto) drops that loop and the ones above it, a
3 rem counted loop (no entry of its own) too. results at $c0c0 (49344): 1 = pass. [0] the
4 rem outer loop's next: ?next without for; [1] an integer variable's; [2] the loops above
5 rem the dropped one go too; [3] not past a gosub: the caller's loop stays; [4] $aa
10 trap 900
20 e=0:n=0:for i=1 to 3
21 if i=1 then goto 23
22 next i:goto 26
23 for j=1 to 2:for i=5 to 6:n=n+1:next i:next j
24 next i
26 if e=10 and l=24 and n=2 then poke 49344,1
30 e=0:n=0:for i%=1 to 3
31 if i%=1 then goto 33
32 next i%:goto 36
33 for i%=5 to 6:n=n+1:next i%
34 next i%
36 if e=10 and l=34 and n=2 then poke 49345,1
40 e=0:n=0:for a=1 to 3:for b=1 to 3
41 if b=1 then goto 43
42 next b:next a:goto 46
43 for a=7 to 8:n=n+1:next a
44 next b
46 if e=10 and l=44 and n=2 then poke 49346,1
50 e=0:n=0:for k=1 to 2:gosub 500:next k
51 if e=0 and n=2 and k=8 then poke 49347,1
60 poke 49348,170
70 end
500 for k=5 to 6:n=n+1:next k:return
900 e=er:l=el:resume next
