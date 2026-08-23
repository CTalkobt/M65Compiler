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

CC_SOURCES = $(SRC_DIR)/cc45_main.cpp
CA_SOURCES = $(SRC_DIR)/ca45_main.cpp

# Common objects
COMMON_SOURCES = $(SRC_DIR)/Lexer.cpp $(SRC_DIR)/Parser.cpp $(SRC_DIR)/AST.cpp $(SRC_DIR)/TypeInfo.cpp $(SRC_DIR)/M65Emitter.cpp $(SRC_DIR)/Preprocessor.cpp $(SRC_DIR)/ConstantFolder.cpp $(SRC_DIR)/LoopOptimizer.cpp $(SRC_DIR)/LoopIdiomRegistry.cpp $(SRC_DIR)/AddressTemplates.cpp $(SRC_DIR)/AddressTemplateDetector.cpp $(SRC_DIR)/CodeGenerator.cpp $(SRC_DIR)/LoopInterchange.cpp $(SRC_DIR)/FunctionAnalyzer.cpp $(SRC_DIR)/OptimizationSelector.cpp $(SRC_DIR)/InlineSelector.cpp $(SRC_DIR)/CallGraphAnalyzer.cpp $(SRC_DIR)/DevirtualizationDetector.cpp $(SRC_DIR)/CoOptimizationSelector.cpp $(SRC_DIR)/DevirtualizationHints.cpp $(SRC_DIR)/CoOptimizationApplier.cpp $(SRC_DIR)/OptimizationCatalog.cpp $(SRC_DIR)/OptimizationController.cpp $(SRC_DIR)/AssemblerOpcodeDatabase.cpp $(SRC_DIR)/O45Reader.cpp $(SRC_DIR)/O45Writer.cpp $(SRC_DIR)/O45IRSerializer.cpp $(SRC_DIR)/O45Emitter.cpp $(SRC_DIR)/TypeSystem.cpp $(SRC_DIR)/ScopeManager.cpp $(SRC_DIR)/ConfigLoader.cpp $(SRC_DIR)/GlobalFunctionDatabase.cpp $(SRC_DIR)/CallPatternAnalyzer.cpp $(SRC_DIR)/IPOAnalyzer.cpp $(SRC_DIR)/IPOProfiler.cpp $(SRC_DIR)/SpecializationCodeGenerator.cpp $(SRC_DIR)/SpecializationOptimizer.cpp $(SRC_DIR)/IRSpecializationGenerator.cpp $(SRC_DIR)/StructFieldStriper.cpp $(SRC_DIR)/FieldStripedOffsetCalc.cpp $(SRC_DIR)/AddressSpaceValidator.cpp $(SRC_DIR)/GlobalPointerFieldDatabase.cpp $(SRC_DIR)/InterTUPatternDetector.cpp $(SRC_DIR)/FieldCachingAnalyzer.cpp $(SRC_DIR)/FieldCachingLinkerIntegration.cpp $(SRC_DIR)/FarAddressMemorySupport.cpp $(SRC_DIR)/FarAddressCodeGenerator.cpp $(SRC_DIR)/CrossModuleVariableDatabase.cpp $(SRC_DIR)/MemoryBankAssigner.cpp $(SRC_DIR)/BankLayoutGenerator.cpp $(SRC_DIR)/BankSetupOptimizer.cpp $(SRC_DIR)/BankAwareCodeGenerator.cpp $(SRC_DIR)/OptimizationHintCollector.cpp $(SRC_DIR)/OptimizationConstraintResolver.cpp $(SRC_DIR)/LinkTimeOptimizationCoordinator.cpp $(SRC_DIR)/OptimizationDependencyAnalyzer.cpp $(SRC_DIR)/LinkTimeOptimizationCodeGenerator.cpp $(SRC_DIR)/LinkTimeOptimizationValidator.cpp $(SRC_DIR)/CompilationProfiler.cpp $(SRC_DIR)/HotSpotProfiler.cpp $(SRC_DIR)/ComparativePerformanceAnalyzer.cpp $(SRC_DIR)/PatternRecognitionEngine.cpp $(SRC_DIR)/PatternBasedOptimizationSelector.cpp $(SRC_DIR)/CrossModuleEnhancer.cpp $(SRC_DIR)/DependencyTracker.cpp $(SRC_DIR)/BenchmarkingSuite.cpp $(SRC_DIR)/BenchmarkCompilerIntegration.cpp $(SRC_DIR)/BenchmarkExecutor.cpp $(SRC_DIR)/BenchmarkMetricsCollector.cpp $(SRC_DIR)/BenchmarkComparativeAnalyzer.cpp $(SRC_DIR)/BenchmarkReportGenerator.cpp $(SRC_DIR)/IterationManager.cpp $(SRC_DIR)/OptimizationLearner.cpp $(SRC_DIR)/FeedbackCoordinator.cpp $(SRC_DIR)/OnlineLearner.cpp $(SRC_DIR)/TuningHooks.cpp $(SRC_DIR)/CompilationSignalCollector.cpp $(SRC_DIR)/CompilerHookIntegrator.cpp $(SRC_DIR)/HookIntegration.cpp $(SRC_DIR)/CompilerDecisionLogic.cpp $(SRC_DIR)/OptimizationEffectivenessCollector.cpp $(SRC_DIR)/OptimizationProfileDatabase.cpp $(SRC_DIR)/AdaptiveThresholdAdjuster.cpp $(SRC_DIR)/OptimizationPatternAnalyzer.cpp $(SRC_DIR)/LearnerBasedOptimizationSelector.cpp $(SRC_DIR)/AdaptiveLearnerIntegration.cpp $(SRC_DIR)/TemplateOptimizationPass.cpp $(SRC_DIR)/TemplateRegistry.cpp $(SRC_DIR)/TemplateOptimizationSystem.cpp $(SRC_DIR)/MathLibraryOptimization.cpp $(SRC_DIR)/RegisterResidentLoops.cpp $(SRC_DIR)/TableDrivenDispatch.cpp $(SRC_DIR)/LearnerFeedbackRecorder.cpp $(SRC_DIR)/CompilationPipeline.cpp $(SRC_DIR)/PreprocessStage.cpp $(SRC_DIR)/ParseStage.cpp $(SRC_DIR)/OptimizeStage.cpp $(SRC_DIR)/CodegenStage.cpp $(SRC_DIR)/AssemblyStage.cpp $(SRC_DIR)/LinkingStage.cpp
COMMON_OBJECTS = $(OBJ_DIR)/Lexer.o $(OBJ_DIR)/Parser.o $(OBJ_DIR)/AST.o $(OBJ_DIR)/TypeInfo.o $(OBJ_DIR)/M65Emitter.o $(OBJ_DIR)/Preprocessor.o $(OBJ_DIR)/ConstantFolder.o $(OBJ_DIR)/LoopOptimizer.o $(OBJ_DIR)/LoopIdiomRegistry.o $(OBJ_DIR)/AddressTemplates.o $(OBJ_DIR)/AddressTemplateDetector.o $(OBJ_DIR)/CodeGenerator.o $(OBJ_DIR)/LoopInterchange.o $(OBJ_DIR)/FunctionAnalyzer.o $(OBJ_DIR)/OptimizationSelector.o $(OBJ_DIR)/InlineSelector.o $(OBJ_DIR)/CallGraphAnalyzer.o $(OBJ_DIR)/DevirtualizationDetector.o $(OBJ_DIR)/CoOptimizationSelector.o $(OBJ_DIR)/DevirtualizationHints.o $(OBJ_DIR)/CoOptimizationApplier.o $(OBJ_DIR)/OptimizationCatalog.o $(OBJ_DIR)/OptimizationController.o $(OBJ_DIR)/AssemblerOpcodeDatabase.o $(OBJ_DIR)/O45Reader.o $(OBJ_DIR)/O45Writer.o $(OBJ_DIR)/O45IRSerializer.o $(OBJ_DIR)/O45Emitter.o $(OBJ_DIR)/TypeSystem.o $(OBJ_DIR)/ScopeManager.o $(OBJ_DIR)/ConfigLoader.o $(OBJ_DIR)/GlobalFunctionDatabase.o $(OBJ_DIR)/CallPatternAnalyzer.o $(OBJ_DIR)/IPOAnalyzer.o $(OBJ_DIR)/IPOProfiler.o $(OBJ_DIR)/SpecializationCodeGenerator.o $(OBJ_DIR)/SpecializationOptimizer.o $(OBJ_DIR)/IRSpecializationGenerator.o $(OBJ_DIR)/StructFieldStriper.o $(OBJ_DIR)/FieldStripedOffsetCalc.o $(OBJ_DIR)/AddressSpaceValidator.o $(OBJ_DIR)/GlobalPointerFieldDatabase.o $(OBJ_DIR)/InterTUPatternDetector.o $(OBJ_DIR)/FieldCachingAnalyzer.o $(OBJ_DIR)/FieldCachingLinkerIntegration.o $(OBJ_DIR)/FarAddressMemorySupport.o $(OBJ_DIR)/FarAddressCodeGenerator.o $(OBJ_DIR)/CrossModuleVariableDatabase.o $(OBJ_DIR)/MemoryBankAssigner.o $(OBJ_DIR)/BankLayoutGenerator.o $(OBJ_DIR)/BankSetupOptimizer.o $(OBJ_DIR)/BankAwareCodeGenerator.o $(OBJ_DIR)/OptimizationHintCollector.o $(OBJ_DIR)/OptimizationConstraintResolver.o $(OBJ_DIR)/LinkTimeOptimizationCoordinator.o $(OBJ_DIR)/OptimizationDependencyAnalyzer.o $(OBJ_DIR)/LinkTimeOptimizationCodeGenerator.o $(OBJ_DIR)/LinkTimeOptimizationValidator.o $(OBJ_DIR)/CompilationProfiler.o $(OBJ_DIR)/HotSpotProfiler.o $(OBJ_DIR)/ComparativePerformanceAnalyzer.o $(OBJ_DIR)/PatternRecognitionEngine.o $(OBJ_DIR)/PatternBasedOptimizationSelector.o $(OBJ_DIR)/CrossModuleEnhancer.o $(OBJ_DIR)/DependencyTracker.o $(OBJ_DIR)/BenchmarkingSuite.o $(OBJ_DIR)/BenchmarkCompilerIntegration.o $(OBJ_DIR)/BenchmarkExecutor.o $(OBJ_DIR)/BenchmarkMetricsCollector.o $(OBJ_DIR)/BenchmarkComparativeAnalyzer.o $(OBJ_DIR)/BenchmarkReportGenerator.o $(OBJ_DIR)/IterationManager.o $(OBJ_DIR)/OptimizationLearner.o $(OBJ_DIR)/FeedbackCoordinator.o $(OBJ_DIR)/OnlineLearner.o $(OBJ_DIR)/TuningHooks.o $(OBJ_DIR)/CompilationSignalCollector.o $(OBJ_DIR)/CompilerHookIntegrator.o $(OBJ_DIR)/HookIntegration.o $(OBJ_DIR)/CompilerDecisionLogic.o $(OBJ_DIR)/OptimizationEffectivenessCollector.o $(OBJ_DIR)/OptimizationProfileDatabase.o $(OBJ_DIR)/AdaptiveThresholdAdjuster.o $(OBJ_DIR)/OptimizationPatternAnalyzer.o $(OBJ_DIR)/LearnerBasedOptimizationSelector.o $(OBJ_DIR)/AdaptiveLearnerIntegration.o $(OBJ_DIR)/TemplateOptimizationPass.o $(OBJ_DIR)/TemplateRegistry.o $(OBJ_DIR)/TemplateOptimizationSystem.o $(OBJ_DIR)/MathLibraryOptimization.o $(OBJ_DIR)/RegisterResidentLoops.o $(OBJ_DIR)/TableDrivenDispatch.o $(OBJ_DIR)/LearnerFeedbackRecorder.o $(OBJ_DIR)/CompilationPipeline.o $(OBJ_DIR)/PreprocessStage.o $(OBJ_DIR)/ParseStage.o $(OBJ_DIR)/OptimizeStage.o $(OBJ_DIR)/CodegenStage.o $(OBJ_DIR)/AssemblyStage.o $(OBJ_DIR)/LinkingStage.o

