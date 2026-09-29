#!/usr/bin/env python3
"""
Optimization effectiveness profiler for cc45 compiler.
Measures code size, instruction count, and performance impact of each optimization.
"""

import subprocess
import os
import re
import json
from pathlib import Path
from collections import defaultdict

class OptimizationProfiler:
    def __init__(self, compiler_path="./bin/cc45", test_dir="src/test-resources"):
        self.compiler = compiler_path
        self.test_dir = test_dir
        self.results = defaultdict(lambda: {"o0": {}, "o1": {}, "o2": {}, "o3": {}})

    def compile_test(self, test_file, opt_flags):
        """Compile a test file and return (success, stdout, stderr)"""
        cmd = [self.compiler, test_file] + opt_flags.split() + ["-c", "-o", "/tmp/test.o45"]
        try:
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=5)
            # Also generate assembly for instruction counting
            cmd_asm = [self.compiler, test_file] + opt_flags.split() + ["-S", "-o", "/tmp/test.s45"]
            subprocess.run(cmd_asm, capture_output=True, text=True, timeout=5)
            return result.returncode == 0, result.stdout, result.stderr
        except subprocess.TimeoutExpired:
            return False, "", "Timeout"

    def count_instructions(self, asm_file):
        """Count actual 6502 instructions in assembly"""
        if not os.path.exists(asm_file):
            return 0

        instr_pattern = re.compile(r'^\s+(lda|ldx|ldy|ldz|sta|stx|sty|stz|adc|sbc|and|ora|eor|cmp|bit|clc|sec|cli|sei|clv|cld|sed|pha|pla|phx|plx|phy|ply|jsr|jmp|bne|beq|bcs|bcc|bmi|bpl|bvs|bvc|brk|rti|rts|inc|dec|inx|iny|dex|dey|lsr|asl|ror|rol|tsx|tsy|txs|tys|sax|tax|txa|tay|tya)')

        count = 0
        with open(asm_file, 'r') as f:
            for line in f:
                if instr_pattern.match(line):
                    count += 1
        return count

    def profile_test(self, test_file):
        """Profile a single test file across optimization levels"""
        test_path = os.path.join(self.test_dir, test_file)
        if not os.path.exists(test_path):
            return False

        test_name = os.path.splitext(test_file)[0]
        print(f"Profiling {test_name}...", end=" ", flush=True)

        for opt_level, flags in [("-O0", "-O0"), ("-O1", "-O1"), ("-O2", "-O2"), ("-O3", "-O3")]:
            success, _, _ = self.compile_test(test_path, flags)
            if not success:
                print(f"[{opt_level}: FAIL]", end=" ", flush=True)
                continue

            instr_count = self.count_instructions("/tmp/test.s45")
            obj_size = os.path.getsize("/tmp/test.o45") if os.path.exists("/tmp/test.o45") else 0

            # Use lowercase key without dash (o0, o1, o2, o3)
            key = opt_level[1:].lower()  # Convert "-O1" to "o1"
            self.results[test_name][key] = {
                "instructions": instr_count,
                "object_size": obj_size
            }
            print(f"[{opt_level}: {instr_count} instr]", end=" ", flush=True)

        print("✓")
        return True

    def run_profiling(self):
        """Run profiling on key test files"""
        # Select representative tests
        test_files = [
            "test_constant_folding.c",
            "test_array_init.c",
            "test_cast.c",
            "test_control_flow.c",
            "test_loop_unroll.c",
            "test_array_loop.c",
            "test_bitfield.c",
            "test_compound.c",
        ]

        for test_file in test_files:
            self.profile_test(test_file)

    def generate_report(self):
        """Generate profiling report with analysis"""
        print("\n" + "="*80)
        print("OPTIMIZATION EFFECTIVENESS ANALYSIS")
        print("="*80 + "\n")

        # Summary table
        print(f"{'Test Name':<30} | {'O0':<6} | {'O1':<6} | {'O2':<6} | {'O3':<6} | {'O1 vs O0':<12}")
        print("-" * 85)

        total_o0 = 0
        total_o1 = 0
        total_o2 = 0
        total_o3 = 0
        total_tests = 0

        for test_name in sorted(self.results.keys()):
            data = self.results[test_name]
            o0_instr = data["o0"].get("instructions", 0)
            o1_instr = data["o1"].get("instructions", 0)
            o2_instr = data["o2"].get("instructions", 0)
            o3_instr = data["o3"].get("instructions", 0)

            if o0_instr > 0:
                reduction = ((o0_instr - o1_instr) / o0_instr) * 100
                reduction_str = f"{reduction:+.1f}%"
            else:
                reduction_str = "N/A"

            print(f"{test_name:<30} | {o0_instr:<6} | {o1_instr:<6} | {o2_instr:<6} | {o3_instr:<6} | {reduction_str:<12}")

            if o0_instr > 0:
                total_o0 += o0_instr
                total_o1 += o1_instr
                total_o2 += o2_instr
                total_o3 += o3_instr
                total_tests += 1

        if total_tests > 0:
            print("-" * 85)
            avg_reduction = ((total_o0 - total_o1) / total_o0) * 100
            print(f"{'TOTAL':<30} | {total_o0:<6} | {total_o1:<6} | {total_o2:<6} | {total_o3:<6} | {avg_reduction:+.1f}%")

        print("\n" + "="*80)
        print("KEY FINDINGS:")
        print("="*80)

        # Identify best and worst optimizations
        improvements = {}
        for test_name, data in self.results.items():
            for level in ["o1", "o2", "o3"]:
                if level not in improvements:
                    improvements[level] = []
                o0 = data["o0"].get("instructions", 0)
                opted = data[level].get("instructions", 0)
                if o0 > 0:
                    pct = ((o0 - opted) / o0) * 100
                    improvements[level].append((test_name, pct))

        for level in ["o1", "o2", "o3"]:
            if improvements[level]:
                print(f"\n{level.upper()} improvements (by test):")
                sorted_impr = sorted(improvements[level], key=lambda x: x[1], reverse=True)
                for test, pct in sorted_impr[:3]:
                    print(f"  • {test}: {pct:+.1f}%")

if __name__ == "__main__":
    profiler = OptimizationProfiler()
    profiler.run_profiling()
    profiler.generate_report()
