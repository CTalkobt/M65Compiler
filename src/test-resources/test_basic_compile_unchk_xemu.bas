10 rem test_basic_compile_unchk_xemu.bas: the conversions compiled without
20 rem --checks (#489: __bas_ayintu, __bas_getadr32u) give the interpreter's
30 rem values in range. each check writes 1 at 49344+i; 170 after the last.
40 s=2048.5:b=200.9:c=-5.5:d=40000:e=-32768:f=65535
50 poke s,b:if peek(s)=200 then poke 49344,1
60 x%=c:if x%=-6 then poke 49345,1
70 if (d and 255)=64 then poke 49346,1
80 if (e or 1)=-32767 then poke 49347,1
90 if (f and 15)=15 then poke 49348,1
100 a$=chr$(b-135.5):if asc(a$)=65 then poke 49349,1
110 y%=d-40001.5:if y%=-2 then poke 49350,1
120 poke 49351,170
