# Reference Documentation

This directory contains reference materials, planning documents, and investigation notes for the MEGA65 C Compiler Suite. These documents are maintained for historical reference and future development planning.

## Session Documentation

### Phase Completion Reports
- **PHASE1_PHASE2_COMPLETION_SUMMARY.md** — Frame pointer verification and ConstantFolder bug fix (2026-07-19)
- **PHASE3_VERIFICATION_COMPLETE.md** — Compilation and linking verification results
- **PHASE4_DOCUMENTATION_COMPLETE.md** — Documentation updates and v1.1 planning completion
- **SESSION_SUMMARY_2026_07_19.md** — Complete session overview (all 4 phases)

### Investigation & Analysis
- **INVESTIGATION_FINDINGS_2026_07_19.md** — Root cause analysis for variable offset corruption
- **PHASE2_INITIAL_FINDINGS.md** — Initial Phase 2 investigation
- **PHASE2_INVESTIGATION_PLAN.md** — Phase 2 detailed investigation plan
- **PHASE3_INVESTIGATION_PLAN.md** — Phase 3 verification strategy

### Planning & Roadmaps
- **V1_1_OPTIMIZATION_ROADMAP.md** — Comprehensive v1.1 optimization planning (5-15% improvement expected)
- **RELEASE-STEPS.md** — Release management procedures

## Archive & Reference

### Bug Investigation
- **ARRAY_BUG_INVESTIGATION.md** — Array-related bug investigation
- **bug.md** — Bug tracking and notes
- **IR_OPTIMIZER_COMPLETE.md** — IR optimizer development notes

### Performance & Analysis
- **SIZE_COMPARISON.md** — Code size comparison analysis
- **opt.md** — Optimization notes and findings
- **compare.md** — Comparative analysis
- **simplify.md** — Code simplification notes

### Build & Infrastructure
- **DOCKER_BUILD_COMPLETE.md** — Docker build documentation
- **run_mmemu_test.sh** — MMemu test runner script (reference implementation)

## How to Use This Directory

1. **For v1.1 Development**: Start with `V1_1_OPTIMIZATION_ROADMAP.md` for the implementation plan
2. **For Bug Investigation**: Reference the phase completion reports for bug fixes already applied
3. **For Future Sessions**: Each phase report documents the investigation methodology and findings
4. **For Performance Analysis**: Check SIZE_COMPARISON.md and opt.md for performance data

## Status

All documents reflect the v1.0.5 release (2026-07-19). Future development should reference the v1.1 roadmap.

**Last Updated:** 2026-07-19  
**Version:** v1.0.5
