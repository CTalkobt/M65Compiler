#!/usr/bin/env python3
"""Simple test to trace parameter passing via mmemu-mcp"""

import subprocess
import json
import sys

# Create test that calls add_short(10, 20) and writes result to $4000
test_asm = """
.cpu 45gs02
* = $2000

; Initialize
  ldx #0
  txs
  ldy #$01
  tys

; Call add_short(10, 20)
  ldax #10
  push.ax
  ldax #20
  push.ax
  jsr _add_short

; Store result to $4000
  sta $4000
  stx $4001

  brk

_add_short:
  tsx
  stx $1e

  ; Load parameter a (offset 2-3)
  ldx $1e
  lda $0102,x
  tax
  stx $1f
  ldx $1e
  lda $0103,x
  ldx $1f
  sta $20
  stx $21

  ; Load parameter b (offset 4-5)
  ldx $1e
  lda $0104,x
  tax
  stx $1f
  ldx $1e
  lda $0105,x
  ldx $1f
  sta $22
  stx $23

  ; Add them
  lda $20
  clc
  adc $22
  sta $24
  lda $21
  adc $23
  sta $25

  ; Return
  lda $24
  ldx $25
  rts
"""

# Write test
with open('/tmp/trace_test.s', 'w') as f:
    f.write(test_asm)

# Assemble
result = subprocess.run(['ca45', '/tmp/trace_test.s', '-o', '/tmp/trace_test.prg'],
                       capture_output=True, text=True)
if result.returncode != 0:
    print(f"Assembly failed: {result.stderr}")
    sys.exit(1)

print("Assembly successful")
print(f"PRG size: {len(open('/tmp/trace_test.prg', 'rb').read())} bytes")
print("\nExpected result at $4000: 30 (0x1E), or 10 + 20")
print("If result is 0x0000, parameters weren't loaded correctly")
