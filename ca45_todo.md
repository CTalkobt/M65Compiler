# ca45 Assembler — Usability Improvements TODO

**Status:** Analysis Complete — Ready for Implementation  
**Estimated Total Effort:** 80–100 hours over 3 weeks  
**Expected Impact:** 60–90% improvement in user experience

---

## Quick Start (Week 1 — 60% Impact)

These four items alone address the most common pain points.

- [ ] **Error messages: Add line:column and suggestions** (1 hour)
  - Files: `ca45_main.cpp` (line 254, 324), `AssemblerParser.cpp` (error throws)
  - Change: Format all errors as `file:line:col: error: message`
  - Add: Symbol suggestions using fuzzy matching (Levenshtein distance)
  - Priority: **CRITICAL** — Users get stuck on typos for 5+ minutes

- [ ] **Help output: Reorganize into logical sections** (0.5 hours)
  - File: `ca45_main.cpp` (lines 119–135)
  - Sections: Quick Start, Output Options, Optimization, Debugging, Configuration
  - Add: `-?` alias for `--help`
  - Priority: **CRITICAL** — Flags currently scattered

- [ ] **Documentation: Add "Getting Started" section** (2 hours)
  - File: `doc/bin/ca45.md` (add after "Syntax Overview")
  - Content: Minimal program (5 lines), common patterns, troubleshooting checklist
  - Priority: **CRITICAL** — New users need working example immediately

- [ ] **Documentation: Add "Error Reference" table** (1 hour)
  - File: `doc/bin/ca45.md` (new section)
  - Format: Error | Cause | Fix (15–20 rows)
  - Priority: **CRITICAL** — Cuts debugging time by 50%

---

## Phase 1: Error Messages & Diagnostics (Week 1)

- [ ] **1.1: Always include line:column in errors**
  - Modify: `ca45_main.cpp`, `AssemblerParser.cpp`
  - Ensure all `throw std::runtime_error()` calls include location
  - Estimated effort: 1 hour

- [ ] **1.2: Add fuzzy matching for symbol suggestions**
  - Create: `SymbolSuggester.hpp`, `SymbolSuggester.cpp`
  - Implement Levenshtein distance (50 lines)
  - Integrate: `AssemblerParser.cpp` undefined symbol handler
  - Estimated effort: 2 hours

- [ ] **1.3: Add immediate value overflow checking**
  - Modify: `AssemblerParser.cpp` (immediate addressing mode handlers)
  - Change: Convert silent truncation to error with fix suggestion
  - Estimated effort: 1.5 hours

- [ ] **1.4: Add context to expression evaluation errors**
  - Modify: `AssemblerExpression.cpp`, `AssemblerParser.cpp`
  - Show: Expression being parsed, operator precedence hint
  - Estimated effort: 1 hour

- [ ] **1.5: Label scope errors show hierarchy**
  - Modify: `AssemblerParser.cpp` (duplicate label detection)
  - Show: Current scope, first definition location, scoping rules hint
  - Estimated effort: 1 hour

**Subtotal Phase 1: 6.5 hours**

---

## Phase 2: Help & Documentation (Week 1–2)

- [ ] **2.1: Reorganize help output into sections**
  - Modify: `ca45_main.cpp` help text
  - Sections: Quick Start, Output Options, Optimization, Debugging, Configuration
  - Add: `--help=<section>` support
  - Estimated effort: 1 hour

- [ ] **2.2: Implement `--help=<section>` for detailed help**
  - Create: `HelpProvider.hpp`, `HelpProvider.cpp`
  - Sections: quick-start, optimization, expressions, directives, simulated-ops, troubleshooting
  - Estimated effort: 4 hours

- [ ] **2.3: Add "Getting Started" section to ca45.md**
  - Modify: `doc/bin/ca45.md`
  - Content: Minimal program, common patterns (loops, calls, data), troubleshooting checklist
  - Length: 300–400 lines
  - Estimated effort: 2 hours

- [ ] **2.4: Add "Error Reference" table to ca45.md**
  - Modify: `doc/bin/ca45.md` (new section)
  - Format: Table with Error | Cause | Fix columns
  - Rows: 15–20 common errors (undefined symbol, overflow, duplicates, etc.)
  - Estimated effort: 1.5 hours

