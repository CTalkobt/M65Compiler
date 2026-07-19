# Reference Documentation

This directory contains active reference materials and planning documents for the MEGA65 C Compiler Suite. Use these documents for current and upcoming development.

## Active Planning & Implementation

### v1.1 Optimization Roadmap ⭐
**File:** `V1_1_OPTIMIZATION_ROADMAP.md`

Comprehensive implementation plan for v1.1 with expected 5-15% code size reduction:
- 5 major optimization opportunities (Lazy FP, Smart FP Recalculation, Frame Allocation, Bitfields, Calling Convention)
- 4-phase implementation plan (2-3 weeks, 80-120 hours)
- Risk mitigation strategies and testing plan
- Success criteria and timeline

**Start here for the next development sprint.**

### Release Management
**File:** `RELEASE-STEPS.md`

Checklist for preparing and publishing releases:
- Pre-release validation steps
- Tag and changelog procedures
- Distribution and announcement steps

## Development Tools

### MMemu Test Runner
**File:** `run_mmemu_test.sh`

Test execution script for validating compiled programs in the MMemu emulator.

## Historical Work

See `doc/archive/` for:
- Phase completion reports (PHASE 1-5)
- Session summaries and investigation notes
- Bug investigation archives
- Performance analysis and comparisons
- Prior optimization work

## How to Use

1. **Starting v1.1 Work:** Read `V1_1_OPTIMIZATION_ROADMAP.md` for the implementation plan
2. **Preparing a Release:** Use `RELEASE-STEPS.md` as a checklist
3. **Historical Context:** Check `doc/archive/` for prior investigation methodology and findings

## Status

**Current Version:** v1.0.5 (2026-07-19)  
**Next Version:** v1.1 (planned, implementation roadmap ready)  
**Last Updated:** 2026-07-19
