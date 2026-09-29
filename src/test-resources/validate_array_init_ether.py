#!/usr/bin/env python3
"""
Validate array_init test results from Ethernet payload

Usage:
    ./validate_array_init_ether.py <hex_payload>
    echo "A5 0F 10 40 64 2C 0B 16 00 00 00 00 AA CC E8 D0 FF" | ./validate_array_init_ether.py

Expected payload format:
    [0xA5] [count=0x0F] [15 bytes...] [0xFF]
"""

import sys
import re

# Expected test values from test_array_init_ether.c
EXPECTED_VALUES = {
    0: ('bytes[0]', 0x10),
    1: ('bytes[3]', 0x40),
    2: ('words[0]', 0x64),
    3: ('words[2]', 0x2C),
    4: ('partial[0]', 0x0B),
    5: ('partial[1]', 0x16),
    6: ('partial[2]', 0x00),
    7: ('partial[4]', 0x00),
    8: ('zeros[0]', 0x00),
    9: ('zeros[2]', 0x00),
    10: ('local_bytes[0]', 0xAA),
    11: ('local_bytes[2]', 0xCC),
    12: ('local_words[0]', 0xE8),
    13: ('local_words[1]', 0xD0),
}

def parse_hex_payload(hex_str):
    """Parse hex string/bytes into list of integers."""
    # Handle various input formats
    hex_str = hex_str.strip().replace('\n', ' ').replace('\t', ' ')

    # Extract hex bytes (0x?? or ??)
    matches = re.findall(r'(?:0x)?([0-9a-fA-F]{2})', hex_str)
    return [int(m, 16) for m in matches]

def validate_payload(payload):
    """Validate array initialization test payload."""
    if len(payload) < 3:
        print("ERROR: Payload too short (minimum 3 bytes)")
        return False

    # Check magic byte
    magic = payload[0]
    if magic != 0xA5:
        print(f"ERROR: Invalid magic byte: 0x{magic:02X} (expected 0xA5)")
        return False
    print(f"✓ Magic byte correct: 0xA5")

    # Check test count
    test_count = payload[1]
    if test_count != 0x0F and test_count != 15:
        print(f"ERROR: Invalid test count: 0x{test_count:02X} (expected 0x0F = 15)")
        return False
    print(f"✓ Test count correct: {test_count} tests")

    # Extract test values
    test_values = payload[2:2+test_count]

    if len(test_values) < test_count:
        print(f"ERROR: Expected {test_count} test values, got {len(test_values)}")
        return False

    # Validate each test value
    all_pass = True
    print(f"\nValidating {test_count} test values:")
    print(f"{'#':<3} {'Test Name':<25} {'Expected':<10} {'Actual':<10} {'Status':<8}")
    print("-" * 60)

    for i, (name, expected) in EXPECTED_VALUES.items():
        if i >= len(test_values):
            print(f"{i:<3} {name:<25} 0x{expected:02X}       (missing)     FAIL")
            all_pass = False
            continue

        actual = test_values[i]
        status = "PASS" if actual == expected else "FAIL"
        if status == "FAIL":
            all_pass = False

        print(f"{i:<3} {name:<25} 0x{expected:02X}       0x{actual:02X}       {status}")

    # Check terminator
    if len(payload) > 2 + test_count:
        terminator = payload[2 + test_count]
        if terminator == 0xFF:
            print(f"\n✓ Terminator byte correct: 0xFF")
        else:
            print(f"\n⚠ Terminator byte unexpected: 0x{terminator:02X} (expected 0xFF)")

    return all_pass

def main():
    if len(sys.argv) > 1:
        # Command-line argument
        hex_input = ' '.join(sys.argv[1:])
    else:
        # Read from stdin
        hex_input = sys.stdin.read()

    payload = parse_hex_payload(hex_input)

    if not payload:
        print("ERROR: No hex bytes found in input")
        sys.exit(1)

    print(f"Parsed {len(payload)} bytes from payload:")
    print("  " + " ".join(f"0x{b:02X}" for b in payload[:32]))
    if len(payload) > 32:
        print("  " + " ".join(f"0x{b:02X}" for b in payload[32:]))
    print()

    if validate_payload(payload):
        print("\n" + "="*60)
        print("✅ ALL TESTS PASSED")
        print("="*60)
        sys.exit(0)
    else:
        print("\n" + "="*60)
        print("❌ SOME TESTS FAILED")
        print("="*60)
        sys.exit(1)

if __name__ == '__main__':
    main()
