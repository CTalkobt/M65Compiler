1 rem test_basic_compile24_xemu -- basic45 --compile (#369): forms that were compile errors.
2 rem results at $c0c0 (49344): 1 = pass. [0] for with an integer variable: steps up and down,
3 rem a step in a variable, limit and start made integers, nested, a wrap at 32767; [1] its
4 rem ?illegal quantity for a limit or start out of range; [2] def fn of a string or integer
5 rem function or parameter: ?type mismatch, nothing defined; [3] dim of a scalar keeps its
6 rem value; [4] input #1,a (a space: the plain input, then ?syntax error; an empty line: no
7 rem error); [5] trap with a computed line; [6] collision with a computed line; [7] $aa
10 trap 900
20 s=0:for i%=1 to 5:s=s+i%:next:ok=-(s=15 and i%=6)
21 s=0:for i%=10 to 1 step -3:s=s+i%:next i%:if s<>22 or i%<>-2 then ok=0
22 for i%=1 to 2.6:next:if i%<>3 then ok=0
23 for i%=1.5 to 3:s=i%:i%=9:next:if s<>1 or i%<>10 then ok=0
24 n=0:for i%=1 to 3:for j%=i% to 3:n=n+1:next j%,i%:if n<>6 then ok=0
25 s%=2:n=0:for i%=0 to 9 step s%:n=n+1:next:if n<>5 or i%<>10 then ok=0
26 s%=-4:n=0:for i%=9 to 0 step s%:n=n+1:next:if n<>3 or i%<>-3 then ok=0
27 n=0:for i%=5 to 1:n=n+1:next:if n<>1 or i%<>6 then ok=0
28 n=0:for a=1 to 2:for i%=1 to 2:n=n+1:next i%:next a:if n<>4 then ok=0
29 n=0:for i%=32765 to 32767:n=n+1:next:if n<>3 or i%<>-32768 then ok=0
30 n=0:for i%=-2 to 2:n=n+i%*i%:next:if n<>10 then ok=0
31 gosub 500:if n<>3 or e<>0 then ok=0
32 poke 49344,ok
40 gosub 600
41 if e1=14 and e=14 then poke 49345,1
50 e=0:def fn a$(x)=x+1:e1=e:e=0:def fn b(x%)=x%*2:e2=e:e=0:r=fn b(5):e3=e
51 e=0:r$=fn a$(1):e4=e:e=0:def fn c(x$)=1:e5=e:e=0:def fn d%(x)=x+1:e6=e:e=0:r=fn d%(2)
52 def fn g(x)=x*3:if e1=22 and e2=22 and e3=27 and e4=22 and e5=22 and e6=22 and e=22 and r=0 and fn g(4)=12 then poke 49346,1
60 e=0:a=5:b%=6:c$="xy":dim a,b%,c$,d(3),z:d(3)=4:dim a
61 if a=5 and b%=6 and c$="xy" and d(3)=4 and z=0 and e=0 then poke 49347,1
70 e=0:a=7:poke 688,53:poke 689,13:poke 208,2:input #1,a:a=8
71 e1=e:a1=a:e=0:a=7:poke 688,13:poke 208,1:input #1,a:a=9
72 e2=e:e=0:a$="q":poke 688,65:poke 689,13:poke 208,2:line input #1,a$:a$="zz"
73 if e1=11 and a1=8 and e2=0 and a=9 and e=11 and a$="zz" then poke 49348,1
80 h=910:trap (h):e=0:x=1/0:e1=e:trap h+10:e=0:x=1/0:e2=e
81 t%=910:trap t%+0:e=0:dim q(2):q(5)=1:e3=e:trap 900
82 if e1=120 and e2=70 and e3=118 then poke 49349,1
90 e=0:for i=0 to 63:poke 1536+i,255:poke 1600+i,255:next:sprite clr:i=bump(1)
91 h=690:c=0:d=0:sprite 0,1:sprite 1,1:movspr 0,100,100:movspr 1,108,104:collision 1,h+10:sleep .3
92 c1=c:i=bump(1):collision 1:c0=c:sleep .3:c2=c:d1=d
93 c=0:d=0:t%=710:i=bump(1):collision 1,(t%):sleep .3:collision 1:sprite clr
94 if c1>0 and c2=c0 and d1=0 and d>0 and c=0 and e=0 then poke 49350,1
99 poke 49351,170:end
500 n=0:for i%=1 to 5:n=n+1:if n=3 then return
510 next:return
600 e=0:for i%=1 to 40000:e1=e:e=0:for i%=70000 to 2:return
700 c=c+1:return
710 d=d+1:return
900 e=er:resume next
910 e=er+100:resume next
920 e=er+50:resume next
