' menu.bas — the menu of the examples disks (basic45 source)
'
' The Makefile makes a menu for each disk, games_menu.bas and
' demos_menu.bas: this file (#include) and, at the label entries, the
' disk's title and its programs (from games.txt, demos.txt) as DATA. Each
' is saved on its disk as AUTOBOOT.C65 (the MEGA65 runs it when it starts
' with the disk in drive 0) and as MENU. A key loads and starts an example:
' DLOAD in a running program loads the next one into BASIC memory and runs
' it, and each example is a BASIC stub (SYS) at $2001 followed by its code.
' The examples end at BASIC's READY. prompt: RUN "MENU" comes back here.

start:
  dim f$(35), d$(35)
  n = 0
  restore entries
  read title$
  read f$
  do while f$ <> "*"
    read d$
    n = n + 1
    f$(n) = f$ : d$(n) = d$
    read f$
  loop
  keys$ = "123456789abcdefghijklmnopqrstuvwxyz"
  ' pages of up to 13 examples (the screen has 25 lines); space turns them
  per = 13
  pages = int((n + per - 1) / per)
  page = 1

show:
  p0 = (page - 1) * per + 1
  p1 = p0 + per - 1
  if p1 > n then p1 = n
  print "{clear}{white}  "; title$
  print "{light-blue}  built with the cc45 suite, disk by disk45   page"; page; "of"; pages; "{white}"
  print
  for i = p0 to p1
    print "  {yellow}"; mid$(keys$, i - p0 + 1, 1); "{white}  "; d$(i)
  next
  print
  print "{light-blue}  press a key to load and run an example,"
  print "  space for the next page."
  print "  when it ends: run "; chr$(34); "menu"; chr$(34)

choose:
  getkey k$
  if k$ <> " " then goto pick
  page = page + 1
  if page > pages then page = 1
  goto show

pick:
  k = instr(keys$, k$)
  if k < 1 or k > p1 - p0 + 1 then goto choose
  k = p0 + k - 1
  print "{clear}{white}loading "; f$(k); " ..."
  dload (f$(k))
