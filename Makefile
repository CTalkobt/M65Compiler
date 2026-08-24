CX = g++
GIT_HASH := $(shell git rev-parse --short HEAD 2>/dev/null || echo "unknown")
EXTRA_CXXFLAGS ?=
CXXFLAGS = -std=c++17 -Wall -Wextra -Iinclude -MMD -MP -DGIT_HASH='"$(GIT_HASH)"' $(EXTRA_CXXFLAGS)

# Optional FUSE3 support for disk45 mount command
HAVE_FUSE3 := $(shell pkg-config --exists fuse3 2>/dev/null && echo 1)
ifeq ($(HAVE_FUSE3),1)
  FUSE3_CFLAGS := $(shell pkg-config --cflags fuse3) -DHAVE_FUSE3
  FUSE3_LIBS := $(shell pkg-config --libs fuse3)
endif
SRC_DIR = src/main
OBJ_DIR = obj
BIN_DIR = bin
LIB_DIR = lib45

CC_TARGET = $(BIN_DIR)/cc45
CA_TARGET = $(BIN_DIR)/ca45
CP_TARGET = $(BIN_DIR)/cp45
NM_TARGET = $(BIN_DIR)/nm45
LN_TARGET = $(BIN_DIR)/ln45
AR_TARGET = $(BIN_DIR)/ar45
OD_TARGET = $(BIN_DIR)/objdump45
DISK_TARGET = $(BIN_DIR)/disk45
CVT_ASM_TARGET = $(BIN_DIR)/cvt_asm
BASIC_TARGET = $(BIN_DIR)/basic45

MAN_DIR = man

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
LIBDIR ?= $(PREFIX)/lib/cc45
INCDIR ?= $(PREFIX)/include/cc45
MANDIR ?= $(PREFIX)/share/man/man1

# ============================================================================
# lib45 LIBRARY DEFINITIONS
# ============================================================================

# lib45-common: Object format, symbols, config, utilities, diagnostics, debug info, source tracking
$(LIB_DIR)/lib45-common.a: $(addprefix $(OBJ_DIR)/, \
    O45Reader.o O45Writer.o O45IRSerializer.o O45Emitter.o O45Archive.o \
    O45Linker.o AssemblerOpcodeDatabase.o ConfigLoader.o EnhancedDiagnostic.o \
    DebugInfoBuilder.o DIEBuilder.o SourceLocationTracker.o LineNumberProgram.o)
	@mkdir -p $(LIB_DIR)
	$(AR) rcs $@ $^

# lib45-c-compile: C compiler frontend (Lexer, Parser, AST, Validator)
$(LIB_DIR)/lib45-c-compile.a: $(addprefix $(OBJ_DIR)/, \
    Lexer.o Parser.o AST.o Preprocessor.o TypeSystem.o ScopeManager.o \
    TypeInfo.o M65Emitter.o)
	@mkdir -p $(LIB_DIR)
	$(AR) rcs $@ $^

# lib45-ir: Intermediate representation (basic IR structures, no codegen)
$(LIB_DIR)/lib45-ir.a: $(addprefix $(OBJ_DIR)/, \
    IRBuilder.o IRPrinter.o IROptimizer.o VRegAllocator.o)
	@mkdir -p $(LIB_DIR)
	$(AR) rcs $@ $^

# lib45-opt: Optimization passes + code generation
$(LIB_DIR)/lib45-opt.a: $(addprefix $(OBJ_DIR)/, \
    IRCodeGen.o CodeGenerator.o \
    ConstantFolder.o LoopOptimizer.o LoopIdiomRegistry.o \
    LoopInterchange.o StrengthReduction.o AlgebraicSimplification.o \
    RedundantLoadElimination.o DeadStoreElimination.o \
    CommonSubexpressionElimination.o LoopInvariantCodeMotion.o \
    CopyPropagation.o BranchOptimization.o GlobalValueNumbering.o \
    EscapeAnalysis.o OptimizationSelector.o InlineSelector.o \
    CallGraphAnalyzer.o DevirtualizationDetector.o FunctionAnalyzer.o \
    CoOptimizationSelector.o DevirtualizationHints.o CoOptimizationApplier.o \
    OptimizationCatalog.o OptimizationController.o \
    GlobalFunctionDatabase.o CallPatternAnalyzer.o IPOAnalyzer.o IPOProfiler.o \
    SpecializationCodeGenerator.o SpecializationOptimizer.o \
    IRSpecializationGenerator.o StructFieldStriper.o FieldStripedOffsetCalc.o \
    AddressSpaceValidator.o GlobalPointerFieldDatabase.o \
    InterTUPatternDetector.o FieldCachingAnalyzer.o \
    FieldCachingLinkerIntegration.o FarAddressMemorySupport.o \
    FarAddressCodeGenerator.o CrossModuleVariableDatabase.o \
    MemoryBankAssigner.o BankLayoutGenerator.o BankSetupOptimizer.o \
    BankAwareCodeGenerator.o OptimizationHintCollector.o \
    OptimizationConstraintResolver.o LinkTimeOptimizationCoordinator.o \
    OptimizationDependencyAnalyzer.o LinkTimeOptimizationCodeGenerator.o \
    LinkTimeOptimizationValidator.o CompilationProfiler.o HotSpotProfiler.o \
    ComparativePerformanceAnalyzer.o PatternRecognitionEngine.o \
    PatternBasedOptimizationSelector.o CrossModuleEnhancer.o \
    DependencyTracker.o BenchmarkingSuite.o BenchmarkCompilerIntegration.o \
    BenchmarkExecutor.o BenchmarkMetricsCollector.o \
    BenchmarkComparativeAnalyzer.o BenchmarkReportGenerator.o \
    IterationManager.o OptimizationLearner.o FeedbackCoordinator.o \
    OnlineLearner.o TuningHooks.o CompilationSignalCollector.o \
    CompilerHookIntegrator.o HookIntegration.o CompilerDecisionLogic.o \
    OptimizationEffectivenessCollector.o OptimizationProfileDatabase.o \
    AdaptiveThresholdAdjuster.o OptimizationPatternAnalyzer.o \
    LearnerBasedOptimizationSelector.o AdaptiveLearnerIntegration.o \
    TemplateOptimizationPass.o TemplateRegistry.o TemplateOptimizationSystem.o \
    MathLibraryOptimization.o RegisterResidentLoops.o TableDrivenDispatch.o \
    PeepholeOptimization.o LearnerFeedbackRecorder.o AddressTemplates.o \
    AddressTemplateDetector.o)
	@mkdir -p $(LIB_DIR)
	$(AR) rcs $@ $^