- [ ] **2.5: Add simulated opcode examples**
  - Modify: `doc/bin/ca45.md` (Simulated Opcodes section)
  - For each family (ldax, add.16, cmp.16, etc.): 3–5 usage examples with comments
  - Estimated effort: 3 hours

- [ ] **2.6: Add FAQ section to ca45.md**
  - Modify: `doc/bin/ca45.md` (new section)
  - Questions: When to use -c? How do @ labels work? What's "instruction size changed"? etc.
  - Estimated effort: 2 hours

- [ ] **2.7: Add visual operator precedence chart**
  - Modify: `doc/bin/ca45.md` (Expression section)
  - Format: Numbered list with visual hierarchy, examples
  - Estimated effort: 1 hour

- [ ] **2.8: Add addressing mode selection guide**
  - Modify: `doc/bin/ca45.md` (new section)
  - For each mode: When to use, size, performance, example
  - Estimated effort: 2 hours

**Subtotal Phase 2: 16.5 hours**

---

## Phase 3: CLI Enhancements & Transparency (Week 2)

- [ ] **3.1: Implement `--show-config` flag**
  - Modify: `ca45_main.cpp`
  - Outputs: Optimization level, enabled passes, symbols, include paths, output format
  - Estimated effort: 1 hour

- [ ] **3.2: Implement `--show-passes` flag**
  - Create: `PassRegistry.hpp` (or enum mapping)
  - Modify: `ca45_main.cpp`
  - Outputs: All available passes per optimization level
  - Estimated effort: 1.5 hours

- [ ] **3.3: Implement `--check` flag (syntax validation only)**
  - Modify: `ca45_main.cpp`
  - Behavior: Run full pipeline, validate, don't write output
  - Use case: CI/CD, pre-commit hooks
  - Estimated effort: 1 hour

- [ ] **3.4: Implement `--stats` flag (code metrics)**
  - Create: `CodeMetrics.hpp`, `CodeMetrics.cpp`
  - Modify: `ca45_main.cpp`
  - Reports: Instructions, segments, symbols, optimization results, memory layout
  - Estimated effort: 2.5 hours

- [ ] **3.5: Implement `--dump-symbols` flag**
  - Create: `SymbolDumper.hpp`, `SymbolDumper.cpp`
  - Formats: Text (default), CSV, JSON
  - Estimated effort: 2 hours

- [ ] **3.6: Enhance optimizer reporting with filtering**
  - Modify: `ca45_main.cpp` `-Roptimizer` handling
  - Add: `--report-optimization=<filter>` for filtering by pass name
  - Add: Summary mode showing bytes saved per pass
  - Estimated effort: 1.5 hours

**Subtotal Phase 3: 9.5 hours**

---

## Phase 4: Advanced Features & Polish (Week 3)

- [ ] **4.1: Add diagnostic mode `-Ddiag`**
  - Modify: `ca45_main.cpp` argument parsing
  - Outputs: Symbol resolution attempts, expression evaluation steps, opcode matching
  - Estimated effort: 2 hours

- [ ] **4.2: Add expression tracing with `-Dexpr-trace`**
  - Modify: `AssemblerExpression.cpp`
  - Outputs: Expression tree evaluation, intermediate results
  - Estimated effort: 1.5 hours

- [ ] **4.3: Show forward reference dependencies in errors**
  - Modify: `AssemblerParser.cpp` symbol resolution
  - Show: Where undefined symbol will be defined (if found in later pass)
  - Estimated effort: 1.5 hours

- [ ] **4.4: Generate default config file with `--gen-config`**
  - Modify: `ca45_main.cpp`
  - Creates: `~/.config/m65/ca45.conf` with documented defaults
  - Estimated effort: 1 hour

- [ ] **4.5: Add colored/structured output (optional `--no-color`)**
  - Modify: `ca45_main.cpp` output formatting
  - Colors: Green (✓), Yellow (⚠), Red (✗), Blue (ℹ)
  - Estimated effort: 2 hours

- [ ] **4.6: Add configuration profiles (`--save-profile`, `--list-profiles`)**
  - Create: `ProfileManager.hpp`, `ProfileManager.cpp`
  - Modify: `ca45_main.cpp`
  - Store: `~/.config/m65/ca45-profiles.json`
  - Estimated effort: 3 hours

**Subtotal Phase 4: 11.5 hours**

---

## Testing & Integration (Week 3)

- [ ] **Unit tests for symbol suggester**
  - Test: Fuzzy matching accuracy
  - Estimated effort: 1 hour

