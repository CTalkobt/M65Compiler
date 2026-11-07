1 rem test_basic_compile5_xemu -- basic45 --compile (#369): run compiled and interpreted (the
2 rem same results), an empty d81 as drive 8. results at $c0c0 (49344): 1 = pass
3 rem [0] ds / ds$ (00,ok); [1] dopen w, append, dopen to read, dclose#; [2] a missing file: ds 62,
4 rem ds$ kept until a dopen; dclose of all; [3] "@" replaces; [4] a name expression, d0, u(x),
5 rem on u8; [5] a relative file (l20, l); [6] ?illegal quantity (lf 0, d12, l0), ?missing file
6 rem name, ?syntax (append "@"), ?too many files; [7] $aa
10 if ds=0 and ds$="00,ok,00,00" then poke 49344,1
20 dopen#1,"d1",w:d1=ds:print#1,"one":dclose#1
21 append#1,"d1":print#1,"two":dclose#1
22 dopen#2,"d1",d0,u8:input#2,a$,b$:dclose#2
23 if d1=0 and a$="one" and b$="two" and ds=0 then poke 49345,1
30 dopen#3,"nofile":e1=ds:e$=left$(ds$,2):open 9,3:e2=ds:close 9:dclose
31 dopen#3,"d1":e3=ds:dclose
32 if e1=62 and e$="62" and e2=62 and e3=0 then poke 49346,1
40 dopen#4,"@d1",w:print#4,"three":dclose#4,u8
41 dopen#4,"d1":input#4,c$:dclose u8:if c$="three" then poke 49347,1
50 dv=8:n$="d":dopen#5,(n$+"1"),d0,u(dv):get#5,g$:dclose
51 dopen#6,"d2",w on u8:print#6,g$:dclose:dopen#6,"d2":input#6,h$:dclose
52 if g$="t" and h$="t" then poke 49348,1
60 dopen#1,"rel1",l20:r1=ds:dclose:dopen#1,"rel1",l:r2=ds:dclose
61 if r1=0 and r2=0 then poke 49349,1
70 trap 900:c=0
71 dopen#0,"x"
72 dopen#1,""
73 append#1,"@x"
74 dopen#1,"x",d12
75 dopen#1,"x",l0
76 for i=1 to 12:dopen#i,"f"+str$(i),w:next
77 dclose:if c>=6 and left$(k$,15)="14 8 11 14 14 1" then poke 49350,1
80 poke 49351,170:end
900 c=c+1:k$=k$+mid$(str$(er),2):if c<6 then k$=k$+" "
910 resume next
