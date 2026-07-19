# math.h examples for cc45 / MEGA65
#
# Builds and links each test against c45.lib.
# Each test returns A=$00 on success, non-zero on failure.
#
# Usage:
#   make              Build all test PRGs
#   make clean        Remove build artifacts

ROOT = ../../../..
CC45 = $(ROOT)/bin/cc45
LN45 = $(ROOT)/bin/ln45
CRT0 = $(ROOT)/lib/build/crt0.o45
STDLIB = $(ROOT)/lib/build/c45.lib
INCLUDES = -I$(ROOT)/lib/include

TESTS = test_min_max test_gcd_lcm
TARGETS = $(addsuffix .prg,$(TESTS))

.PHONY: all clean

all: $(TARGETS)

%.o45: %.c
	$(CC45) $(INCLUDES) -c $< -o $@

%.prg: %.o45 $(CRT0) $(STDLIB)
	$(LN45) -basic -o $@ $(CRT0) $< $(STDLIB)

clean:
	rm -f *.o45 *.o45.s45 *.prg