# lib45-codegen: Code generation & assembly (includes preprocessor & emitter)
$(LIB_DIR)/lib45-codegen.a: $(addprefix $(OBJ_DIR)/, \
    Preprocessor.o M65Emitter.o \
    AssemblerLexer.o AssemblerParser.o AssemblerExpression.o SymbolSuggester.o \
    AssemblerOptimizer.o AssemblerSimulatedOps.o AssemblerGenerator.o OpEffect.o)
	@mkdir -p $(LIB_DIR)
	$(AR) rcs $@ $^

# lib45-linker: Linking and relocation
$(LIB_DIR)/lib45-linker.a: $(addprefix $(OBJ_DIR)/, \
    CrossModuleOptimizer.o)
	@mkdir -p $(LIB_DIR)
	$(AR) rcs $@ $^

# lib45-basic: BASIC language features
$(LIB_DIR)/lib45-basic.a: $(addprefix $(OBJ_DIR)/, \
    BasicTokenizer.o PETSCIIEncoder.o BasicEmitter.o BasicPreprocessor.o \
    BasicDocGenerator.o BasicValidator.o LabelBasedSourceParser.o \
    SymbolExpressionEvaluator.o BasicMinifier.o BasicTooling.o \
    BasicCharacterMap.o BasicDocumentationGenerator.o BasicStructures.o \
    BasicArchiveSupport.o BasicStandardLibrary.o)
	@mkdir -p $(LIB_DIR)
	$(AR) rcs $@ $^

# lib45-tools: Disk utilities, format converters (includes optional FUSE3 support)
LIB45_TOOLS_OBJS = $(addprefix $(OBJ_DIR)/, \
    disk45_catalog.o DiskImage.o DiskImageFactory.o BAMOperations.o \
    D64Image.o D71Image.o D81Image.o D65Image.o ArkImage.o ArcImage.o \
    LnxImage.o TapImage.o T64Image.o G64Image.o D80Image.o GeosCvtImage.o \
    P00Image.o X64Image.o ZipcodeImage.o D90Image.o CmdImage.o NibImage.o \
    GzipHelper.o AsmParser.o AsmWriter.o Ca45Parser.o Ca45Writer.o \
    Ca65Parser.o Ca65Writer.o AcmeParser.o AcmeWriter.o OscarParser.o \
    OscarWriter.o Merlin64Parser.o Merlin64Writer.o X65Parser.o X65Writer.o \
    FormatDetection.o KickAssemblerParser.o KickAssemblerWriter.o)

ifeq ($(HAVE_FUSE3),1)
  LIB45_TOOLS_OBJS += $(OBJ_DIR)/disk45_fuse.o
endif

$(LIB_DIR)/lib45-tools.a: $(LIB45_TOOLS_OBJS)
	@mkdir -p $(LIB_DIR)
	$(AR) rcs $@ $^

# ============================================================================
# BINARY BUILD RULES
# ============================================================================

all: $(CC_TARGET) $(CA_TARGET) $(CP_TARGET) $(NM_TARGET) $(LN_TARGET) \
     $(AR_TARGET) $(OD_TARGET) $(DISK_TARGET) $(CVT_ASM_TARGET) $(BASIC_TARGET)

man: $(MAN_DIR)/cc45.1 $(MAN_DIR)/ca45.1 $(MAN_DIR)/cp45.1 $(MAN_DIR)/ln45.1 $(MAN_DIR)/nm45.1 $(MAN_DIR)/ar45.1 $(MAN_DIR)/objdump45.1

$(MAN_DIR)/%.1: doc/bin/%.md
	@mkdir -p $(MAN_DIR)
	pandoc -s -t man $< -o $@ -M title="$(basename $(notdir $@))" -M section="1" -M date="$(shell date +%F)" -M footer="$(basename $(notdir $@)) manual" -M header="User Commands"

