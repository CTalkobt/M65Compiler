# Issue 45 reproduction — linker symbol overlap
#
# Build and link, then check the symbol map for overlapping addresses.
# With the fix (ca4cc92+), frac1000q88 and _projectallvertices should
# be at different addresses.

CC45  ?= cc45
CA45  ?= ca45
LN45  ?= ln45

OBJS = crt0.o45 main.o45 init.o45 load.o45 pvert.o45

all: 20.prg

main.o45: main.c
	$(CC45) -c $< -o $@

%.o45: %.s
	$(CA45) -c $< -o $@

20.prg: $(OBJS)
	$(LN45) -basic -m -o $@ $(OBJS) > 20.report
	@echo "--- Symbol map ---"
	@cat 20.report
	@echo
	@if grep -q 'frac1000q88' 20.report && grep -q '_projectallvertices' 20.report; then \
		FRAC=$$(grep 'frac1000q88' 20.report | awk '{print $$1}'); \
		PROJ=$$(grep '_projectallvertices' 20.report | awk '{print $$1}'); \
		if [ "$$FRAC" = "$$PROJ" ]; then \
			echo "BUG: frac1000q88 and _projectallvertices overlap at $$FRAC"; \
			exit 1; \
		else \
			echo "OK: frac1000q88=$$FRAC _projectallvertices=$$PROJ (no overlap)"; \
		fi; \
	fi

clean:
	rm -f *.o45 *.o45.s *.prg *.report

.PHONY: all clean
