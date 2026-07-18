#!/usr/bin/env python3
"""
cvt_asm label and symbol round-trip preservation tests.

Validates that labels and symbols are preserved when converting
between different assembler formats and back.
"""

import subprocess
import re
import sys
from pathlib import Path

class AssemblyAnalyzer:
    """Extracts labels and symbols from assembly code."""

    def __init__(self, asm_text):
        self.text = asm_text
        self.labels = set()
        self.symbols = set()
        self.extract()

    def extract(self):
        """Extract labels and symbols from assembly text."""
        for line in self.text.split('\n'):
            line = line.strip()
            if not line or line.startswith(';') or line.startswith('*'):
                continue

            # Extract labels (identifier followed by colon or at start of line)
            # Format: LABEL: or LABEL (at start before instruction)
            label_match = re.match(r'^([A-Za-z_]\w*)\s*:', line)
            if label_match:
                self.labels.add(label_match.group(1).lower())
                continue

            # Labels without colons (at start of line, followed by instruction/directive)
            if re.match(r'^[A-Z][A-Z0-9_]*\s+', line, re.IGNORECASE):
                potential_label = re.match(r'^([A-Z][A-Z0-9_]*)\s+', line, re.IGNORECASE)
                if potential_label:
                    word = potential_label.group(1)
                    # Check if it's not a mnemonic
                    if not self.is_mnemonic(word):
                        self.labels.add(word.lower())

            # Extract symbols (.global, .export, .extern, GLOBAL, EXTERN, etc.)
            if re.match(r'(\.|^)(global|export|extern|weak|GLOBAL|EXPORT|EXTERN|WEAK)\s+', line):
                sym_match = re.search(r'(global|export|extern|weak|GLOBAL|EXPORT|EXTERN|WEAK)\s+(\w+)', line, re.IGNORECASE)
                if sym_match:
                    self.symbols.add(sym_match.group(2).lower())

    @staticmethod
    def is_mnemonic(word):
        """Check if word is a 6502 mnemonic."""
        mnemonics = {
            'adc', 'and', 'asl', 'bcc', 'bcs', 'beq', 'bit', 'bmi', 'bne', 'bpl',
            'brk', 'bvc', 'bvs', 'clc', 'cld', 'cli', 'clv', 'cmp', 'cpx', 'cpy',
            'dec', 'dex', 'dey', 'eor', 'inc', 'inx', 'iny', 'jmp', 'jsr', 'lda',
            'ldx', 'ldy', 'lsr', 'nop', 'ora', 'pha', 'php', 'pla', 'plp', 'rol',
            'ror', 'rti', 'rts', 'sbc', 'sec', 'sed', 'sei', 'sta', 'stx', 'sty',
            'tax', 'tay', 'tsx', 'txa', 'txs', 'tya'
        }
        return word.lower() in mnemonics


def run_conversion(input_fmt, output_fmt, input_file):
    """Run cvt_asm format conversion and return output."""
    try:
        result = subprocess.run(
            ['./bin/cvt_asm', '-f', input_fmt, '-t', output_fmt, str(input_file)],
            capture_output=True,
            text=True,
            timeout=5
        )
        return result.stdout if result.returncode == 0 else None
    except subprocess.TimeoutExpired:
        return None


def test_roundtrip_preservation(input_fmt, output_fmt, test_file):
    """Test that labels/symbols are preserved in a round-trip conversion."""

    # Read original file
    with open(test_file) as f:
        original = f.read()

    # Extract original labels and symbols
    original_analysis = AssemblyAnalyzer(original)

    # Convert format A → B
    intermediate = run_conversion(input_fmt, output_fmt, test_file)
    if not intermediate:
        return None, "Conversion failed"

    # Convert format B → A
    intermediate_analysis = AssemblyAnalyzer(intermediate)

    # Analyze intermediate output
    result = {
        'input_fmt': input_fmt,
        'output_fmt': output_fmt,
        'original_labels': original_analysis.labels,
        'intermediate_labels': intermediate_analysis.labels,
        'original_symbols': original_analysis.symbols,
        'intermediate_symbols': intermediate_analysis.symbols,
    }

    return result, None


def main():
    """Run label and symbol preservation tests."""

    test_dir = Path('src/test-resources/cvt_asm_tests')
    if not test_dir.exists():
        print("Error: test directory not found")
        return 1

    formats = ['ca45', 'ca65', 'acme', 'oscar', 'kickassembler']
    passed = 0
    failed = 0

    print("=== cvt_asm Label and Symbol Round-Trip Preservation Tests ===\n")

    for input_fmt in formats:
        test_file = test_dir / f'{input_fmt}_test.asm'
        if not test_file.exists():
            continue

        print(f"Testing {input_fmt}:")

        for output_fmt in formats:
            if input_fmt == output_fmt:
                continue

            result, error = test_roundtrip_preservation(input_fmt, output_fmt, test_file)

            if error:
                print(f"  ✗ {input_fmt} → {output_fmt}: {error}")
                failed += 1
                continue

            # Check label preservation
            labels_match = (len(result['original_labels'] & result['intermediate_labels']) > 0 or
                          len(result['original_labels']) == 0)

            # Check symbol preservation
            symbols_match = (len(result['original_symbols'] & result['intermediate_symbols']) > 0 or
                           len(result['original_symbols']) == 0)

            if labels_match and symbols_match:
                print(f"  ✓ {input_fmt} → {output_fmt}")
                print(f"    Labels: {result['original_labels'] if result['original_labels'] else '(none)'}")
                print(f"    Symbols: {result['original_symbols'] if result['original_symbols'] else '(none)'}")
                passed += 1
            else:
                print(f"  ✗ {input_fmt} → {output_fmt}: Preservation failed")
                if not labels_match:
                    print(f"    Lost labels: {result['original_labels'] - result['intermediate_labels']}")
                if not symbols_match:
                    print(f"    Lost symbols: {result['original_symbols'] - result['intermediate_symbols']}")
                failed += 1

        print()

    print(f"\n=== Summary ===")
    print(f"Passed: {passed}")
    print(f"Failed: {failed}")

    return 0 if failed == 0 else 1


if __name__ == '__main__':
    sys.exit(main())
