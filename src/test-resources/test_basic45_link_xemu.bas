' test_basic45_link_xemu.bas -- BASIC calling C (basic45 --link)
'
' Results POKEd at $C0C0 (1 = pass):
'   [0] SYS {add}, 40, 2: arguments in, the result back with RREG
'   [1] globals: bump twice, then get_counter
'   [2] a library call (strlen) on initialized data
'   [3] the address of a C global: PEEK({counter})
'   [4] C writes memory BASIC passes it ($C0C8)
'   [5] $AA sentinel
start: sys {add}, 40, 2 : rreg a, x
  if a + 256 * x = 42 then poke 49344, 1
  sys {bump} : sys {bump} : sys {get_counter} : rreg a, x
  if a = 2 and x = 0 then poke 49345, 1
  sys {msglen} : rreg a
  if a = 7 then poke 49346, 1
  ' PEEK reads RAM in BANK 0 (the default bank maps ROM over parts of
  ' it); SYS goes back to the default BANK 128, which the C code needs
  bank 0 : v = peek({counter}) : bank 128
  if v = 2 then poke 49347, 1
  sys {poke_from_c}, 200, 192
  bank 0 : v = peek(49352) : bank 128
  if v = 90 then poke 49348, 1
  poke 49349, 170
  end
