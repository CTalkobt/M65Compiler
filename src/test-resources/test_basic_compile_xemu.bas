1 rem test_basic_compile_xemu -- basic45 --compile (#369): run compiled and interpreted (the
2 rem same results). results at $c0c0 (49344): 1 = pass
3 rem [0] float arithmetic; [1] % variables; [2] functions; [3] and or not, comparisons
4 rem [4] strings; [5] for step, nested next j,i; [6] goto out of a loop, the loop again
5 rem [7] on goto, on gosub; [8] if else, nested if, begin/bend; [9] gosub, return drops for
6 rem [10] print: numbers, comma, tab, spc on the screen; [11] str$ of numbers; [12] $aa
10 a=7:b=2:if a/b=3.5 and a*b=14 and a^b=49 and a-b=5 and -a=-7 and a+b*2=11 then poke 49344,1
20 a%=-3:b%=a%*5+1:c%=1.5:d%=-1.5
25 if b%=-14 and c%=1 and d%=-2 and int(-2.5)=-3 and a%/2=-1.5 and b%+a%=-17 then poke 49345,1
30 if sqr(16)=4 and abs(-4)=4 and sgn(-2)=-1 and sgn(0)=0 and sgn(3)=1 and int(2.5)=2 then 35
32 goto 40
35 if str$(atn(1)*4)=" 3.1415927" and log(1)=0 and exp(0)=1 and sin(0)=0 and cos(0)>.99 then poke 49346,1
40 if (5 and 3)=1 and (5 or 3)=7 and not 5=-6 and (3>2)=-1 and (2>3)=0 and "a"<"b" and "b">="b" then poke 49347,1
50 s$="hello world":t$=left$(s$,5)+"/"+right$(s$,5)+"/"+mid$(s$,7,3)+"/"+mid$(s$,7)
55 if t$="hello/world/wor/world" and len(s$)=11 and asc(s$)=72 and chr$(65)="a" and str$(-1.25)="-1.25" then poke 49348,1
60 n=0:for i=10 to 1 step -3:n=n+i:next:m=0:for i=1 to 3:for j=1 to 2:m=m+i*j:next j,i
65 if n=22 and i=4 and j=3 and m=18 then poke 49349,1
70 n=0:for k=1 to 3:for i=1 to 10:if i=4 then 80
75 n=n+1:next i
80 next k:if n=9 and i=4 and k=4 then poke 49350,1
90 c=0:for k=1 to 3:on k goto 92,94,96
92 c=c+1:goto 98
94 c=c+10:goto 98
96 c=c+100
98 next k:d=0:for k=1 to 3:on k gosub 500,510,520:next:on 4 goto 99:if c=111 and d=321 then poke 49351,1
99 a=0:if 1 then if 0 then a=1:else a=2
100 b=0:if 0 then b=1:else b=2
105 c=0:if 0 then begin
106 c=1
107 c=c:bend:else c=3
108 e=0:if 1 then begin:e=e+1
109 e=e+1:bend:else e=10
110 if a=2 and b=2 and c=3 and e=2 then poke 49352,1
120 g=0:gosub 600:for i=1 to 3:gosub 610:next:if g=1 and x=3 then poke 49353,1
130 print chr$(147);"ab";12;"c":print 1,2:print "x";tab(5);"y";spc(2);"z"
135 s=2048:if peek(s)=1 and peek(s+3)=49 and peek(s+4)=50 and peek(s+6)=3 and peek(s+81)=49 and peek(s+91)=50 then 137
136 goto 140
137 if peek(s+160)=24 and peek(s+165)=25 and peek(s+168)=26 then poke 49354,1
140 if str$(1e10)=" 1e10" and str$(123456789)=" 123456789" and str$(1/3)=" 0.33333333" and str$(-.001)="-0.001" then poke 49355,1
150 poke 49356,170:end
500 d=d+1:return
510 d=d+20:return
520 d=d+300:return
600 g=g+1:return
610 for x=1 to 3:if x=3 then return
615 next x:g=-100:return
