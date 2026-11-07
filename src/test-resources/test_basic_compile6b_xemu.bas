1 rem test_basic_compile6b_xemu -- loaded by test_basic_compile6_xemu.bas (dload "bc6b"): the
2 rem variables it exported (rem pragma import, its map: basic45 --import-map). [0] a string; [1] a
3 rem float; [2] an integer %; [3] a whole number (an integer narrowed in the exporter); [5] $aa
4 rem pragma import
10 if a$="chained" and z$="" then poke 49344,1
20 if b=1.5 then poke 49345,1
30 if c%=-7 then poke 49346,1
40 if n=42 and p=0 then poke 49347,1
50 poke 49349,170