CC_OBJECTS = $(OBJ_DIR)/cc45_main.o $(OBJ_DIR)/AssemblerLexer.o $(OBJ_DIR)/AssemblerParser.o $(OBJ_DIR)/AssemblerExpression.o $(OBJ_DIR)/AssemblerOptimizer.o $(OBJ_DIR)/AssemblerSimulatedOps.o $(OBJ_DIR)/AssemblerGenerator.o $(OBJ_DIR)/OpEffect.o $(OBJ_DIR)/IRBuilder.o $(OBJ_DIR)/IRPrinter.o $(OBJ_DIR)/IRCodeGen.o $(OBJ_DIR)/IROptimizer.o $(OBJ_DIR)/VRegAllocator.o $(COMMON_OBJECTS)
CA_OBJECTS = $(OBJ_DIR)/ca45_main.o $(OBJ_DIR)/AssemblerLexer.o $(OBJ_DIR)/AssemblerParser.o $(OBJ_DIR)/AssemblerExpression.o $(OBJ_DIR)/AssemblerOptimizer.o $(OBJ_DIR)/AssemblerSimulatedOps.o $(OBJ_DIR)/AssemblerGenerator.o $(OBJ_DIR)/OpEffect.o $(COMMON_OBJECTS)

MAN_DIR = man

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
LIBDIR ?= $(PREFIX)/lib/cc45
INCDIR ?= $(PREFIX)/include/cc45
MANDIR ?= $(PREFIX)/share/man/man1