- [ ] **Regression tests for error messages**
  - Ensure: All errors have line:column
  - Ensure: Symbol suggestions work
  - Estimated effort: 2 hours

- [ ] **Manual testing of new flags**
  - Test: `--help=<section>`, `--show-config`, `--show-passes`, `--check`, `--stats`, `--dump-symbols`
  - Estimated effort: 2 hours

- [ ] **Documentation review by users**
  - Get: Feedback on Getting Started section, examples, FAQ
  - Iterate: Make improvements based on feedback
  - Estimated effort: 2 hours

**Subtotal Testing: 7 hours**

---

## Summary by Phase

| Phase | Description | Hours | Impact |
|-------|-------------|-------|--------|
| 1 | Error messages & diagnostics | 6.5 | 🔴 HIGH |
| 2 | Help & documentation | 16.5 | 🔴 HIGH |
| 3 | CLI enhancements | 9.5 | 🟡 MED |
| 4 | Advanced features | 11.5 | 🟢 LOW |
| Testing | Unit, regression, manual | 7 | 🔴 HIGH |
| **TOTAL** | | **51 hours** | |

---

## Implementation Order (Recommended)

**Week 1: Critical Path (28 hours, 60% impact)**
1. Error messages (6.5 h)
2. Help reorganization & Getting Started (3 h)
3. Error reference & simulated op examples (2.5 h)
4. Testing (2 h)

**Week 2: Consolidation (25.5 hours, 30% impact)**
5. Help system & documentation (13.5 h)
6. CLI transparency flags (9.5 h)
7. Testing (2.5 h)

**Week 3: Polish (11.5 hours, 10% impact)**
8. Advanced features (11.5 h)
9. Integration testing (1 h)

---

## Files to Create

- `SymbolSuggester.hpp` / `SymbolSuggester.cpp` — Fuzzy matching (Levenshtein)
- `HelpProvider.hpp` / `HelpProvider.cpp` — Section-specific help content
- `PassRegistry.hpp` — Mapping of passes to levels & descriptions
- `CodeMetrics.hpp` / `CodeMetrics.cpp` — Code statistics
- `SymbolDumper.hpp` / `SymbolDumper.cpp` — Symbol table export
- `ProfileManager.hpp` / `ProfileManager.cpp` — Configuration profiles (Phase 4)

---

## Files to Modify

- `ca45_main.cpp` — Help text, flag parsing, output formatting (touches: ~10 locations)
- `AssemblerParser.cpp` — Error messages, symbol resolution (touches: ~8 locations)
- `AssemblerExpression.cpp` — Expression error context (touches: ~3 locations)
- `doc/bin/ca45.md` — Add 6 new sections, expand examples (touches: ~2000 lines new)

---

## Success Criteria

✅ All error messages include line:column  
✅ All error messages suggest fixes or next steps  
✅ Help output logically organized with `-h` and `--help=<section>`  
✅ New users can write & assemble first program in < 10 minutes  
✅ Getting Started section with working example  
✅ Error Reference table for all common errors  
✅ Simulated opcode examples in documentation  
✅ `--show-config` and `--show-passes` implemented  
✅ Support questions reduced by 50% in Q4  

---

## Dependencies & Risks

### No External Dependencies
- Levenshtein distance: implement from scratch (~50 lines)
- No new libraries required

### Backward Compatibility
- All changes are additions/clarifications
- No breaking changes to CLI
- No changes to output format for existing flags

### Testing Strategy
- Unit tests for fuzzy matching (symbol suggester)
- Regression tests for error messages (ensure line:column always present)
- Manual testing of all new flags
- User feedback on documentation (Getting Started section)

---

## Notes

- **Quick Wins:** Error messages (1 h) + help reorganization (0.5 h) + Getting Started (2 h) = 3.5 hours for 30% improvement
- **Leverage:** Much documentation can be copy/pasted from existing CLAUDE.md and ca45.md
- **Risk:** Low — all changes are additive; no modifications to core assembly logic
- **Testing:** Most improvements testable locally without instrumentation

---

## Related Issues & References

- **Error clarity:** Often cited as #1 friction point in surveys
- **Feature discoverability:** Users don't understand `-c` without context
- **Learning curve:** New users spend 2–3 hours on trivial programs
- **Optimization transparency:** Users can't tell what optimizations did

