1 rem test_basic_compile15_xemu -- basic45 --compile (#369): run compiled and interpreted (the
2 rem same results; the values are the interpreter's). results at $c0c0 (49344): 1 = pass
3 rem [0] sys, rreg (registers given, kept, skipped; after float code); [1] usr (a routine,
4 rem the default: ?undef'd function); [2] fre; [3] dma (jobs, chained: ?illegal quantity),
5 rem edma; [4] clr variable, err$, ti, st; [5] clr (variables, arrays, def fn, data, the
6 rem trap); [6] clr in a for loop (?next without for); [7] def fn defined again; [8] $aa
10 trap 900:dim v(40):n=0
20 for i=0 to 11:read b:poke 4096+i,b:next
30 data 24,105,1,170,232,152,105,2,168,107,123,96
40 sys 4096,10,20,30,40:rreg a,x,y,z,s:v(n)=a:v(n+1)=x:v(n+2)=y:v(n+3)=z:v(n+4)=s and 195:n=n+5
50 sys 4096:rreg a,,y:v(n)=a:v(n+1)=y:n=n+2:sys 4096,,,,7:x=sin(1):rreg ,x,,z:v(n)=x:v(n+1)=z:n=n+2
60 poke 4200,96:poke $2f8,104,16:v(n)=usr(5):v(n+1)=usr(-2.5)*2:n=n+2
70 v(n)=fre(-1):v(n+1)=fre(2)+fre(4)+fre(9):n=n+2
80 for i=0 to 3:poke $50000+i,i+1:next:dma 0,4,0,5,256,5:v(n)=peek($50100)+peek($50103):n=n+1
90 dma 3,10,170,0,0,5,0,0,0,0,4,512,5,768,5:v(n)=peek($50000)+peek($50009)+peek($50302):n=n+1
100 poke $50000,9,8,7,6:edma 0,4,$50000,$50400:v(n)=peek($50401)+peek($50403):n=n+1
110 dma 4,1,0,5,0,5:v(n)=ee:n=n+1
120 a=5:c$="hi":clr a:clr c$:v(n)=a+len(c$):n=n+1:x=1/0:clr err$:v(n)=er+el:n=n+1
130 clr ti:v(n)=-(ti<1):clr st:v(n+1)=st:n=n+2
140 if v(0)=0 and v(1)=12 and v(2)=32 and v(3)=40 and v(4)=2 then poke 49344,1
150 if v(5)=0 and v(6)=34 and v(7)=2 and v(8)=7 then 155
152 goto 160
155 if v(9)=5 and v(10)=-5 then poke 49345,1
160 if v(11)=920413 and v(12)=65535 then poke 49346,1
170 if v(13)=5 and v(14)=340 and v(15)=14 and v(16)=14 then poke 49347,1
180 if v(17)=0 and v(18)=65534 and v(19)=1 and v(20)=0 then poke 49348,1
190 q=1:dim d(3):d(2)=9:def fn f(x)=x*2:read r:clr:trap 900:read s:dim d(5)
200 x=fn f(2):if q=0 and ee=27 and s=24 and r=0 then poke 49349,1
210 ee=0:for i=1 to 2:clr:trap 900:next:if ee=10 then poke 49350,1
215 def fn g(x)=x*2:a=fn g(5):def fn g(y)=y+100:b=fn g(5):def fn g(x)=x*2:c=fn g(fn g(1))
216 if a=10 and b=105 and c=4 then poke 49351,1
220 poke 49352,170:end
230 data 24
900 e=e+1:ee=er:resume next