.PHONY: all clean test man test-mmemu test-stdlib test-regression test-zpcall test-integration bench bench-save lib install install_local uninstall uninstall_local cppcheck coverage coverage-build coverage-clean coverage-report docker validate_performance

cppcheck:
	cppcheck --enable=warning,performance,portability --inline-suppr -I include/ src/main/

NM_OBJECTS = $(OBJ_DIR)/nm45_main.o $(OBJ_DIR)/O45Reader.o $(OBJ_DIR)/O45Writer.o $(OBJ_DIR)/O45IRSerializer.o $(OBJ_DIR)/O45Linker.o $(OBJ_DIR)/O45Archive.o
LN_OBJECTS = $(OBJ_DIR)/ln45_main.o $(OBJ_DIR)/O45Reader.o $(OBJ_DIR)/O45Writer.o $(OBJ_DIR)/O45IRSerializer.o $(OBJ_DIR)/O45Linker.o $(OBJ_DIR)/O45Archive.o $(OBJ_DIR)/CrossModuleOptimizer.o
AR_OBJECTS = $(OBJ_DIR)/ar45_main.o $(OBJ_DIR)/O45Reader.o $(OBJ_DIR)/O45Writer.o $(OBJ_DIR)/O45IRSerializer.o $(OBJ_DIR)/O45Archive.o
OD_OBJECTS = $(OBJ_DIR)/objdump45_main.o $(OBJ_DIR)/O45Reader.o $(OBJ_DIR)/O45Writer.o $(OBJ_DIR)/O45IRSerializer.o $(OBJ_DIR)/O45Linker.o $(OBJ_DIR)/O45Archive.o $(OBJ_DIR)/AssemblerOpcodeDatabase.o $(OBJ_DIR)/BasicTokenizer.o
DISK_OBJECTS = $(OBJ_DIR)/disk45_main.o $(OBJ_DIR)/disk45_catalog.o $(OBJ_DIR)/DiskImage.o $(OBJ_DIR)/DiskImageFactory.o $(OBJ_DIR)/BAMOperations.o $(OBJ_DIR)/D64Image.o $(OBJ_DIR)/D71Image.o $(OBJ_DIR)/D81Image.o $(OBJ_DIR)/D65Image.o $(OBJ_DIR)/ArkImage.o $(OBJ_DIR)/ArcImage.o $(OBJ_DIR)/LnxImage.o $(OBJ_DIR)/TapImage.o $(OBJ_DIR)/T64Image.o $(OBJ_DIR)/G64Image.o $(OBJ_DIR)/D80Image.o $(OBJ_DIR)/GeosCvtImage.o $(OBJ_DIR)/P00Image.o $(OBJ_DIR)/X64Image.o $(OBJ_DIR)/ZipcodeImage.o $(OBJ_DIR)/D90Image.o $(OBJ_DIR)/CmdImage.o $(OBJ_DIR)/NibImage.o $(OBJ_DIR)/GzipHelper.o
ifeq ($(HAVE_FUSE3),1)
  DISK_OBJECTS += $(OBJ_DIR)/disk45_fuse.o
