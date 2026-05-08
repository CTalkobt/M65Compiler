ROOT := ../..
CC45 := $(ROOT)/bin/cc45

all: check-warning

check-warning: main.c
	@echo "=== Compiling issue26/main.c to check for dead code warning ==="
	$(CC45) $< 2>&1 | tee compile.log
	@if grep -q "warning: unreachable code" compile.log; then \
		echo "PASS: Found expected warning for unreachable code"; \
	else \
		echo "FAIL: Expected warning not found"; \
		exit 1; \
	fi

clean:
	rm -f compile.log

.PHONY: all check-warning clean
