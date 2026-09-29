#!/usr/bin/env python3
"""
Dual-metric optimization analysis: Space vs Speed ROI.
Measures both code size (space) and approximate execution cost (speed).
"""

import subprocess
import os
import re
from collections import defaultdict

class DualMetricProfiler:
    def __init__(self, compiler_path="./bin/cc45", test_dir="src/test-resources"):
        self.compiler = compiler_path
        self.test_dir = test_dir
        self.results = defaultdict(lambda: {"o0": {}, "o1": {}, "o2": {}, "o3": {}})

        # Instruction cycle costs (approximate for 45GS02)
        self.cycles = {
            # Load/Store
            'lda': 2, 'ldx': 2, 'ldy': 2, 'ldz': 2,
            'sta': 3, 'stx': 3, 'sty': 3, 'stz': 3,
            # Arithmetic
            'adc': 2, 'sbc': 2, 'add.16': 4, 'sub.16': 4,
            'mul': 4, 'mul.16': 20, 'div.16': 30,  # Multiply/divide are expensive
            'asl': 2, 'lsr': 2, 'rol': 2, 'ror': 2,
            # Bitwise
            'and': 2, 'and.16': 4, 'ora': 2, 'ora.16': 4, 'eor': 2, 'eor.16': 4,
            # Branches
            'beq': 3, 'bne': 3, 'bcs': 3, 'bcc': 3, 'bmi': 3, 'bpl': 3,
            'jsr': 6, 'jmp': 3, 'rts': 6,
            # Register ops
            'inx': 2, 'iny': 2, 'dex': 2, 'dey': 2, 'inc': 6,
            'tax': 2, 'txa': 2, 'tay': 2, 'tya': 2,
        }

    def compile_test(self, test_file, opt_flags):
        """Compile test file"""
        cmd = [self.compiler, test_file] + opt_flags.split() + ["-S", "-o", "/tmp/test.s45"]
        try:
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=5)
            return result.returncode == 0
        except subprocess.TimeoutExpired:
            return False

    def count_instructions(self, asm_file):
        """Count instruction bytes and cycle cost"""
        if not os.path.exists(asm_file):
            return 0, 0

        instr_pattern = re.compile(r'^\s+([a-z0-9.]+)')
        total_bytes = 0
        total_cycles = 0
        instr_count = 0

        with open(asm_file, 'r') as f:
            for line in f:
                match = instr_pattern.match(line)
                if match:
                    instr = match.group(1)
                    instr_count += 1

                    # Estimate instruction size (6502 is 1-3 bytes per instruction)
                    if instr in ['jsr', 'jmp', 'beq', 'bne', 'bcs', 'bcc', 'bmi', 'bpl']:
                        total_bytes += 3  # 3-byte instructions
                    elif any(x in instr for x in ['.16', '.32', 'add', 'sub', 'mul', 'div']):
                        total_bytes += 2  # Extended ops
                    else:
                        total_bytes += 2  # Most ops are 2 bytes

                    # Add cycle cost
                    for instr_name, cost in self.cycles.items():
                        if instr.startswith(instr_name):
                            total_cycles += cost
                            break
                    else:
                        total_cycles += 2  # Default 2 cycles

        return total_bytes, total_cycles

    def profile_test(self, test_file):
        """Profile single test across optimization levels"""
        test_path = os.path.join(self.test_dir, test_file)
        if not os.path.exists(test_path):
            return False

        test_name = os.path.splitext(test_file)[0]
        print(f"{test_name:<30}", end=" ", flush=True)

        for opt_level, flags in [("-O0", "-O0"), ("-O1", "-O1")]:
            if not self.compile_test(test_path, flags):
                print(f"[{opt_level}: FAIL]", end=" ", flush=True)
                continue

            bytes_count, cycles = self.count_instructions("/tmp/test.s45")
            key = opt_level[1:].lower()
            self.results[test_name][key] = {
                "bytes": bytes_count,
                "cycles": cycles
            }
            print(f"[{opt_level}: {bytes_count}B, {cycles}cy]", end=" ", flush=True)

        print()
        return True

    def run_profiling(self):
        """Profile key tests"""
        tests = [
            "test_constant_folding.c",
            "test_bitfield.c",
            "test_cast.c",
            "test_compound.c",
            "test_array_loop.c",
            "test_control_flow.c",
        ]
        for test in tests:
            self.profile_test(test)

    def generate_report(self):
        """Generate dual-metric ROI report"""
        print("\n" + "="*100)
        print("DUAL-METRIC OPTIMIZATION ROI: SPACE vs SPEED")
        print("="*100 + "\n")

        print(f"{'Test':<25} | {'O0 Bytes':<8} | {'O1 Bytes':<8} | {'Space %':<10} | {'O0 Cycles':<10} | {'O1 Cycles':<10} | {'Speed %':<10}")
        print("-" * 100)

        total_o0_bytes = 0
        total_o1_bytes = 0
        total_o0_cycles = 0
        total_o1_cycles = 0
        test_count = 0

        for test_name in sorted(self.results.keys()):
            data = self.results[test_name]
            o0_bytes = data["o0"].get("bytes", 0)
            o1_bytes = data["o1"].get("bytes", 0)
            o0_cycles = data["o0"].get("cycles", 0)
            o1_cycles = data["o1"].get("cycles", 0)

            if o0_bytes > 0:
                space_pct = ((o0_bytes - o1_bytes) / o0_bytes) * 100
            else:
                space_pct = 0

            if o0_cycles > 0:
                speed_pct = ((o0_cycles - o1_cycles) / o0_cycles) * 100
            else:
                speed_pct = 0

            print(f"{test_name:<25} | {o0_bytes:<8} | {o1_bytes:<8} | {space_pct:+7.1f}% | {o0_cycles:<10} | {o1_cycles:<10} | {speed_pct:+7.1f}%")

            if o0_bytes > 0:
                total_o0_bytes += o0_bytes
                total_o1_bytes += o1_bytes
                total_o0_cycles += o0_cycles
                total_o1_cycles += o1_cycles
                test_count += 1

        print("-" * 100)
        if total_o0_bytes > 0:
            avg_space = ((total_o0_bytes - total_o1_bytes) / total_o0_bytes) * 100
            avg_speed = ((total_o0_cycles - total_o1_cycles) / total_o0_cycles) * 100
            print(f"{'TOTAL':<25} | {total_o0_bytes:<8} | {total_o1_bytes:<8} | {avg_space:+7.1f}% | {total_o0_cycles:<10} | {total_o1_cycles:<10} | {avg_speed:+7.1f}%")

        print("\n" + "="*100)
        print("TRADEOFF ANALYSIS")
        print("="*100 + "\n")

        print("Space vs Speed ROI by test:")
        print("(Positive = smaller code and faster; Negative = larger code but faster)\n")

        for test_name in sorted(self.results.keys()):
            data = self.results[test_name]
            o0_bytes = data["o0"].get("bytes", 0)
            o1_bytes = data["o1"].get("bytes", 0)
            o0_cycles = data["o0"].get("cycles", 0)
            o1_cycles = data["o1"].get("cycles", 0)

            if o0_bytes > 0 and o0_cycles > 0:
                space_pct = ((o0_bytes - o1_bytes) / o0_bytes) * 100
                speed_pct = ((o0_cycles - o1_cycles) / o0_cycles) * 100

                # ROI score: positive if both win, negative if tradeoff
                roi_score = space_pct + speed_pct

                if space_pct > 0 and speed_pct > 0:
                    verdict = "✅ BOTH WIN"
                elif space_pct > 0 and speed_pct <= 0:
                    verdict = "⚠️ SPACE WINS (speed cost)"
                elif space_pct <= 0 and speed_pct > 0:
                    verdict = "⚠️ SPEED WINS (space cost)"
                else:
                    verdict = "❌ BOTH LOSE"

                print(f"{test_name:<25} | Space: {space_pct:+6.1f}% | Speed: {speed_pct:+6.1f}% | {verdict}")

if __name__ == "__main__":
    profiler = DualMetricProfiler()
    profiler.run_profiling()
    profiler.generate_report()