endif

CVT_ASM_OBJECTS = $(OBJ_DIR)/cvt_asm_main.o $(OBJ_DIR)/AsmParser.o $(OBJ_DIR)/AsmWriter.o $(OBJ_DIR)/Ca45Parser.o $(OBJ_DIR)/Ca45Writer.o $(OBJ_DIR)/Ca65Parser.o $(OBJ_DIR)/Ca65Writer.o $(OBJ_DIR)/AcmeParser.o $(OBJ_DIR)/AcmeWriter.o $(OBJ_DIR)/OscarParser.o $(OBJ_DIR)/OscarWriter.o $(OBJ_DIR)/Merlin64Parser.o $(OBJ_DIR)/Merlin64Writer.o $(OBJ_DIR)/X65Parser.o $(OBJ_DIR)/X65Writer.o $(OBJ_DIR)/FormatDetection.o $(OBJ_DIR)/KickAssemblerParser.o $(OBJ_DIR)/KickAssemblerWriter.o $(OBJ_DIR)/AssemblerLexer.o $(OBJ_DIR)/AssemblerParser.o $(OBJ_DIR)/AssemblerExpression.o $(OBJ_DIR)/AssemblerOpcodeDatabase.o $(OBJ_DIR)/AssemblerOptimizer.o $(OBJ_DIR)/AssemblerSimulatedOps.o $(OBJ_DIR)/AssemblerGenerator.o $(OBJ_DIR)/OpEffect.o $(COMMON_OBJECTS)