# Define pipeline object files (compiler-only)
PIPELINE_OBJECTS = $(addprefix $(OBJ_DIR)/, \
    CompilationPipeline.o PreprocessStage.o ParseStage.o OptimizeStage.o \
    CodegenStage.o AssemblyStage.o LinkingStage.o)

# C Compiler (cc45) - uses all libraries plus pipeline objects
# Note: Link order matters - list libraries twice to resolve circular dependencies
$(CC_TARGET): $(OBJ_DIR)/cc45_main.o $(PIPELINE_OBJECTS) | $(LIB_DIR)
$(CC_TARGET): $(LIB_DIR)/lib45-opt.a $(LIB_DIR)/lib45-codegen.a \
              $(LIB_DIR)/lib45-ir.a $(LIB_DIR)/lib45-c-compile.a $(LIB_DIR)/lib45-common.a
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ_DIR)/cc45_main.o $(PIPELINE_OBJECTS) \
	  $(LIB_DIR)/lib45-opt.a $(LIB_DIR)/lib45-codegen.a $(LIB_DIR)/lib45-ir.a \
	  $(LIB_DIR)/lib45-c-compile.a $(LIB_DIR)/lib45-common.a \
	  $(LIB_DIR)/lib45-opt.a $(LIB_DIR)/lib45-ir.a

# Preprocessor (cp45) - symlink to cc45
$(CP_TARGET): $(CC_TARGET)
	@mkdir -p $(BIN_DIR)
	ln -sf cc45 $(CP_TARGET)

# Assembler (ca45) - uses codegen + common
$(CA_TARGET): $(OBJ_DIR)/ca45_main.o | $(LIB_DIR)
$(CA_TARGET): $(LIB_DIR)/lib45-codegen.a $(LIB_DIR)/lib45-common.a
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ_DIR)/ca45_main.o $(LIB_DIR)/lib45-codegen.a $(LIB_DIR)/lib45-common.a

# Symbol Inspector (nm45) - uses common
$(NM_TARGET): $(OBJ_DIR)/nm45_main.o | $(LIB_DIR)
$(NM_TARGET): $(LIB_DIR)/lib45-common.a
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ_DIR)/nm45_main.o $(LIB_DIR)/lib45-common.a

# Linker (ln45) - uses linker + opt + common
$(LN_TARGET): $(OBJ_DIR)/ln45_main.o | $(LIB_DIR)
$(LN_TARGET): $(LIB_DIR)/lib45-linker.a $(LIB_DIR)/lib45-opt.a $(LIB_DIR)/lib45-common.a
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ_DIR)/ln45_main.o $(LIB_DIR)/lib45-linker.a $(LIB_DIR)/lib45-opt.a $(LIB_DIR)/lib45-common.a

# Archiver (ar45) - uses common
$(AR_TARGET): $(OBJ_DIR)/ar45_main.o | $(LIB_DIR)
$(AR_TARGET): $(LIB_DIR)/lib45-common.a
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ_DIR)/ar45_main.o $(LIB_DIR)/lib45-common.a

# Object Disassembler (objdump45) - uses codegen, basic, common
$(OD_TARGET): $(OBJ_DIR)/objdump45_main.o | $(LIB_DIR)
$(OD_TARGET): $(LIB_DIR)/lib45-codegen.a $(LIB_DIR)/lib45-basic.a $(LIB_DIR)/lib45-common.a
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ_DIR)/objdump45_main.o $(LIB_DIR)/lib45-codegen.a $(LIB_DIR)/lib45-basic.a $(LIB_DIR)/lib45-common.a

# Disk Utility (disk45) - uses tools + common
$(DISK_TARGET): $(OBJ_DIR)/disk45_main.o | $(LIB_DIR)
$(DISK_TARGET): $(LIB_DIR)/lib45-tools.a $(LIB_DIR)/lib45-common.a
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ_DIR)/disk45_main.o $(LIB_DIR)/lib45-tools.a $(LIB_DIR)/lib45-common.a -lz -lsqlite3 $(FUSE3_LIBS)

# Assembly Format Converter (cvt_asm) - uses tools, codegen, opt + common
$(CVT_ASM_TARGET): $(OBJ_DIR)/cvt_asm_main.o | $(LIB_DIR)
$(CVT_ASM_TARGET): $(LIB_DIR)/lib45-tools.a $(LIB_DIR)/lib45-codegen.a \
                   $(LIB_DIR)/lib45-opt.a $(LIB_DIR)/lib45-common.a
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ_DIR)/cvt_asm_main.o $(LIB_DIR)/lib45-tools.a $(LIB_DIR)/lib45-codegen.a $(LIB_DIR)/lib45-opt.a $(LIB_DIR)/lib45-common.a

# BASIC Compiler (basic45) - uses basic + tools + common
$(BASIC_TARGET): $(OBJ_DIR)/basic45_main.o | $(LIB_DIR)
$(BASIC_TARGET): $(LIB_DIR)/lib45-basic.a $(LIB_DIR)/lib45-tools.a $(LIB_DIR)/lib45-common.a
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ_DIR)/basic45_main.o $(LIB_DIR)/lib45-basic.a $(LIB_DIR)/lib45-tools.a $(LIB_DIR)/lib45-common.a

# ============================================================================
# COMPILATION RULES
# ============================================================================

# disk45 FUSE module needs FUSE3 headers
$(OBJ_DIR)/disk45_fuse.o: $(SRC_DIR)/disk45_fuse.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(FUSE3_CFLAGS) -c -o $@ $<

