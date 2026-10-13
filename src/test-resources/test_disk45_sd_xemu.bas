' test_disk45_sd_xemu.bas -- a MEGA65 SD card image made by disk45 (sd create,
' sd add): xemu boots from it (MEGA65.ROM read from the card) and mounts the
' card's MEGA65.D81 as drive 8
'
' Results POKEd at $C0C0 (1 = pass):
'   [0] the SEQ file disk45 put on the D81 reads back
'   [1] a file written to the D81 (test_xemu.sh then finds it with disk45)
'   [2] $AA sentinel
start: dopen#2, "HELLO"
  input#2, a$
  if a$ = "SDCARD" then poke 49344, 1
  dclose#2
  dopen#3, "BACK", w
  print#3, "WRITTEN"
  dclose#3
  if ds = 0 then poke 49345, 1
  poke 49346, 170
  end
