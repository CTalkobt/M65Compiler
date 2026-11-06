1 rem test_basic_compile2_xemu -- basic45 --compile (#369): run compiled and interpreted (the
2 rem same results). results at $c0c0 (49344): 1 = pass
3 rem [0] input, get, the keys put in the keyboard buffer (688, its count 208); [1] do / loop / exit
4 rem [2] rnd (a seeded sequence), ti, ti$; [3] read / data / restore; [4] $aa
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
199 poke 49348,170:end
