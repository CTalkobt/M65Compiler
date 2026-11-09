1 rem test_basic_compile19_xemu -- basic45 --compile (#369): run compiled and interpreted (the
2 rem same results; the values are the interpreter's). drive 8: an empty d81. results at $c0c0
3 rem (49344): 1 = pass. [0] type (the screen's codes); [1] disk (a command, the status); [2]
4 rem lock, unlock, mkdir; [3] set bit, clr bit (?syntax error from bit 8); [4] set def, set
5 rem verify, set disk (?illegal quantity below 8); [5] boot (a bsave'd routine, rreg); [6] the
6 rem errors trapped; [7] $aa
10 trap 900
20 open 2,8,2,"note,s,w":print#2,"hello there":print#2,"line two";:close 2
30 scnclr:type "note":if peek(2048)=8 and peek(2048+80)=12 and peek(2048+160)=32 then poke 49344,1
40 disk "s0:nofile":a=ds:disk:if a=1 then poke 49345,1
50 lock "note":a=ds:unlock "note":b=ds:mkdir "sub":c=ds:if a=0 and b=0 and c=0 then poke 49346,1
60 poke 4200,0:set bit 4200,3:set bit 4200,0:a=peek(4200):clr bit 4200,3:b=peek(4200)
65 poke $50010,0:set bit $50010,7:c=peek($50010):set bit 4200,8:if a=9 and b=1 and c=128 and ee=11 then poke 49347,1
70 set def 9:a=peek(4358):set def 8:b=peek(4358):set verify on:c=ds:set verify off:set disk 8:set disk 7
75 if a=9 and b=8 and c=0 and ee=14 then poke 49348,1
80 for i=0 to 3:read b:poke 4400+i,b:next:bsave "mc",p4400 to p4404:poke 4400,0:poke 4401,0
85 boot "mc":rreg a:if a=42 and peek(4400)=169 then poke 49349,1
90 if e=2 then poke 49350,1
95 poke 49351,170:end
100 data 169,42,96,0
900 e=e+1:ee=er:resume next
