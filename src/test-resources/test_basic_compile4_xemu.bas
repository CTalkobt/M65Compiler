1 rem test_basic_compile4_xemu -- basic45 --compile (#369): run compiled and interpreted (the
2 rem same results), an empty d81 as drive 8. results at $c0c0 (49344): 1 = pass
3 rem [0] open / print# / close of a seq file, st; [1] input# (items, a quoted comma, numbers);
4 rem [2] get# (chr$(0), a signed byte, st 64 at the end); [3] cmd, then print#; [4] a channel
5 rem over 127 (a line feed after the return); [5] a missing file (st), ?file not open trapped;
6 rem [6] ?file data trapped, the channel released; [7] $aa
10 open 2,8,2,"t1,s,w"
11 print#2,"alpha,beta";chr$(13);"123.5"
12 print#2,"x";:print#2,-7
13 print#2,chr$(34)+"q,s"+chr$(34);",";4
14 close 2:if st=0 then poke 49344,1
20 open 2,8,2,"t1,s,r"
21 input#2,a$,b$,c:input#2,d$:input#2,e$,f
22 close 2
23 if a$="alpha" and b$="beta" and c=123.5 and d$="x-7 " and e$="q,s" and f=4 then poke 49345,1
30 open 3,8,3,"t2,s,w":print#3,chr$(200);chr$(0);"a";:close 3
31 open 3,8,3,"t2,s,r":get#3,n1,g$,n3:s1=st:get#3,h$:s2=st:close 3
32 if n1=-56 and len(g$)=1 and asc(g$)=0 and n3=65 and s1=64 and s2=-62 then poke 49346,1
40 open 4,8,4,"t3,s,w":cmd 4,"one";:print "two";1:print#4,"three":close 4
41 open 4,8,4,"t3,s,r":input#4,i$:input#4,j$:close 4
42 if i$="onetwo 1 " and j$="three" then poke 49347,1
50 open 130,8,5,"t4,s,w":print#130,"z":close 130
51 open 5,8,5,"t4,s,r":get#5,k1,k2,k3:close 5
52 if k1=90 and k2=13 and k3=10 then poke 49348,1
60 open 6,8,6,"nofile,s,r":input#6,m$:s3=st:close 6
61 trap 100:print#9,"x":if s3=-62 and m$="" and e1=3 then poke 49349,1
70 trap 110:open 7,8,7,"t1,s,r":input#7,p
71 if e2=24 then print "ok";:poke 49350,1
80 poke 49351,170:end
100 e1=er:resume next
110 e2=er:close 7:resume 71
