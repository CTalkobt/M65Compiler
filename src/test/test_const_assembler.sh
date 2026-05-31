#!/bin/bash
# Test .const directive in assembler - uses shared test utilities

source "$(dirname "$0")/test-lib.sh"

print_section "Assembler .const directive"

TEMP_ASM="test_const.s"
TEMP_BIN="test_const.bin"

# Basic .const usage
cat <<EOF > $TEMP_ASM
.const MY_CONST = \$1234
lda #MY_CONST & \$FF
ldx #MY_CONST >> 8
EOF

if $AS "$TEMP_ASM" -o "$TEMP_BIN" 2>/dev/null; then
    if hexdump -C "$TEMP_BIN" 2>/dev/null | grep -q "a9 34 a2 12"; then
        print_pass "basic .const usage"
    else
        print_fail "basic .const usage (incorrect binary output)"
    fi
else
    print_fail "basic .const usage"
fi

# Redefinition of .const with .const (should fail)
cat <<EOF > $TEMP_ASM
.const MY_CONST = \$1234
.const MY_CONST = \$5678
EOF

if $AS "$TEMP_ASM" -o "$TEMP_BIN" 2>&1 | grep -q "Error: Redefinition of symbol"; then
    print_pass "redefinition of .const fails"
else
    print_fail "redefinition of .const fails"
fi

# Redefinition of .const with equate (should fail)
cat <<EOF > $TEMP_ASM
.const MY_CONST = \$1234
MY_CONST = \$5678
EOF

if $AS "$TEMP_ASM" -o "$TEMP_BIN" 2>&1 | grep -q "Error: Redefinition of constant symbol"; then
    print_pass "redefinition of .const with equate fails"
else
    print_fail "redefinition of .const with equate fails"
fi

# Redefinition of .const with .var (should fail)
cat <<EOF > $TEMP_ASM
.const MY_CONST = \$1234
.var MY_CONST = \$5678
EOF

if $AS "$TEMP_ASM" -o "$TEMP_BIN" 2>&1 | grep -q "Error: Cannot redefine constant"; then
    print_pass "redefinition of .const with .var fails"
else
    print_fail "redefinition of .const with .var fails"
fi

rm -f $TEMP_ASM $TEMP_BIN

test_summary
exit $?
