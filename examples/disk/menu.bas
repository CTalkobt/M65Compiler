' menu.bas — the menu of the examples disk (basic45 source)
'
' Saved on examples.d81 as AUTOBOOT.C65 (the MEGA65 runs it when it starts
' with the disk in drive 0) and as MENU. A key loads and starts an example:
' DLOAD in a running program loads the next one into BASIC memory and runs
' it, and each example is a BASIC stub (SYS) at $2001 followed by its code.
' The examples end at BASIC's READY. prompt: RUN "MENU" comes back here.

start:
  dim f$(35), d$(35)
  n = 0
  restore entries
  read f$
  do while f$ <> "*"
    read d$
    n = n + 1
    f$(n) = f$ : d$(n) = d$
    read f$
  loop
  keys$ = "123456789abcdefghijklmnopqrstuvwxyz"

show:
  print "{clear}{white}  cc45 examples for the mega65"
  print "{light-blue}  built with the cc45 suite, disk by disk45{white}"
  print
  for i = 1 to n
    print "  {yellow}"; mid$(keys$, i, 1); "{white}  "; d$(i)
  next
  print
  print "{light-blue}  press a key to load and run an example."
  print "  when it ends: run "; chr$(34); "menu"; chr$(34)

choose:
  getkey k$
  k = instr(keys$, k$)
  if k < 1 or k > n then goto choose
  print "{clear}{white}loading "; f$(k); " ..."
  dload (f$(k))

entries:
  data "invaders",     "invaders - a space invaders game"
  data "rogue",        "rogue - a dungeon crawl"
  data "mandelbrot",   "mandelbrot - the set, zoom and scroll"
  data "musicgen",     "musicgen - random music in 7 styles"
  data "calc graph",   "calc graph - a calculator that plots"
  data "forth",        "forth - a tiny forth"
  data "rtc clock",    "rtc clock - the real-time clock"
  data "far scroller", "far scroller - a dma landscape"
  data "text scroller","text scroller - a sprite and a message"
  data "game of life", "game of life - conway's cellular world"
  data "palette fade", "palette fade - c and assembly"
  data "vic4 colors",  "vic4 colors - registers as a struct"
  data "bitfield regs","bitfield regs - registers as bitfields"
  data "memset screen","memset screen - every character"
  data "hello world",  "hello world"
  data "unit convert", "unit convert - long arithmetic"
  data "multi module", "multi module - three files linked"
  data "ctype demo",   "ctype demo - petscii classes"
  data "basic+c",      "basic+c - basic 65 calling c"
  data "*"
