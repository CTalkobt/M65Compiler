#!/bin/bash
# test_opcodes.sh — Validate that ca45 correctly assembles all opcodes
# from AssemblerOpcodeDatabase.cpp to their expected byte encodings.

CA="./bin/ca45"
META_FILE="build/full_opcode_meta.txt"
DB_FILE="src/main/AssemblerOpcodeDatabase.cpp"

mkdir -p build

# 1. Extract opcode entries from the C++ database
# Format: mnemonic|addressing_mode_enum|expected_hex_byte
python3 - "$DB_FILE" "$META_FILE" << 'PYEOF'
import re, sys

db_file = sys.argv[1]
meta_file = sys.argv[2]

with open(db_file) as f:
    content = f.read()

# Map C++ AddressingMode enum names to test syntax
mode_map = {
    'IMPLIED':                 ('imp',    None),
    'ACCUMULATOR':             ('acc',    'A'),
    'IMMEDIATE':               ('imm',    '#$12'),
    'IMMEDIATE16':             ('imm16',  '#$1234'),
    'BASE_PAGE':               ('bp',     '$12'),
    'BASE_PAGE_X':             ('bp_x',   '$12,X'),
    'BASE_PAGE_Y':             ('bp_y',   '$12,Y'),
    'ABSOLUTE':                ('abs',    '$1234'),
    'ABSOLUTE_X':              ('abs_x',  '$1234,X'),
    'ABSOLUTE_Y':              ('abs_y',  '$1234,Y'),
    'BASE_PAGE_X_INDIRECT':    ('bp_xi',  '($12,X)'),
    'BASE_PAGE_INDIRECT_Y':    ('bp_iy',  '($12),Y'),
    'BASE_PAGE_INDIRECT_Z':    ('bp_iz',  '($12),Z'),
    'ABSOLUTE_INDIRECT':       ('abs_i',  '($1234)'),
    'ABSOLUTE_X_INDIRECT':     ('abs_xi', '($1234,X)'),
    'RELATIVE':                ('rel',    None),     # needs label
    'RELATIVE16':              ('rel16',  None),     # needs label
    'BASE_PAGE_RELATIVE':      ('bp_rel', None),     # needs special handling
    'BASE_PAGE_INDIRECT_SP_Y': ('bp_spy', '($12,SP),Y'),
    'FLAT_INDIRECT_Z':         ('flat_z', '[$12],Z'),
    'QUAD_Q':                  ('quad',   None),     # skip (Q register ops)
}

# Skip these modes — they need special assembly syntax or are aliases
skip_modes = {'STACK_RELATIVE', 'STACK_RELATIVE_INDIRECT_Y',
              'LINEAR_ABSOLUTE', 'LINEAR_ABSOLUTE_X', 'LINEAR_ABSOLUTE_Y',
              'QUAD_Q', 'NONE'}

# Quad instructions (ldq/stq) use $42 $42 prefix before the base opcode
quad_mnemonics = {'ldq', 'stq'}

# Parse entries like: {{"mnemonic", AddressingMode::MODE}, 0xHH},
pattern = re.compile(r'\{\{"(\w+)",\s*AddressingMode::(\w+)\},\s*0x([0-9A-Fa-f]{2})\}')

entries = []
seen = set()
for m in pattern.finditer(content):
    mnemonic = m.group(1)
    mode_enum = m.group(2)
    opcode = m.group(3).lower()

    if mode_enum in skip_modes:
        continue

    key = (mnemonic, mode_enum)
    if key in seen:
        continue  # skip duplicates (e.g., FLAT_INDIRECT_Z aliases BASE_PAGE_INDIRECT_Z)
    seen.add(key)

    if mode_enum not in mode_map:
        continue

    mode_tag, operand = mode_map[mode_enum]

    # Build expected bytes with prefixes
    if mnemonic in quad_mnemonics:
        if mode_tag == 'flat_z':
            expected = f"42 42 ea {opcode}"
        else:
            expected = f"42 42 {opcode}"
    elif mode_tag == 'flat_z':
        expected = f"ea {opcode}"
    else:
        expected = opcode

    # RTN uses bare decimal operand, not #immediate
    if mnemonic == 'rtn' and mode_tag == 'imm':
        operand = '2'

    entries.append((mnemonic, mode_tag, operand, expected))

with open(meta_file, 'w') as f:
    for mnemonic, mode_tag, operand, expected in entries:
        f.write(f"{mnemonic}|{mode_tag}|{operand or ''}|{expected}\n")

print(f"Extracted {len(entries)} opcode entries from {db_file}")
PYEOF

if [ ! -f "$META_FILE" ]; then
    echo "Error: failed to extract opcodes"
    exit 1
fi

# 2. Validate each opcode by assembling and checking the first byte
passed=0
failed=0
skipped=0

while IFS='|' read -r mnemonic mode_tag operand expected_bytes; do
    asm_code=""

    case $mode_tag in
        imp)    asm_code="${mnemonic}" ;;
        acc)    asm_code="${mnemonic} ${operand}" ;;
        imm|imm16|bp|bp_x|bp_y|abs|abs_x|abs_y|bp_xi|bp_iy|bp_iz|abs_i|abs_xi|bp_spy|flat_z)
                asm_code="${mnemonic} ${operand}" ;;
        rel)
            asm_code="target:\n${mnemonic} target"
            ;;
        rel16)
            # Force 16-bit branch by placing target far away
            asm_code="${mnemonic} target\n.fill 200, \$EA\ntarget:"
            ;;
        bp_rel)
            # bbr/bbs: mnemonic $zp, label
            asm_code="target:\n${mnemonic} \$12, target"
            ;;
        *)
            skipped=$((skipped + 1))
            continue
            ;;
    esac

    echo ".org \$2000" > build/single_op.s45
    printf '%b\n' "$asm_code" >> build/single_op.s45

    $CA -o build/single_op.bin build/single_op.s45 > /dev/null 2>&1
    if [ $? -ne 0 ]; then
        echo "FAIL (assemble): ${mnemonic} ${mode_tag}"
        failed=$((failed + 1))
        continue
    fi

    # Extract actual bytes and compare opcode byte(s)
    actual_bytes=$(hexdump -v -e '1/1 "%02x " ' build/single_op.bin | xargs)
    expected_lower=$(echo "$expected_bytes" | tr '[:upper:]' '[:lower:]' | xargs)
    count=$(echo "$expected_lower" | wc -w)
    actual_prefix=$(echo "$actual_bytes" | cut -d' ' -f1-"$count")

    if [ "$actual_prefix" = "$expected_lower" ]; then
        passed=$((passed + 1))
    else
        echo "FAIL (bytes): ${mnemonic} ${mode_tag} — expected: ${expected_lower}, got: ${actual_prefix}"
        failed=$((failed + 1))
    fi
done < "$META_FILE"

echo ""
echo "Summary: $passed passed, $failed failed, $skipped skipped."

if [ $failed -eq 0 ]; then
    exit 0
else
    exit 1
fi
