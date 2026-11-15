1 rem test_header_id_xemu -- header's disk id (#369): i and two characters, without quotes,
2 rem letters and reserved variables' names too (basic45 cut iab to ia as a long variable
3 rem name). each id read back from the directory's header. results at $c0c0 (49344):
4 rem 1 = pass. [0] iab [1] i5x [2] ist [3] iti [4] i12 [5] $aa
10 trap 900
20 e=0:header "n",iab,u8:gosub 500:if a$="a" and b$="b" and e=0 then poke 49344,1
30 e=0:header "n",i5x,u8:gosub 500:if a$="5" and b$="x" and e=0 then poke 49345,1
40 e=0:header "n",ist,u8:gosub 500:if a$="s" and b$="t" and e=0 then poke 49346,1
50 e=0:header "n",iti,u8:gosub 500:if a$="t" and b$="i" and e=0 then poke 49347,1
60 e=0:header "n",i12,u8:gosub 500:if a$="1" and b$="2" and e=0 then poke 49348,1
70 poke 49349,170:end
500 open 2,8,0,"$":for i=1 to 26:get#2,c$:next
510 get#2,a$,b$:close 2:return
900 e=er:resume next
