' test_disk45_rel_xemu.bas -- a REL file made by disk45 (rel-create, rel-write)
' used by BASIC 65's DOS on a D81 (xemu drive 8)
'
' Results POKEd at $C0C0 (1 = pass):
'   [0] the record disk45 wrote (5: "HELLO")
'   [1] an empty record reads as CHR$(255)
'   [2] a record BASIC writes (7) reads back
'   [3] $AA sentinel
' test_xemu.sh then checks with disk45 that records 7 and 40 (DOS extended
' the file through disk45's super side sector) hold what BASIC wrote.
start: dopen#2, "SCORES", l20, u8
  record#2, 5 : input#2, a$
  if a$ = "HELLO" then poke 49344, 1
  record#2, 3 : get#2, b$
  if asc(b$) = 255 then poke 49345, 1
  record#2, 7 : print#2, "BASIC"
  record#2, 40 : print#2, "FAR"
  record#2, 7 : input#2, c$
  if c$ = "BASIC" then poke 49346, 1
  dclose#2
  poke 49347, 170
  end
