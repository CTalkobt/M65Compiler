# glamlambs — Glam Lambs, a sheep makeover game; its music is tune45's
#
# Usage:
#   make              Build glamlambs.prg and glamlambs.d81 (the program,
#                     GLAMMUSIC, the music, which the game loads into bank 5,
#                     and GLAMSALON / GLAMSALON2, the salon's picture: banks 4, 5)
#   make demo         Build glamdemo.prg and glamdemo.d81, which play by themselves
#   make demo-studio  The same playing the styling studio (glamstudio: tests)
#   make gfx          Regenerate gfx.h (gen_gfx.py)
#   make salon        Regenerate salon.h, salon1.dat, salon2.dat (gen_salon.py, PIL)
#   make music        Play the music on the PC (tune45 play)
#   make clean        Remove build artifacts
#   make run          Run in xemu with the disk in drive 8

ROOT = ../../..
CC45 = $(ROOT)/bin/cc45
LN45 = $(ROOT)/bin/ln45
DISK45 = $(ROOT)/bin/disk45
TUNE45 = $(ROOT)/bin/tune45
STDLIB = $(ROOT)/lib/build/c45.lib
INCLUDES = -I$(ROOT)/lib/include
XEMU = xemu-xmega65

TARGET = glamlambs.prg

.PHONY: all clean run demo demo-studio gfx salon music

all: glamlambs.d81

# the music: a file for the disk (with a load address, as LOAD reads it) and
# its tunes' offsets in it
glammusic.bin music.h: music.txt $(TUNE45)
	$(TUNE45) convert music.txt --bin --load-at 0 -o glammusic.bin --header music.h

# main.c the game; title.c and salon.c code overlays (#pragma cc45
# overlay): they take turns in one region of bank 0, their code kept in
# bank 5 from GLAMLAMBS.OV1, which the program loads at start
SOURCES = main.c title.c salon.c
HEADERS = glam.h gfx.h music.h salon.h

%.o45: %.c $(HEADERS)
	$(CC45) -O2 $(INCLUDES) -c $< -o $@

demo_%.o45: %.c $(HEADERS)
	$(CC45) -O2 -DDEMO $(INCLUDES) -c $< -o $@

studio_%.o45: %.c $(HEADERS)
	$(CC45) -O2 -DDEMO -DDEMO_STUDIO $(INCLUDES) -c $< -o $@

# a stack share of 1 KB (the overlays' region is under it); the overlays
# kept in bank 5 from $5A000 (the music, the salon's screen and picture
# are below, the screen shown above; the field's background is in colour RAM)
LNFLAGS = --stack-size=1024 --overlay-store=0x5A000-0x5DFFF

$(TARGET): $(SOURCES:.c=.o45) $(STDLIB)
	$(LN45) -basic $(LNFLAGS) -M glamlambs.map -o $(TARGET) $(SOURCES:.c=.o45) $(STDLIB)

glamlambs.d81: $(TARGET) glammusic.bin salon1.dat salon2.dat
	rm -f glamlambs.d81
	$(DISK45) create glamlambs.d81 -n "GLAM LAMBS" -i "GL"
	$(DISK45) add glamlambs.d81 $(TARGET) "glamlambs"
	$(DISK45) add glamlambs.d81 glamlambs.ov1 "glamlambs.ov1"
	$(DISK45) add glamlambs.d81 glammusic.bin "glammusic"
	$(DISK45) add glamlambs.d81 salon1.dat "glamsalon"
	$(DISK45) add glamlambs.d81 salon2.dat "glamsalon2"

demo: $(addprefix demo_,$(SOURCES:.c=.o45)) glammusic.bin salon1.dat salon2.dat $(STDLIB)
	$(LN45) -basic $(LNFLAGS) -o glamdemo.prg $(addprefix demo_,$(SOURCES:.c=.o45)) $(STDLIB)
	rm -f glamdemo.d81
	$(DISK45) create glamdemo.d81 -n "GLAM LAMBS" -i "GL"
	$(DISK45) add glamdemo.d81 glamdemo.prg "glamdemo"
	$(DISK45) add glamdemo.d81 glamdemo.ov1 "glamdemo.ov1"
	$(DISK45) add glamdemo.d81 glammusic.bin "glammusic"
	$(DISK45) add glamdemo.d81 salon1.dat "glamsalon"
	$(DISK45) add glamdemo.d81 salon2.dat "glamsalon2"

# the demo playing the styling studio (tests)
demo-studio: $(addprefix studio_,$(SOURCES:.c=.o45)) glammusic.bin salon1.dat salon2.dat $(STDLIB)
	$(LN45) -basic $(LNFLAGS) -o glamstudio.prg $(addprefix studio_,$(SOURCES:.c=.o45)) $(STDLIB)
	rm -f glamstudio.d81
	$(DISK45) create glamstudio.d81 -n "GLAM LAMBS" -i "GL"
	$(DISK45) add glamstudio.d81 glamstudio.prg "glamstudio"
	$(DISK45) add glamstudio.d81 glamstudio.ov1 "glamstudio.ov1"
	$(DISK45) add glamstudio.d81 glammusic.bin "glammusic"
	$(DISK45) add glamstudio.d81 salon1.dat "glamsalon"
	$(DISK45) add glamstudio.d81 salon2.dat "glamsalon2"

gfx:
	python3 gen_gfx.py .

# the salon's full-colour picture (salon1.dat, salon2.dat, salon.h: kept in
# the repository; gen_salon.py needs Python's PIL)
salon:
	python3 gen_salon.py .

music: music.txt
	$(TUNE45) play music.txt

clean:
	rm -f *.o45 *.o45.s45 music.h glammusic.bin $(TARGET) glamlambs.map glamlambs.d81 glamlambs.ov1 \
		glamlambs.ovd glamdemo.prg glamdemo.d81 glamdemo.ov1 glamdemo.ovd \
		glamstudio.prg glamstudio.d81 glamstudio.ov1 glamstudio.ovd

run: glamlambs.d81
	$(XEMU) -prgmode 65 -8 glamlambs.d81 -prg $(TARGET)
