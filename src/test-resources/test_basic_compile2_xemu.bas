1 rem test_basic_compile2_xemu -- basic45 --compile (#369): run compiled and interpreted (the
2 rem same results). results at $c0c0 (49344): 1 = pass
3 rem [0] input, get, the keys put in the keyboard buffer (688, its count 208); [1] do / loop / exit
4 rem [2] rnd (a seeded sequence), ti, ti$; [3] read / data / restore; [4] counted for loops
5 rem [5] and / or / not to 65535, xor, << >>, mod; [6] hex$ dec strbin$ val instr; [7] fn, wpoke / wpeek, wait,
6 rem pos, sleep; [8] $aa
160 poke 688,52:poke 689,44:poke 690,72:poke 691,13:poke 692,88:poke 208,5:input "";n,h$:get k$:get z:get e$
161 poke 688,65:poke 689,13:poke 690,57:poke 691,13:poke 208,4:input m:if n=4 and h$="h" and k$="x" and z=0 then 163
162 goto 170
163 if e$="" and m=9 then poke 49344,1
170 i=0:do:i=i+1:loop until i=5:j=0:do while j<3:j=j+1:loop:k=0:do:k=k+1:if k=4 then exit
171 print "";:loop
172 n=0:do:do:n=n+1:loop while n<3:n=n+10:loop until n>30:if i=5 and j=3 and k=4 and n=35 then poke 49345,1
180 x=rnd(-1.5):y=rnd(1):t=ti:for i=1 to 500:next:u=ti:a$=ti$:if str$(x)=" 0.001312286" and y>0 and y<1 then 182
181 goto 190
182 if u>=t and t>=0 and len(a$)=8 and mid$(a$,3,1)=":" and mid$(a$,6,1)=":" then poke 49346,1
190 read a,b$,c:x=0:restore 196:read d,e$:if a=1.5 and b$="two" and c=-3 and d=-3 and e$=" sp " then poke 49347,1
195 data 1.5,two
196 data -3," sp "
197 n=0:for i=1 to 5:if i=3 then n=n+100:next
198 a=n:b=i:n=0:for i=1 to 3:for j=1 to 2:n=n+i*j:next j,i:s=0:for x=0 to 1 step .25:s=s+x:next:if a=0 and b=1 then 200
199 goto 210
200 t=0:for k=10 to 1 step -2:t=t+k:next:if n=18 and i=4 and j=3 and s=2.5 and x=1.25 and t=30 and k=0 then poke 49348,1
210 if (40000 and 1)=0 and (65535 or 0)=-1 and (not 40000)=25535 and (-1.5 and 255)=254 then 212
211 goto 220
212 if (3 xor 5)=6 and (5 xor 3 and 1)=4 and (1<<4)=16 and (-16>>2)=-4 and (32767<<1)=-2 then 214
213 goto 220
214 if mod(7,3)=1 and mod(-7,3)=2 and mod(7.5,2)=1.5 and mod(7,-3)=-2 then poke 49349,1
220 if hex$(255)="00ff" and hex$(65536)="00010000" and hex$(-1)="ffffffff" and dec("ff")=255 then 222
221 goto 230
222 if dec("1g")=1 and strbin$(5)="00000101" and val("12ab")=12 and val("1e5x")=100000 and val("1.2.3")=0 then 224
223 goto 230
224 if instr("hello","l")=3 and instr("hello","l",4)=4 and instr("hello","lo",5)=0 and instr("hello","")=0 then 226
225 goto 230
226 if val("3e")=0 and val(" -3.5e2")=-350 then poke 49350,1
230 print chr$(147);"ab";:p=pos(0):def fn a(x)=x*2+1:def fn b(y)=fn a(y)+y:x=100:wpoke 4030,1234:poke 4032,7:wait 4032,4
231 t=ti:sleep .2:u=ti:if fn a(3)=7 and fn b(4)=13 and x=100 and fn a(x)=201 then 233
232 goto 240
233 if wpeek(4030)=1234 and peek(4031)=4 and u-t>=.15 and p=2 then poke 49351,1
240 poke 49352,170:end
