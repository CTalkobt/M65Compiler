1 rem test_basic_compile3_xemu -- basic45 --compile (#369): run compiled and interpreted (the
2 rem same results). results at $c0c0 (49344): 1 = pass
3 rem [0] trap, er / el / err$, resume next; [1] resume line, a trap in the handler's line; [2] resume
4 rem (the statement again); [3] a trap set in a subroutine; [4] trap off; err$ of a number without one;
5 rem [5] $aa
10 if er=-1 and el=65535 and err$(14)="illegal quantity" and err$(0)="ok" and err$(45)="ok" then 12
11 goto 20
12 if err$(44)="cbdos drives only" then poke 49347,1
20 c=0:trap 100:x=1/0:if c=1 and e=20 and l=20 then poke 49344,1
30 c=0:dim b(2):b(5)=1:print "no"
40 b(5)=2:goto 45
45 if c=2 and e=18 and l=40 then poke 49345,1
50 d=0:trap 200:x=5/d:if x=5 then poke 49346,1
60 c=0:for i=1 to 3:gosub 300:next:if c=30 and er=-1 then 70
65 goto 80
70 trap:poke 49348,1
80 poke 49349,170:end
100 c=c+1:e=er:l=el:if er=20 then resume next
110 if c<2 then resume 40
120 resume next
200 d=1:resume
300 trap 350:z=sqr(-i):return
350 c=c+10:resume next
