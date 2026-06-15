#!/bin/bash
# adapt_tests.sh — Convert SDCC GCC torture tests for cc45
#
# Usage: ./adapt_tests.sh <sdcc_gte_dir> <output_dir>
#
# Filters tests for cc45 compatibility (no float, no long long, no printf,
# no signal, no malloc) and adds the testfwk.h include.
# Tests using features not yet supported by cc45 (e.g., multi-var declarations,
# long int) are INCLUDED — they document what needs to be fixed.

SRC="${1:-/tmp/sdcc_gte}"
DST="${2:-.}"
ADAPTED=0
SKIPPED=0

for f in "$SRC"/*.c; do
    base=$(basename "$f")
    skip=0

    # Skip features cc45 will NEVER support
    grep -q '\bfloat\b\|\bdouble\b' "$f" && skip=1
    grep -q '\blong long\b' "$f" && skip=1
    grep -q '\bprintf\b\|\bfprintf\b\|\bsprintf\b' "$f" && skip=1
    grep -q '\bsignal\b' "$f" && skip=1
    grep -q '\bmalloc\b\|\bcalloc\b\|\brealloc\b' "$f" && skip=1
    grep -q 'STACK_SIZE' "$f" && skip=1

    if [ $skip -eq 1 ]; then
        SKIPPED=$((SKIPPED + 1))
        continue
    fi

    # Adapt: prepend testfwk.h include, strip standard includes
    {
        echo "// Adapted from SDCC GCC torture test: $base"
        echo "// Original: gcc/testsuite/gcc.c-torture/execute/"
        echo "#include \"testfwk.h\""
        echo ""
        sed -e 's/#include <stdlib.h>//' \
            -e 's/#include <stdio.h>//' \
            -e 's/#include <limits.h>//' \
            "$f"
    } > "$DST/$base"

    ADAPTED=$((ADAPTED + 1))
done

echo "Adapted: $ADAPTED  Skipped: $SKIPPED  Total: $((ADAPTED + SKIPPED))"
