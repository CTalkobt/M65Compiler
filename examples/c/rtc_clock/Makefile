# rtc_clock — the MEGA65 real-time clock in big digits
#
# Usage:
#   make              Build rtc_clock.prg
#   make check        Build the RTC test, run it in xemu and check the result
#   make clean        Remove build artifacts
#   make run          Build and run in xemu (if available)

ROOT = ../../..
CC45 = $(ROOT)/bin/cc45
LN45 = $(ROOT)/bin/ln45
STDLIB = $(ROOT)/lib/build/c45.lib
INCLUDES = -I$(ROOT)/lib/include
XEMU = xemu-xmega65

TARGET = rtc_clock.prg

.PHONY: all clean run check

all: $(TARGET)

main.o45: main.c
	$(CC45) -O2 $(INCLUDES) -c main.c -o main.o45

$(TARGET): main.o45 $(STDLIB)
	$(LN45) -basic -M rtc_clock.map -o $(TARGET) main.o45 $(STDLIB)

rtctest.prg: main.c $(STDLIB)
	$(CC45) -O2 -DRTC_TEST $(INCLUDES) main.c -o rtctest.prg

# $C0C0: valid BCD, 24-hour mode, valid after a second, one second later, $AA
check: rtctest.prg
	rm -f rtctest.mem
	timeout 30 $(XEMU) -headless -besure -sleepless -prgmode 65 -prgexit -dumpmem rtctest.mem -prg rtctest.prg </dev/null >/dev/null 2>&1; \
	test "$$(od -An -tx1 -j $$((0xC0C0)) -N 5 rtctest.mem | tr -d ' \n')" = "01010101aa" && echo "rtc check: pass" || { echo "rtc check: FAIL"; exit 1; }

clean:
	rm -f main.o45 main.o45.s45 $(TARGET) rtc_clock.map rtctest.prg rtctest.mem

run: $(TARGET)
	$(XEMU) -prg $(TARGET)