BASIC_OBJECTS = $(OBJ_DIR)/basic45_main.o $(OBJ_DIR)/BasicTokenizer.o $(OBJ_DIR)/PETSCIIEncoder.o $(OBJ_DIR)/BasicEmitter.o $(OBJ_DIR)/BasicPreprocessor.o $(OBJ_DIR)/BasicDocGenerator.o

all: $(CC_TARGET) $(CA_TARGET) $(CP_TARGET) $(NM_TARGET) $(LN_TARGET) $(AR_TARGET) $(OD_TARGET) $(DISK_TARGET) $(CVT_ASM_TARGET) $(BASIC_TARGET)

man: $(MAN_DIR)/cc45.1 $(MAN_DIR)/ca45.1 $(MAN_DIR)/cp45.1 $(MAN_DIR)/ln45.1 $(MAN_DIR)/nm45.1 $(MAN_DIR)/ar45.1 $(MAN_DIR)/objdump45.1

$(MAN_DIR)/%.1: doc/bin/%.md
	@mkdir -p $(MAN_DIR)
	pandoc -s -t man $< -o $@ -M title="$(basename $(notdir $@))" -M section="1" -M date="$(shell date +%F)" -M footer="$(basename $(notdir $@)) manual" -M header="User Commands"

$(CC_TARGET): $(CC_OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(CP_TARGET): $(CC_TARGET)
	@mkdir -p $(BIN_DIR)
	ln -sf cc45 $(CP_TARGET)

$(CA_TARGET): $(CA_OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(NM_TARGET): $(NM_OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(LN_TARGET): $(LN_OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(AR_TARGET): $(AR_OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OD_TARGET): $(OD_OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(DISK_TARGET): $(DISK_OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lz -lsqlite3 $(FUSE3_LIBS)

$(CVT_ASM_TARGET): $(CVT_ASM_OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BASIC_TARGET): $(BASIC_OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

# disk45 FUSE module needs FUSE3 headers
$(OBJ_DIR)/disk45_fuse.o: $(SRC_DIR)/disk45_fuse.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(FUSE3_CFLAGS) -c -o $@ $<

# disk45_main needs HAVE_FUSE3 define
$(OBJ_DIR)/disk45_main.o: $(SRC_DIR)/disk45_main.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(FUSE3_CFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

-include $(OBJ_DIR)/*.d

lib: all
	@$(MAKE) -C lib

clean:
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
