1 rem test_basic_compile26_xemu -- basic45 --compile (#580): gosub / return through the run
2 rem time (__bas_gosub, __bas_return), the string functions' and peek's arguments and results
3 rem in registers. results at $c0c0 (49344): 1 = pass. [0] nested gosubs; [1] return drops a
4 rem for loop of the subroutine (200 times: the stack would fill); [2] a do loop's entry too;
5 rem [3] on gosub; [4] + left$ right$ mid$ chr$ str$ and comparisons; [5] peek / wpeek in
6 rem bank 4; [6] $aa
10 n=0:gosub 500:if n=111 then poke 49344,1
20 c=0:for k=1 to 200:gosub 600:next k:if c=200 and k=201 then poke 49345,1
30 c=0:for k=1 to 200:gosub 650:next k:if c=400 then poke 49346,1
40 c=0:for k=1 to 3:on k gosub 700,710,720:next k:if c=321 then poke 49347,1
50 a$="hello":b$=a$+" "+"world":c$=left$(b$,2)+right$(b$,2)+mid$(b$,5,3)+chr$(65)+chr$(66)
51 d$=str$(12)+str$(-3):e$=left$(a$,1)+mid$(a$,2)+right$(a$,0)
52 q=0:if b$="hello world" and c$="heldo wAB" and d$=" 12-3" then q=q+1
53 if e$=a$ and a$<b$ and b$>a$ and c$<>d$ and not (a$>b$) then q=q+1
54 if q=2 then poke 49348,1
60 poke 294912,77:wpoke 294914,4660:x=peek(294912):y=wpeek(294914):z=peek(294915)
61 if x=77 and y=4660 and z=18 and peek(294912)+peek(294914)=77+52 then poke 49349,1
70 poke 49350,170
80 end
500 n=n+1:gosub 510:return
510 n=n+10:gosub 520:return
520 n=n+100:return
600 for i=1 to 5:if i=3 then c=c+1:return
610 next i:return
650 do:c=c+1:for j=1 to 2:c=c+1:return
660 next j:loop:return
700 c=c+1:return
710 c=c+20:return
720 c=c+300:return
