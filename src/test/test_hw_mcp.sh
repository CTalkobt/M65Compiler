#!/bin/bash

# test_hw_mcp.sh — Hardware I/O tests for MEGA65 via mmemu MCP
#
# These tests require the mmemu MCP server for mega65 mode with MAP clear.
# The CLI step engine doesn't properly apply MAP instruction changes (mmemu#80).
#
# To run: ensure mmemu MCP server is available, then use Claude Code or
# another MCP client to execute the test sequence:
#
#   1. create_machine mega65
#   2. set_breakpoint $E1C6  (KERNAL main loop)
#   3. run_cpu 10000000      (boot to BASIC READY)
#   4. set_map_state enables=$00  (clear MAP for flat RAM)
#   5. For each test:
#      a. load_image <test.prg>
#      b. set_pc $2000
#      c. run_cpu 5000000
#      d. read_memory $4000 <size>
#      e. Compare with expected values
#   6. destroy_machine
#
# Expected results (verified via MCP):
#
# VIC-IV test (test_vic4_mmemu.prg):
#   $4000: AA BB CC DD 0F 11 22 E1 FF E3 C1 99 C3 00 06 0F
#   $4010: 10 40 04
#
# SID/CIA test (test_sid_cia_mmemu.prg):
#   $4000: 0C 11 11 09 0F AA BB 80 05 FF xx 03 09 xx 55 11
#   $4010: 11 01
#   (bytes 10,13 are CIA timer values — timing-dependent)
#
# DMA/math test (test_hw_extra_mmemu.prg):
#   $4000: 01 06 41 44 10 xx 30 xx 64 07 01 0A 05 FF A0 00
#   $4010: 03 80
#   (bytes 5,7 are DMA/math registers — may vary)
#
# Device test (test_devices_mmemu.prg):
#   $4000: xx xx xx xx 10 20 30 40 01 06 5A 00 xx 40 01 06
#   (bytes 0-3 FDC/SD, byte 12 Ethernet — device-dependent)
#
# Keyboard test (test_keyboard_mmemu.prg):
#   $4000: 27 08 11 20 3F 00 00 xx 00 xx 00
#   (bytes 7,9 depend on key state — need press_key MCP)

echo "Hardware I/O tests require mmemu MCP server (mega65 mode)."
echo "See comments in this script for the MCP test sequence."
echo "CLI-based testing uses rawMega65 mode (no I/O emulation) — see mmemu#79, #80."
exit 0
