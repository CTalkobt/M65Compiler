1 rem test_basic_compile7_xemu -- basic45 --compile (#369): run compiled and interpreted (the
2 rem same results), an empty d81 as drive 8. results at $c0c0 (49344): 1 = pass
3 rem [0] rename, copy, concat; [1] scratch, erase, delete "name" (ds read again after opening
4 rem the command channel); [2] collect, dclear; [3] record# on a relative file; [4] header (the disk's name,
5 rem an id), format (header's alias; turbo: nothing); [5] ?syntax at run time (rename "@x"); [6] copy / backup to drive 1 (xemu has none:
6 rem copy ok, backup ?74, drive not ready); [7] $aa
10 trap 900
20 dopen#1,"a",w:print#1,"one":dclose
21 rename "a" to "b":gosub 800:d1=d
22 copy "b" to "c":gosub 800:d2=d
23 concat "b" to "c":gosub 800:d3=d
24 dopen#1,"c":input#1,x$,y$:dclose
25 if d1=0 and d2=0 and d3=0 and x$="one" and y$="one" and e=0 then poke 49344,1
30 scratch "b":gosub 800:s1=d:s$=d$
31 erase "zz":gosub 800:s2=d
32 delete "c":gosub 800
33 dopen#1,"c":s3=ds:dclose
34 if s1=0 and s$="00,ok,00,00" and s2=0 and s3=62 and e=0 then poke 49345,1
40 collect:dclear u8:collect d0,u8:gosub 800:if e=0 then poke 49346,1
50 dopen#2,"r1",l10:record#2,3:print#2,"third":record#2,1:print#2,"first":dclose
51 dopen#2,"r1",l10:record#2,3:input#2,z$:record#2,1,1:input#2,w$:dclose
52 if z$="third" and w$="first" and e=0 then poke 49347,1
60 i$="42":header "newdisk",i(i$),d0 on u8:gosub 800:h1=d
61 open 9,8,0,"$":h$="":for i=1 to 32:get#9,a$:h$=h$+a$:next:close 9
62 if h1=0 and instr(h$,"newdisk")>0 and instr(h$,"42")>0 and e=0 then poke 49348,1
63 format "fmtdisk",i(i$),d0 on u8:gosub 800:h2=d:turbo
64 open 9,8,0,"$":h$="":for i=1 to 32:get#9,a$:h$=h$+a$:next:close 9
65 if h2<>0 or instr(h$,"fmtdisk")=0 or e<>0 then poke 49348,0
70 rename "@x" to "y":if e=1 and r=11 then poke 49349,1
80 e=0:copy d0 to d1:gosub 800:c1=d:backup d0 to d1:gosub 800:c2=d:backup u8 to u9:gosub 800
81 if c1=0 and c2=74 and d=74 and e=0 then poke 49350,1
90 poke 49351,170:end
800 open 9,8,15:close 9:d=ds:d$=ds$:return
900 e=e+1:r=er:resume next
