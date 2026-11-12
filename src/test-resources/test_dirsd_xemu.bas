' test_dirsd_xemu.bas -- DIR of the SD card (unit 12), run interpreted and
' compiled (basic45 --compile, #369) on a card disk45 makes: every entry, a
' pattern with *, one with ? (test_xemu.sh compares the two screens)
start:
  dir u12
  print "--"
  dir "*.seq", u12
  print "--"
  dir "?1*", u12
  print "end"
