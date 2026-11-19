10 rem test_basic_compile_dt_xemu.bas: dt$, the date (#636), interpreted and
20 rem compiled: "dd-mmm-20yy " from the rtc, 12 characters (or "no-rtc-2000 ").
30 rem each check writes 1 at 49344+i; 170 after the last.
40 a$=dt$
50 if len(a$)=12 and right$(a$,1)=" " then poke 49344,1
60 if mid$(a$,3,1)="-" and mid$(a$,7,1)="-" then poke 49345,1
70 if a$="no-rtc-2000 " or mid$(a$,8,2)="20" then poke 49346,1
80 m$=mid$(a$,4,3):k=0:for i=1 to 12:if mid$("janfebmaraprmayjunjulaugsepoctnovdec",i*3-2,3)=m$ then k=i
90 next:if k>0 or a$="no-rtc-2000 " then poke 49347,1
100 poke 49348,170
