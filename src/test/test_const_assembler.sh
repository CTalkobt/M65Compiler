#!/bin/bash
# Test for .const directive in assembler

ASM="./bin/ca45"
TEMP_ASM="test_const.s45"
TEMP_BIN="test_const.bin"

echo "Testing .const directive..."

# 1. Basic .const usage
cat <<EOF > $TEMP_ASM
.const MY_CONST = \$1234
lda #MY_CONST & \$FF
ldx #MY_CONST >> 8
EOF

$ASM $TEMP_ASM -o $TEMP_BIN
if [ $? -ne 0 ]; then
    echo "FAILED: Basic .const usage failed"
    exit 1
fi

# Check binary content (LDA #$34, LDX #$12)
# LDA imm is $A9, LDX imm is $A2
# Expected: A9 34 A2 12
hexdump -C $TEMP_BIN | grep "a9 34 a2 12" > /dev/null
if [ $? -ne 0 ]; then
    echo "FAILED: Incorrect binary output for .const"
    hexdump -C $TEMP_BIN
    exit 1
fi
echo "PASSED: Basic .const usage"

# 2. Redefinition of .const (should fail)
cat <<EOF > $TEMP_ASM
.const MY_CONST = \$1234
.const MY_CONST = \$5678
EOF

$ASM $TEMP_ASM -o $TEMP_BIN 2>&1 | grep "Error: Redefinition of symbol" > /dev/null
if [ $? -ne 0 ]; then
    echo "FAILED: Redefinition of .const should fail"
    exit 1
fi
echo "PASSED: Redefinition of .const fails"

# 3. Redefinition of .const with equate (should fail)
cat <<EOF > $TEMP_ASM
.const MY_CONST = \$1234
MY_CONST = \$5678
EOF

$ASM $TEMP_ASM -o $TEMP_BIN 2>&1 | grep "Error: Redefinition of constant symbol" > /dev/null
if [ $? -ne 0 ]; then
    echo "FAILED: Redefinition of .const with equate should fail"
    exit 1
fi
echo "PASSED: Redefinition of .const with equate fails"

# 4. Redefinition of .const with .var (should fail)
cat <<EOF > $TEMP_ASM
.const MY_CONST = \$1234
.var MY_CONST = \$5678
EOF

$ASM $TEMP_ASM -o $TEMP_BIN 2>&1 | grep "Error: Cannot redefine constant" > /dev/null
if [ $? -ne 0 ]; then
    echo "FAILED: Redefinition of .const with .var should fail"
    exit 1
fi
echo "PASSED: Redefinition of .const with .var fails"

# Clean up
rm $TEMP_ASM $TEMP_BIN
echo "All assembler .const tests passed!"