# disk45_main needs HAVE_FUSE3 define
$(OBJ_DIR)/disk45_main.o: $(SRC_DIR)/disk45_main.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(FUSE3_CFLAGS) -c -o $@ $<

# Pipeline objects (compiler-only)
$(OBJ_DIR)/CompilationPipeline.o: $(SRC_DIR)/CompilationPipeline.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/PreprocessStage.o: $(SRC_DIR)/PreprocessStage.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/ParseStage.o: $(SRC_DIR)/ParseStage.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/OptimizeStage.o: $(SRC_DIR)/OptimizeStage.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/CodegenStage.o: $(SRC_DIR)/CodegenStage.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/AssemblyStage.o: $(SRC_DIR)/AssemblyStage.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/LinkingStage.o: $(SRC_DIR)/LinkingStage.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Default compilation rule for all object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

$(LIB_DIR):
	@mkdir -p $(LIB_DIR)

-include $(OBJ_DIR)/*.d

# ============================================================================
# CONVENIENCE TARGETS
# ============================================================================

.PHONY: lib45-libraries lib45-sizes lib45-clean show-lib-deps parallel-build
.PHONY: all clean test man test-mmemu test-stdlib test-regression test-zpcall
.PHONY: test-integration bench bench-save lib install install_local uninstall
.PHONY: uninstall_local cppcheck coverage coverage-build coverage-clean
.PHONY: coverage-report docker validate_performance test-assembler test-const
.PHONY: test-restrict test-register test-cast-fold test-o45 test-move-fill
.PHONY: test-validation-parser test-validation-struct test-validation-const
.PHONY: test-validation-semantic test-validation-preprocessor test-opcodes
.PHONY: test-validation-linker test-segment-emission test-validation-assembler
.PHONY: test-validation-simops test-validation-directives test-validation-symbols
.PHONY: test-validation-segments test-validation-proc test-validation-addressing
.PHONY: test-validation-simops-extended test-objdump45

# Build all lib45 libraries
lib45-libraries: $(LIB_DIR)/lib45-common.a $(LIB_DIR)/lib45-c-compile.a \
                $(LIB_DIR)/lib45-ir.a $(LIB_DIR)/lib45-opt.a \
                $(LIB_DIR)/lib45-codegen.a $(LIB_DIR)/lib45-linker.a \
                $(LIB_DIR)/lib45-basic.a $(LIB_DIR)/lib45-tools.a

# Show library dependency information
show-lib-deps:
	@echo "=== lib45 Library Dependencies ==="
	@echo "lib45-common.a        : O45 format, configuration, utilities (no deps)"
	@echo "lib45-c-compile.a     : C compiler frontend → lib45-common"
	@echo "lib45-ir.a            : Intermediate representation → lib45-common"
	@echo "lib45-opt.a           : Optimization passes → lib45-ir lib45-common"
	@echo "lib45-codegen.a       : Code generation, assembly → lib45-common"
	@echo "lib45-linker.a        : Linker, relocation → lib45-opt lib45-common"
	@echo "lib45-basic.a         : BASIC language features → lib45-tools lib45-common"
	@echo "lib45-tools.a         : Disk utilities, converters → lib45-common"
	@echo ""
	@echo "=== Binary Dependencies ==="
	@echo "cc45      : lib45-opt lib45-codegen lib45-ir lib45-c-compile lib45-common"
	@echo "ca45      : lib45-codegen lib45-common"
	@echo "cp45      : symlink to cc45"
	@echo "ln45      : lib45-linker lib45-opt lib45-common"
	@echo "nm45      : lib45-common"
	@echo "ar45      : lib45-common"
	@echo "objdump45 : lib45-codegen lib45-common"
	@echo "disk45    : lib45-tools lib45-common"
	@echo "cvt_asm   : lib45-tools lib45-codegen lib45-opt lib45-common"
	@echo "basic45   : lib45-basic lib45-tools lib45-common"

# Display library file sizes
lib45-sizes: lib45-libraries
	@echo "=== lib45 Library Sizes ==="
	@ls -lh $(LIB_DIR)/lib45-*.a 2>/dev/null | awk '{printf "%-25s %8s\n", $$9, $$5}' || echo "No libraries built"
	@echo "=== Total Size ==="
	@du -sh $(LIB_DIR) 2>/dev/null || echo "Library directory empty"

# Clean lib45 libraries only
lib45-clean:
	@rm -f $(LIB_DIR)/lib45-*.a
	@rmdir $(LIB_DIR) 2>/dev/null || true

# Parallel build (useful for multi-core systems)
parallel-build:
	$(MAKE) -j8 lib45-libraries
	$(MAKE) -j8 all

lib: all
	@$(MAKE) -C lib

clean: lib45-clean
	rm -rf $(OBJ_DIR) $(BIN_DIR) build
	@$(MAKE) -C bug clean
	@$(MAKE) -C lib clean

test: all lib
	@echo "Validating documentation structure and links..."
	@python3 src/test/check_docs.py
	@echo "Running compiler tests..."
	@bash src/test/test_compiler.sh
	@echo "Running assembler feature tests..."
	@bash src/test/test_assembler.sh
	@echo "Running const keyword tests..."
	@bash src/test/test_const_compiler.sh
	@bash src/test/test_const_assembler.sh
	@echo "Running restrict qualifier tests..."
	@bash src/test/test_restrict.sh
	@echo "Running register keyword tests..."
	@bash src/test/test_register.sh
	@echo "Running cast fold type preservation tests..."
	@bash src/test/test_cast_fold.sh
	@echo "Running clobber tracking tests..."
	@bash src/test/test_clobber.sh
	@echo "Validating mmemu-cli integration..."
	@bash src/test/test_mmemu.sh
	@echo "Running parser syntax error validation tests..."
	@$(MAKE) test-validation-parser
	@echo "Running struct/union semantic error validation tests..."
	@$(MAKE) test-validation-struct
	@echo "Running const/register constraint error validation tests..."
	@$(MAKE) test-validation-const
	@echo "Running semantic/type error validation tests..."
	@$(MAKE) test-validation-semantic
	@echo "Running preprocessor error validation tests..."
	@$(MAKE) test-validation-preprocessor
	@echo "Running linker error validation tests..."
	@$(MAKE) test-validation-linker
	@echo "Running segment emission tests..."
	@$(MAKE) test-segment-emission
	@echo "Running assembler expression/emitter error validation tests..."
	@$(MAKE) test-validation-assembler
	@echo "Running simulated ops register constraint validation tests..."
	@$(MAKE) test-validation-simops
	@echo "Running data directives validation tests..."
	@$(MAKE) test-validation-directives
	@echo "Running symbol/expression validation tests..."
	@$(MAKE) test-validation-symbols
	@echo "Running segment/visibility validation tests..."
	@$(MAKE) test-validation-segments
	@echo "Running proc/function attribute validation tests..."
	@$(MAKE) test-validation-proc
	@echo "Running addressing mode validation tests..."
	@$(MAKE) test-validation-addressing
	@echo "Running extended simulated ops validation tests..."
	@$(MAKE) test-validation-simops-extended
	@$(MAKE) test-opcodes
	@echo "Running Move/Fill operation tests..."
	@$(MAKE) test-move-fill
	@echo "Running objdump45 tests..."
	@bash src/test/test_objdump45.sh
	@echo "Running disk45 tests..."
	@bash src/test/test_disk45.sh
	@echo "Running new stdlib tests..."
	@bash src/test/test_stdlib_new.sh

test-assembler: all
	@bash src/test/test_assembler.sh

test-const: all
	@bash src/test/test_const_compiler.sh
	@bash src/test/test_const_assembler.sh

test-restrict: all
	@bash src/test/test_restrict.sh

test-register: all
	@bash src/test/test_register.sh

test-cast-fold: all
	@bash src/test/test_cast_fold.sh

test-integration: all
	@bash src/test/test_integration.sh

test-mmemu: all
	@bash src/test/test_mmemu.sh

test-stdlib: all lib
	@bash src/test/test_stdlib.sh

test-regression: all lib
	@bash src/test/test_regression.sh

test-zpcall: all
	@bash src/test/test_zpcall.sh

bench: all lib
	@bash src/test/bench.sh

bench-optimizer: all
	@bash src/test/bench_optimizer.sh

bench-save: all lib
	@bash src/test/bench.sh
	@cp build/test/bench_results.json src/test/bench_baseline.json
	@echo "Baseline saved to src/test/bench_baseline.json"

test-objdump45: all
	@bash src/test/test_objdump45.sh

test-opcodes: all
	@echo "Validating opcodes and addressing modes..."
	@bash src/test/test_opcodes.sh

# O45 format unit test
TEST_O45_TARGET = $(BIN_DIR)/test_o45
TEST_O45_OBJECTS = $(OBJ_DIR)/test_o45.o $(OBJ_DIR)/O45Writer.o $(OBJ_DIR)/O45Reader.o $(OBJ_DIR)/O45IRSerializer.o $(OBJ_DIR)/O45Linker.o $(OBJ_DIR)/O45Archive.o

$(TEST_O45_TARGET): $(TEST_O45_OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJ_DIR)/test_o45.o: src/test/test_o45.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

test-o45: $(TEST_O45_TARGET)
	@$(TEST_O45_TARGET)

# Move/Fill operations unit test
TEST_MOVE_FILL_TARGET = $(BIN_DIR)/test_move_fill
TEST_MOVE_FILL_OBJECTS = $(OBJ_DIR)/test_move_fill.o

$(TEST_MOVE_FILL_TARGET): $(TEST_MOVE_FILL_OBJECTS) $(CA_TARGET)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(TEST_MOVE_FILL_OBJECTS)

$(OBJ_DIR)/test_move_fill.o: src/test/test_move_fill.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

test-move-fill: $(TEST_MOVE_FILL_TARGET) all
	@$(TEST_MOVE_FILL_TARGET)

# Parser Syntax Error Validation unit test
TEST_VALIDATION_PARSER_TARGET = $(BIN_DIR)/test_validation_parser
TEST_VALIDATION_PARSER_OBJECTS = $(OBJ_DIR)/test_validation_parser.o

$(TEST_VALIDATION_PARSER_TARGET): $(TEST_VALIDATION_PARSER_OBJECTS) $(CC_TARGET)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(TEST_VALIDATION_PARSER_OBJECTS)

$(OBJ_DIR)/test_validation_parser.o: src/test/test_validation_parser.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

test-validation-parser: $(TEST_VALIDATION_PARSER_TARGET) all
	@$(TEST_VALIDATION_PARSER_TARGET)

# Assembler expression/emitter validation test
TEST_VALIDATION_ASSEMBLER_TARGET = $(BIN_DIR)/test_validation_assembler
TEST_VALIDATION_ASSEMBLER_OBJECTS = $(OBJ_DIR)/test_validation_assembler.o

$(TEST_VALIDATION_ASSEMBLER_TARGET): $(TEST_VALIDATION_ASSEMBLER_OBJECTS) $(CA_TARGET)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(TEST_VALIDATION_ASSEMBLER_OBJECTS)

$(OBJ_DIR)/test_validation_assembler.o: src/test/test_validation_assembler.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

test-validation-assembler: $(TEST_VALIDATION_ASSEMBLER_TARGET) all
	@$(TEST_VALIDATION_ASSEMBLER_TARGET)

# Simulated Ops validation test
TEST_VALIDATION_SIMOPS_TARGET = $(BIN_DIR)/test_validation_simops
TEST_VALIDATION_SIMOPS_OBJECTS = $(OBJ_DIR)/test_validation_simops.o

$(TEST_VALIDATION_SIMOPS_TARGET): $(TEST_VALIDATION_SIMOPS_OBJECTS) $(CA_TARGET)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(TEST_VALIDATION_SIMOPS_OBJECTS)

$(OBJ_DIR)/test_validation_simops.o: src/test/test_validation_simops.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

test-validation-simops: $(TEST_VALIDATION_SIMOPS_TARGET) all
	@$(TEST_VALIDATION_SIMOPS_TARGET)

# Addressing Mode validation test
TEST_VALIDATION_ADDRESSING_TARGET = $(BIN_DIR)/test_validation_addressing
TEST_VALIDATION_ADDRESSING_OBJECTS = $(OBJ_DIR)/test_validation_addressing.o

$(TEST_VALIDATION_ADDRESSING_TARGET): $(TEST_VALIDATION_ADDRESSING_OBJECTS) $(CA_TARGET)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(TEST_VALIDATION_ADDRESSING_OBJECTS)

$(OBJ_DIR)/test_validation_addressing.o: src/test/test_validation_addressing.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

test-validation-addressing: $(TEST_VALIDATION_ADDRESSING_TARGET) all
	@$(TEST_VALIDATION_ADDRESSING_TARGET)

# Extended Simulated Ops validation test
TEST_VALIDATION_SIMOPS_EXTENDED_TARGET = $(BIN_DIR)/test_validation_simops_extended
TEST_VALIDATION_SIMOPS_EXTENDED_OBJECTS = $(OBJ_DIR)/test_validation_simops_extended.o

$(TEST_VALIDATION_SIMOPS_EXTENDED_TARGET): $(TEST_VALIDATION_SIMOPS_EXTENDED_OBJECTS) $(CA_TARGET)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(TEST_VALIDATION_SIMOPS_EXTENDED_OBJECTS)

$(OBJ_DIR)/test_validation_simops_extended.o: src/test/test_validation_simops_extended.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

test-validation-simops-extended: $(TEST_VALIDATION_SIMOPS_EXTENDED_TARGET) all
	@$(TEST_VALIDATION_SIMOPS_EXTENDED_TARGET)

# Segment and Visibility Directives validation test
TEST_VALIDATION_SEGMENTS_TARGET = $(BIN_DIR)/test_validation_segments
TEST_VALIDATION_SEGMENTS_OBJECTS = $(OBJ_DIR)/test_validation_segments.o

$(TEST_VALIDATION_SEGMENTS_TARGET): $(TEST_VALIDATION_SEGMENTS_OBJECTS) $(CA_TARGET)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(TEST_VALIDATION_SEGMENTS_OBJECTS)

$(OBJ_DIR)/test_validation_segments.o: src/test/test_validation_segments.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

test-validation-segments: $(TEST_VALIDATION_SEGMENTS_TARGET) all
	@$(TEST_VALIDATION_SEGMENTS_TARGET)

# Symbol & Expression validation test
TEST_VALIDATION_SYMBOLS_TARGET = $(BIN_DIR)/test_validation_symbols
TEST_VALIDATION_SYMBOLS_OBJECTS = $(OBJ_DIR)/test_validation_symbols.o

$(TEST_VALIDATION_SYMBOLS_TARGET): $(TEST_VALIDATION_SYMBOLS_OBJECTS) $(CA_TARGET)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(TEST_VALIDATION_SYMBOLS_OBJECTS)

$(OBJ_DIR)/test_validation_symbols.o: src/test/test_validation_symbols.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

test-validation-symbols: $(TEST_VALIDATION_SYMBOLS_TARGET) all
	@$(TEST_VALIDATION_SYMBOLS_TARGET)

# Data Directives validation test
TEST_VALIDATION_DIRECTIVES_TARGET = $(BIN_DIR)/test_validation_directives
TEST_VALIDATION_DIRECTIVES_OBJECTS = $(OBJ_DIR)/test_validation_directives.o

$(TEST_VALIDATION_DIRECTIVES_TARGET): $(TEST_VALIDATION_DIRECTIVES_OBJECTS) $(CA_TARGET)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(TEST_VALIDATION_DIRECTIVES_OBJECTS)

$(OBJ_DIR)/test_validation_directives.o: src/test-resources/test_validation_directives.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

test-validation-directives: $(TEST_VALIDATION_DIRECTIVES_TARGET) all
	@$(TEST_VALIDATION_DIRECTIVES_TARGET)

# Proc / Function Attribute validation test
TEST_VALIDATION_PROC_TARGET = $(BIN_DIR)/test_validation_proc
TEST_VALIDATION_PROC_OBJECTS = $(OBJ_DIR)/test_validation_proc.o

$(TEST_VALIDATION_PROC_TARGET): $(TEST_VALIDATION_PROC_OBJECTS) $(CA_TARGET)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(TEST_VALIDATION_PROC_OBJECTS)

$(OBJ_DIR)/test_validation_proc.o: src/test/test_validation_proc.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

test-validation-proc: $(TEST_VALIDATION_PROC_TARGET) all
	@$(TEST_VALIDATION_PROC_TARGET)

# Struct/Union Semantic Error Validation unit test
TEST_VALIDATION_STRUCT_TARGET = $(BIN_DIR)/test_validation_struct
TEST_VALIDATION_STRUCT_OBJECTS = $(OBJ_DIR)/test_validation_struct.o

$(TEST_VALIDATION_STRUCT_TARGET): $(TEST_VALIDATION_STRUCT_OBJECTS) $(CC_TARGET)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(TEST_VALIDATION_STRUCT_OBJECTS)

$(OBJ_DIR)/test_validation_struct.o: src/test/test_validation_struct.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

test-validation-struct: $(TEST_VALIDATION_STRUCT_TARGET) all
	@$(TEST_VALIDATION_STRUCT_TARGET)

# Const/Register Constraint Error Validation unit test
TEST_VALIDATION_CONST_TARGET = $(BIN_DIR)/test_validation_const
TEST_VALIDATION_CONST_OBJECTS = $(OBJ_DIR)/test_validation_const.o

$(TEST_VALIDATION_CONST_TARGET): $(TEST_VALIDATION_CONST_OBJECTS) $(CC_TARGET)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(TEST_VALIDATION_CONST_OBJECTS)

$(OBJ_DIR)/test_validation_const.o: src/test/test_validation_const.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

test-validation-const: $(TEST_VALIDATION_CONST_TARGET) all
	@$(TEST_VALIDATION_CONST_TARGET)

# Semantic/Type Error Validation unit test
TEST_VALIDATION_SEMANTIC_TARGET = $(BIN_DIR)/test_validation_semantic
TEST_VALIDATION_SEMANTIC_OBJECTS = $(OBJ_DIR)/test_validation_semantic.o

$(TEST_VALIDATION_SEMANTIC_TARGET): $(TEST_VALIDATION_SEMANTIC_OBJECTS) $(CC_TARGET)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(TEST_VALIDATION_SEMANTIC_OBJECTS)

$(OBJ_DIR)/test_validation_semantic.o: src/test/test_validation_semantic.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

test-validation-semantic: $(TEST_VALIDATION_SEMANTIC_TARGET) all
	@$(TEST_VALIDATION_SEMANTIC_TARGET)

# Preprocessor Error Validation unit test
TEST_VALIDATION_PREPROCESSOR_TARGET = $(BIN_DIR)/test_validation_preprocessor
TEST_VALIDATION_PREPROCESSOR_OBJECTS = $(OBJ_DIR)/test_validation_preprocessor.o

$(TEST_VALIDATION_PREPROCESSOR_TARGET): $(TEST_VALIDATION_PREPROCESSOR_OBJECTS) $(CC_TARGET)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(TEST_VALIDATION_PREPROCESSOR_OBJECTS)

$(OBJ_DIR)/test_validation_preprocessor.o: src/test/test_validation_preprocessor.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

test-validation-preprocessor: $(TEST_VALIDATION_PREPROCESSOR_TARGET) all
	@mkdir -p src/test/build
	@$(TEST_VALIDATION_PREPROCESSOR_TARGET)

# Linker Error Validation unit test
TEST_VALIDATION_LINKER_TARGET = $(BIN_DIR)/test_validation_linker
TEST_VALIDATION_LINKER_OBJECTS = $(OBJ_DIR)/test_validation_linker.o $(OBJ_DIR)/O45Reader.o $(OBJ_DIR)/O45Writer.o $(OBJ_DIR)/O45IRSerializer.o $(OBJ_DIR)/O45Linker.o $(OBJ_DIR)/O45Archive.o $(OBJ_DIR)/AssemblerOpcodeDatabase.o

$(TEST_VALIDATION_LINKER_TARGET): $(TEST_VALIDATION_LINKER_OBJECTS) $(CA_TARGET)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(TEST_VALIDATION_LINKER_OBJECTS)

$(OBJ_DIR)/test_validation_linker.o: src/test/test_validation_linker.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

test-validation-linker: $(TEST_VALIDATION_LINKER_TARGET) all
	@$(TEST_VALIDATION_LINKER_TARGET)

# Segment emission unit test
TEST_SEGMENT_EMISSION_TARGET = $(BIN_DIR)/test_segment_emission
TEST_SEGMENT_EMISSION_OBJECTS = $(OBJ_DIR)/test_segment_emission.o $(OBJ_DIR)/O45Reader.o $(OBJ_DIR)/O45Writer.o $(OBJ_DIR)/O45IRSerializer.o $(OBJ_DIR)/O45Linker.o $(OBJ_DIR)/O45Archive.o $(OBJ_DIR)/AssemblerOpcodeDatabase.o

$(TEST_SEGMENT_EMISSION_TARGET): $(TEST_SEGMENT_EMISSION_OBJECTS) $(CA_TARGET)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(TEST_SEGMENT_EMISSION_OBJECTS)

$(OBJ_DIR)/test_segment_emission.o: src/test/test_segment_emission.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

test-segment-emission: $(TEST_SEGMENT_EMISSION_TARGET) all
	@$(TEST_SEGMENT_EMISSION_TARGET)

# --- Code coverage ---
# Usage: make coverage
# Requires: gcov (always available with g++), gcovr (pip install gcovr) for reports
# Output: build/coverage/ (HTML report), console summary
COV_DIR = build/coverage

coverage-clean:
	@rm -rf $(COV_DIR) $(OBJ_DIR)/*.gcda $(OBJ_DIR)/*.gcno

coverage-build: coverage-clean
	@echo "Building with coverage instrumentation..."
	@$(MAKE) clean
	@EXTRA_CXXFLAGS=--coverage $(MAKE) all lib

coverage-report:
	@mkdir -p $(COV_DIR)
	@if command -v gcovr >/dev/null 2>&1; then \
		echo "Generating coverage report..."; \
		gcovr --root . --filter 'src/main/' \
			--exclude '.*_main\.cpp' \
			--print-summary \
			--html-details $(COV_DIR)/index.html \
			--txt $(COV_DIR)/summary.txt \
			$(OBJ_DIR)/; \
		echo ""; \
		echo "HTML report: $(COV_DIR)/index.html"; \
		echo "Text summary: $(COV_DIR)/summary.txt"; \
	else \
		echo "gcovr not found — install with: pip install gcovr"; \
		echo "Falling back to raw gcov..."; \
		cd $(OBJ_DIR) && gcov *.gcda 2>/dev/null | grep -A1 "^File.*src/main" | head -40; \
	fi

coverage: coverage-build
	@echo ""
	@echo "Running test suite for coverage..."
	-@EXTRA_CXXFLAGS=--coverage $(MAKE) test
	@echo ""
	@$(MAKE) coverage-report

install: all lib
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(CC_TARGET) $(CA_TARGET) $(NM_TARGET) $(LN_TARGET) $(AR_TARGET) $(OD_TARGET) $(DISK_TARGET) $(CVT_ASM_TARGET) $(BASIC_TARGET) $(DESTDIR)$(BINDIR)
	ln -sf cc45 $(DESTDIR)$(BINDIR)/cp45
	install -d $(DESTDIR)$(LIBDIR)
	install -m 644 lib/build/c45.lib lib/build/c45_zp.lib $(DESTDIR)$(LIBDIR)
	install -d $(DESTDIR)$(INCDIR)
	install -m 644 lib/include/*.h $(DESTDIR)$(INCDIR)
	install -d $(DESTDIR)$(MANDIR)
	if [ -d $(MAN_DIR) ] && ls $(MAN_DIR)/*.1 >/dev/null 2>&1; then \
		install -m 644 $(MAN_DIR)/*.1 $(DESTDIR)$(MANDIR); \
	fi

install_local:
	@$(MAKE) install PREFIX=$(HOME)/.local

uninstall_local:
	@$(MAKE) uninstall PREFIX=$(HOME)/.local

validate_performance: all
	@echo "Validating performance with multi-level optimization benchmark..."
	@bash src/test/validate_performance.sh

docker:
	@echo "Building Docker image..."
	@docker build -f src/Docker/Dockerfile -t mega65-cc45:latest .
	@mkdir -p $(BIN_DIR)
	@echo "Exporting Docker image to $(BIN_DIR)..."
	@docker save mega65-cc45:latest -o $(BIN_DIR)/mega65-cc45.tar
	@echo "✓ Docker image exported to $(BIN_DIR)/mega65-cc45.tar"
	@ls -lh $(BIN_DIR)/mega65-cc45.tar

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/cc45 $(DESTDIR)$(BINDIR)/ca45 $(DESTDIR)$(BINDIR)/cp45
	rm -f $(DESTDIR)$(BINDIR)/nm45 $(DESTDIR)$(BINDIR)/ln45 $(DESTDIR)$(BINDIR)/ar45
	rm -f $(DESTDIR)$(BINDIR)/objdump45 $(DESTDIR)$(BINDIR)/disk45 $(DESTDIR)$(BINDIR)/cvt_asm
	rm -f $(DESTDIR)$(BINDIR)/basic45
	rm -rf $(DESTDIR)$(LIBDIR)
	rm -rf $(DESTDIR)$(INCDIR)
	rm -f $(DESTDIR)$(MANDIR)/cc45.1 $(DESTDIR)$(MANDIR)/ca45.1 $(DESTDIR)$(MANDIR)/cp45.1
	rm -f $(DESTDIR)$(MANDIR)/ln45.1 $(DESTDIR)$(MANDIR)/nm45.1 $(DESTDIR)$(MANDIR)/ar45.1
	rm -f $(DESTDIR)$(MANDIR)/objdump45.1
