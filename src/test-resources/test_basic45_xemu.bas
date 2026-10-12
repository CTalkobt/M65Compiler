' test_basic45_xemu.bas -- basic45 output run by BASIC 65 on xemu
' (test_xemu.sh builds it as is and with --minify)
'
' Results POKEd at $C0C0 (49344) (1 = pass):
'   [0] operator tokens and precedence
'   [1] GOSUB to a label, RETURN
'   [2] ON ... GOTO with a list of labels; a label alone on its line
'   [3] strings, LEFT$, IF ... THEN <label>, GO TO
'   [4] FOR/NEXT, AND, < >
'   [5] READ/DATA (DATA items are not tokenized), REM text
'   [6] ? (PRINT), TAB(, SPC(, PETSCII escapes in a string
'   [7] $AA sentinel
start: a = 2 + 3 * 4 - 8 / 2 ^ 2
  if a = 12 then poke 49344, 1
  b = 0
  gosub addone
  gosub AddOne
  if b = 2 then poke 49345, 1
  on 2 goto first, second, third
first: poke 49346, 0 : goto joined
second: poke 49346, 1 : goto joined
third: poke 49346, 0
joined:
  x$ = "hi" : if left$(x$, 1) = "h" then strok
  goto strdone
strok: poke 49347, 1
strdone:
  c = 0
  for i = 1 to 5 : c = c + i : next
  if c = 15 and c > 14 and c < 16 and c <> 3 then poke 49348, 1
  rem a rem with goto first and + * in it
  read d, e$, f$ : if d = 7 and e$ = "OK" and f$ = "A+B" then poke 49349, 1
  ? "{clear}{down}{red}basic45"; tab(12); spc(2); "{white}ok"
  poke 49350, 1
  poke 49351, 170
  end
addone: b = b + 1 : return
  data 7, ok, a+b
