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
TUNE_TARGET = $(BIN_DIR)/tune45
DBG45_TARGET = $(BIN_DIR)/dbg45
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

# (Each archive is made anew: ar rcs adds and replaces members but never
# drops one, so an object taken out of a list would stay in an existing archive.)

# lib45-common: Object format, symbols, config, utilities, diagnostics, debug info, source tracking
$(LIB_DIR)/lib45-common.a: $(addprefix $(OBJ_DIR)/, \
    O45Reader.o O45Writer.o O45DebugSections.o O45Emitter.o O45Archive.o \
    O45Linker.o Logger.o AssemblerOpcodeDatabase.o ConfigLoader.o)
	@mkdir -p $(LIB_DIR)
	@rm -f $@
	$(AR) rcs $@ $^

# lib45-c-compile: C compiler frontend (Lexer, Parser, AST, Validator)
$(LIB_DIR)/lib45-c-compile.a: $(addprefix $(OBJ_DIR)/, \
    Lexer.o Parser.o AST.o TypeSystem.o TypeInfo.o ConstEval.o)
	@mkdir -p $(LIB_DIR)
	@rm -f $@
	$(AR) rcs $@ $^

# lib45-ir: Intermediate representation (basic IR structures, no codegen)
$(LIB_DIR)/lib45-ir.a: $(addprefix $(OBJ_DIR)/, \
    IRBuilder.o IRBuilderLoopIdiom.o IRPrinter.o IROptimizer.o VRegAllocator.o)
	@mkdir -p $(LIB_DIR)
	@rm -f $@
	$(AR) rcs $@ $^

# lib45-opt: Optimization passes + code generation
$(LIB_DIR)/lib45-opt.a: $(addprefix $(OBJ_DIR)/, \
    IRCodeGen.o SacSlotShare.o ConstantFolder.o LoopInterchange.o FunctionAnalyzer.o \
    OptimizationCatalog.o AddressTemplateDetector.o)
	@mkdir -p $(LIB_DIR)
	@rm -f $@
	$(AR) rcs $@ $^

# lib45-codegen: Code generation & assembly (includes preprocessor & emitter)
$(LIB_DIR)/lib45-codegen.a: $(addprefix $(OBJ_DIR)/, \
    Preprocessor.o M65Emitter.o \
    AssemblerLexer.o AssemblerParser.o AssemblerExpression.o SymbolSuggester.o \
    AssemblerOptimizer.o AssemblerSimulatedOps.o AssemblerGenerator.o OpEffect.o)
	@mkdir -p $(LIB_DIR)
	@rm -f $@
	$(AR) rcs $@ $^

# lib45-phases: code of earlier development phases that no tool links (IPO,
# bank assignment, link-time optimization coordination, benchmarking, learning
# and tuning hooks, DWARF output, AST-level optimizers, field striping;
# O45Linker's analysis passes, see
# doc/architecture/cross-module-optimization.md). Built so it keeps compiling;
# a static library adds nothing to a program that does not use it.
$(LIB_DIR)/lib45-phases.a: $(addprefix $(OBJ_DIR)/, \
    AdaptiveLearnerIntegration.o AdaptiveThresholdAdjuster.o \
    AlgebraicSimplification.o \
    BankAwareCodeGenerator.o BankLayoutGenerator.o BankSetupOptimizer.o \
    BenchmarkComparativeAnalyzer.o BenchmarkCompilerIntegration.o \
    BenchmarkExecutor.o BenchmarkMetricsCollector.o \
    BenchmarkReportGenerator.o BenchmarkingSuite.o BranchOptimization.o \
    CallGraphAnalyzer.o CallPatternAnalyzer.o CoOptimizationApplier.o \
    CoOptimizationSelector.o \
    CommonSubexpressionElimination.o ComparativePerformanceAnalyzer.o \
    CompilationProfiler.o CompilationSignalCollector.o \
    CompilerDecisionLogic.o CompilerHookIntegrator.o CopyPropagation.o \
    CrossModuleEnhancer.o CrossModuleVariableDatabase.o DIEBuilder.o \
    DeadStoreElimination.o DebugInfoBuilder.o DependencyTracker.o \
    DevirtualizationDetector.o DevirtualizationHints.o EnhancedDiagnostic.o \
    EscapeAnalysis.o FarAddressCodeGenerator.o FarAddressMemorySupport.o \
    FeedbackCoordinator.o FieldCachingAnalyzer.o \
    FieldCachingLinkerIntegration.o \
    GlobalFunctionDatabase.o GlobalPointerFieldDatabase.o \
    GlobalValueNumbering.o HookIntegration.o HotSpotProfiler.o IPOAnalyzer.o \
    IPOProfiler.o IRSpecializationGenerator.o InlineSelector.o \
    InterTUPatternDetector.o IterationManager.o \
    LearnerBasedOptimizationSelector.o LearnerFeedbackRecorder.o \
    LineNumberProgram.o LinkTimeOptimizationCodeGenerator.o \
    LinkTimeOptimizationCoordinator.o LinkTimeOptimizationValidator.o \
    LoopIdiomRegistry.o LoopInvariantCodeMotion.o LoopOptimizer.o \
    MathLibraryOptimization.o MemoryBankAssigner.o O45DwarfSerialization.o \
    O45IRSerializer.o OnlineLearner.o OptimizationConstraintResolver.o \
    OptimizationController.o OptimizationDependencyAnalyzer.o \
    OptimizationEffectivenessCollector.o OptimizationHintCollector.o \
    OptimizationLearner.o OptimizationPatternAnalyzer.o \
    OptimizationProfileDatabase.o OptimizationSelector.o \
    PatternBasedOptimizationSelector.o PatternRecognitionEngine.o \
    PeepholeOptimization.o RedundantLoadElimination.o \
    RegisterResidentLoops.o ScopeManager.o SourceLocationTracker.o \
    SpecializationCodeGenerator.o SpecializationOptimizer.o \
    StrengthReduction.o StructFieldStriper.o TableDrivenDispatch.o \
    TemplateOptimizationPass.o TemplateOptimizationSystem.o \
    TemplateRegistry.o TuningHooks.o CrossModuleOptimizer.o \
    O45LinkerPhases.o O45WriterPhases.o IRCodeGenPhases.o \
    OptimizeStagePhases.o CodegenStagePhases.o)
	@mkdir -p $(LIB_DIR)
	@rm -f $@
	$(AR) rcs $@ $^

# lib45-basic: BASIC language features
$(LIB_DIR)/lib45-basic.a: $(addprefix $(OBJ_DIR)/, \
    BasicTokenizer.o PETSCIIEncoder.o BasicEmitter.o BasicPreprocessor.o \
    BasicDocGenerator.o BasicValidator.o LabelBasedSourceParser.o \
    SymbolExpressionEvaluator.o BasicMinifier.o BasicTooling.o \
    BasicCharacterMap.o BasicDocumentationGenerator.o BasicStructures.o \
    BasicArchiveSupport.o BasicStandardLibrary.o BasicVarRenamer.o BasicStructured.o \
    BasicDecompiler.o BasicFormatter.o BasicLister.o)
	@mkdir -p $(LIB_DIR)
	@rm -f $@
	$(AR) rcs $@ $^

# lib45-audio: Audio and procedural music generation (includes Phase 12-18)
$(LIB_DIR)/lib45-audio.a: $(addprefix $(OBJ_DIR)/, \
    Song.o Track.o Pattern.o Sequencer.o PlaybackEngine.o \
    Scale.o Chord.o ChordProgression.o \
    MelodicGenerator.o HarmonicGenerator.o RhythmicGenerator.o \
    ProceduralComposer.o \
    JazzComposer.o AmbientComposer.o TechnoComposer.o \
    FolkComposer.o ClassicalComposer.o ChiptureComposer.o RagtimeComposer.o \
    SIDChip.o AudioDriver.o DIGIAudio.o \
    KeyboardController.o SynthesizerUI.o \
    Effect.o EffectsChain.o)
	@mkdir -p $(LIB_DIR)
	@rm -f $@
	$(AR) rcs $@ $^

# lib45-tools: Disk utilities, format converters (includes optional FUSE3 support)
LIB45_TOOLS_OBJS = $(addprefix $(OBJ_DIR)/, \
    disk45_catalog.o disk45_sd.o SdCardImage.o DiskImage.o DiskImageFactory.o BAMOperations.o \
    D64Image.o D71Image.o D81Image.o D65Image.o ArkImage.o ArcImage.o \
    LnxImage.o TapImage.o T64Image.o G64Image.o D80Image.o GeosCvtImage.o \
    P00Image.o X64Image.o ZipcodeImage.o D90Image.o CmdImage.o NibImage.o \
    Gcr.o GcrTracks.o Lz77.o ZipArchive.o \
    GzipHelper.o AsmParser.o AsmWriter.o Ca45Parser.o Ca45Writer.o \
    Ca65Parser.o Ca65Writer.o AcmeParser.o AcmeWriter.o OscarParser.o \
    OscarWriter.o Merlin64Parser.o Merlin64Writer.o X65Parser.o X65Writer.o \
    FormatDetection.o KickAssemblerParser.o KickAssemblerWriter.o DialectReader.o)

ifeq ($(HAVE_FUSE3),1)
  LIB45_TOOLS_OBJS += $(OBJ_DIR)/disk45_fuse.o
endif

$(LIB_DIR)/lib45-tools.a: $(LIB45_TOOLS_OBJS)
	@mkdir -p $(LIB_DIR)
	@rm -f $@
	$(AR) rcs $@ $^

# ============================================================================
# BINARY BUILD RULES
# ============================================================================

all: $(CC_TARGET) $(CA_TARGET) $(CP_TARGET) $(NM_TARGET) $(LN_TARGET) \
     $(AR_TARGET) $(OD_TARGET) $(DISK_TARGET) $(CVT_ASM_TARGET) $(BASIC_TARGET) $(TUNE_TARGET) $(DBG45_TARGET) stdlib | $(LIB_DIR)/lib45-audio.a $(LIB_DIR)/lib45-phases.a

# Rebuild stdlib whenever cc45 or ca45 changes
stdlib: $(CC_TARGET) $(CA_TARGET) $(AR_TARGET)
	$(MAKE) -C lib

man: $(MAN_DIR)/cc45.1 $(MAN_DIR)/ca45.1 $(MAN_DIR)/cp45.1 $(MAN_DIR)/ln45.1 $(MAN_DIR)/nm45.1 $(MAN_DIR)/ar45.1 $(MAN_DIR)/objdump45.1 \
     $(MAN_DIR)/disk45.1 $(MAN_DIR)/basic45.1 $(MAN_DIR)/cvt_asm.1 $(MAN_DIR)/tune45.1 $(MAN_DIR)/dbg45.1

$(MAN_DIR)/%.1: doc/bin/%.md
	@mkdir -p $(MAN_DIR)
	pandoc -s -t man $< -o $@ -M title="$(basename $(notdir $@))" -M section="1" -M date="$(shell date +%F)" -M footer="$(basename $(notdir $@)) manual" -M header="User Commands"

# Define pipeline object files (compiler-only)
PIPELINE_OBJECTS = $(addprefix $(OBJ_DIR)/, \
    CompilationPipeline.o PreprocessStage.o ParseStage.o OptimizeStage.o \
    CodegenStage.o AssemblyStage.o LinkingStage.o)

# C Compiler (cc45) - uses all libraries plus pipeline objects
# Note: Link order matters - list libraries twice to resolve circular dependencies
# cc45 pipeline invokes ca45 externally, so ca45 must be built first
$(CC_TARGET): $(CA_TARGET)
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
$(LN_TARGET): $(LIB_DIR)/lib45-common.a
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ_DIR)/ln45_main.o $(LIB_DIR)/lib45-common.a

# Archiver (ar45) - uses common
# tune45, with reSIDfp (third_party/libresidfp, built as it is: its own
# warnings off) for tune45 play
RESIDFP_DIR = third_party/libresidfp/src
RESIDFP_SRCS = $(wildcard $(RESIDFP_DIR)/*.cpp) $(RESIDFP_DIR)/version.cc \
               $(RESIDFP_DIR)/resample/SincResampler.cpp $(RESIDFP_DIR)/residfp/residfp.cpp
RESIDFP_OBJS = $(patsubst $(RESIDFP_DIR)/%,$(OBJ_DIR)/residfp/%.o,$(RESIDFP_SRCS))
$(OBJ_DIR)/residfp/%.o: $(RESIDFP_DIR)/%
	@mkdir -p $(dir $@)
	$(CXX) -std=c++17 -O2 -pthread -w -I$(RESIDFP_DIR) -MMD -MP -c $< -o $@
$(LIB_DIR)/lib45-residfp.a: $(RESIDFP_OBJS) | $(LIB_DIR)
	$(AR) rcs $@ $^
$(OBJ_DIR)/TuneAudio.o: CXXFLAGS += -I$(RESIDFP_DIR)
TUNE_OBJS = $(OBJ_DIR)/tune45_main.o $(OBJ_DIR)/TuneCompiler.o $(OBJ_DIR)/TunePlayer.o $(OBJ_DIR)/TuneAudio.o \
            $(OBJ_DIR)/TuneCreate.o $(OBJ_DIR)/TuneMidi.o \
            $(OBJ_DIR)/BasicTooling.o $(OBJ_DIR)/Logger.o
$(TUNE_TARGET): $(TUNE_OBJS) $(LIB_DIR)/lib45-residfp.a
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -pthread -o $@ $(TUNE_OBJS) $(LIB_DIR)/lib45-residfp.a

# dbg45, the source-level debugger on xemu's monitor (#202)
DBG45_OBJS = $(OBJ_DIR)/dbg45_main.o $(OBJ_DIR)/Dbg45.o $(OBJ_DIR)/Dbg45Dap.o $(OBJ_DIR)/DbgFile.o $(OBJ_DIR)/XemuMonitor.o $(OBJ_DIR)/Logger.o
$(DBG45_TARGET): $(DBG45_OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(DBG45_OBJS)

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
$(DISK_TARGET): $(LIB_DIR)/lib45-tools.a $(LIB_DIR)/lib45-basic.a $(LIB_DIR)/lib45-common.a
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ_DIR)/disk45_main.o $(LIB_DIR)/lib45-tools.a $(LIB_DIR)/lib45-basic.a $(LIB_DIR)/lib45-common.a -lz -lsqlite3 $(FUSE3_LIBS)

# Assembly Format Converter (cvt_asm) - uses tools, codegen, opt + common
$(CVT_ASM_TARGET): $(OBJ_DIR)/cvt_asm_main.o | $(LIB_DIR)
$(CVT_ASM_TARGET): $(LIB_DIR)/lib45-tools.a $(LIB_DIR)/lib45-codegen.a \
                   $(LIB_DIR)/lib45-common.a
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ_DIR)/cvt_asm_main.o $(LIB_DIR)/lib45-tools.a $(LIB_DIR)/lib45-codegen.a $(LIB_DIR)/lib45-common.a

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

# Audio/procedural compilation rules
AUDIO_PROCEDURAL_SRCS = $(wildcard src/audio/procedural/*.cpp) $(wildcard src/audio/*.cpp)
AUDIO_PROCEDURAL_OBJS = $(patsubst src/audio/%.cpp,$(OBJ_DIR)/%.o,$(AUDIO_PROCEDURAL_SRCS)) \
                        $(patsubst src/audio/procedural/%.cpp,$(OBJ_DIR)/%.o,$(AUDIO_PROCEDURAL_SRCS))

$(OBJ_DIR)/Song.o: src/audio/Song.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/Track.o: src/audio/Track.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/Pattern.o: src/audio/Pattern.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/Sequencer.o: src/audio/Sequencer.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/PlaybackEngine.o: src/audio/PlaybackEngine.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/Scale.o: src/audio/procedural/Scale.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/Chord.o: src/audio/procedural/Chord.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/ChordProgression.o: src/audio/procedural/ChordProgression.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/MelodicGenerator.o: src/audio/procedural/MelodicGenerator.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/HarmonicGenerator.o: src/audio/procedural/HarmonicGenerator.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/RhythmicGenerator.o: src/audio/procedural/RhythmicGenerator.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/ProceduralComposer.o: src/audio/procedural/ProceduralComposer.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/JazzComposer.o: src/audio/procedural/JazzComposer.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/AmbientComposer.o: src/audio/procedural/AmbientComposer.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/TechnoComposer.o: src/audio/procedural/TechnoComposer.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/FolkComposer.o: src/audio/procedural/FolkComposer.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/ClassicalComposer.o: src/audio/procedural/ClassicalComposer.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/ChiptureComposer.o: src/audio/procedural/ChiptureComposer.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/RagtimeComposer.o: src/audio/procedural/RagtimeComposer.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Phase 16: MEGA65 Hardware Integration
$(OBJ_DIR)/SIDChip.o: src/audio/SIDChip.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/AudioDriver.o: src/audio/AudioDriver.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/DIGIAudio.o: src/audio/DIGIAudio.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Phase 17: Real-Time Synthesizer Control
$(OBJ_DIR)/KeyboardController.o: src/audio/KeyboardController.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/SynthesizerUI.o: src/audio/SynthesizerUI.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Phase 18: Audio Effects Processing
$(OBJ_DIR)/Effect.o: src/audio/Effect.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/EffectsChain.o: src/audio/EffectsChain.cpp | $(OBJ_DIR)
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
.PHONY: all clean test man test-mmemu test-stdlib test-regression test-zpcall stdlib
.PHONY: test-integration lib install install_local uninstall
.PHONY: uninstall_local cppcheck coverage coverage-build coverage-clean
.PHONY: coverage-report docker test-assembler test-const
.PHONY: test-restrict test-register test-cast-fold test-o45 test-move-fill
.PHONY: test-validation-parser test-validation-struct test-validation-const
.PHONY: test-validation-semantic test-validation-preprocessor test-opcodes
.PHONY: test-validation-linker test-segment-emission test-validation-assembler
.PHONY: test-validation-simops test-validation-directives test-validation-symbols
.PHONY: test-validation-segments test-validation-proc test-validation-addressing
.PHONY: test-validation-simops-extended test-objdump45 test-examples
.PHONY: test-phase12 test-phase13 test-phase14 test-phase15 test-phase16 test-phase17 test-phase18

# Build all lib45 libraries
lib45-libraries: $(LIB_DIR)/lib45-common.a $(LIB_DIR)/lib45-c-compile.a \
                $(LIB_DIR)/lib45-ir.a $(LIB_DIR)/lib45-opt.a \
                $(LIB_DIR)/lib45-codegen.a $(LIB_DIR)/lib45-phases.a \
                $(LIB_DIR)/lib45-basic.a $(LIB_DIR)/lib45-tools.a \
                $(LIB_DIR)/lib45-audio.a

# Show library dependency information
show-lib-deps:
	@echo "=== lib45 Library Dependencies ==="
	@echo "lib45-common.a        : O45 format, configuration, utilities (no deps)"
	@echo "lib45-c-compile.a     : C compiler frontend → lib45-common"
	@echo "lib45-ir.a            : Intermediate representation → lib45-common"
	@echo "lib45-opt.a           : Optimization passes → lib45-ir lib45-common"
	@echo "lib45-codegen.a       : Code generation, assembly → lib45-common"
	@echo "lib45-phases.a        : earlier phases' code no tool links (IPO, banks, LTO, DWARF, ...)"
	@echo "lib45-basic.a         : BASIC language features → lib45-tools lib45-common"
	@echo "lib45-tools.a         : Disk utilities, converters → lib45-common"
	@echo ""
	@echo "=== Binary Dependencies ==="
	@echo "cc45      : lib45-opt lib45-codegen lib45-ir lib45-c-compile lib45-common"
	@echo "ca45      : lib45-codegen lib45-common"
	@echo "cp45      : symlink to cc45"
	@echo "ln45      : lib45-common"
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

test: all lib $(BIN_DIR)/test_o45 $(BIN_DIR)/test_ast_walker $(BIN_DIR)/test_const_eval
	@echo "Running .o45 format and linker unit tests..."
	@$(TEST_O45_TARGET)
	@echo "Running the AST walker unit test..."
	@$(TEST_AST_WALKER_TARGET)
	@echo "Running the constant evaluator unit test..."
	@$(TEST_CONST_EVAL_TARGET)
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
	@echo "Running inline control tests..."
	@bash src/test/test_inline_control.sh
	@echo "Running optimization switch tests..."
	@bash src/test/test_optimizations.sh
	@echo "Running tune45 tests..."
	@bash src/test/test_tune45.sh
	@echo "Running debug information tests..."
	@bash src/test/test_debug_info.sh
	@echo "Running logging tests..."
	@bash src/test/test_logging.sh
	@echo "Running configuration tests..."
	@bash src/test/test_config.sh
	@echo "Running function-local state tests..."
	@bash src/test/test_function_locals.sh
	@echo "Running string escape tests..."
	@bash src/test/test_string_escapes.sh
	@echo "Running signedness tests..."
	@bash src/test/test_signedness.sh
	@echo "Running installed toolchain tests..."
	@bash src/test/test_install.sh
	@echo "Running cvt_asm tests..."
	@bash src/test/test_cvt_asm.sh
	@echo "Running shadow execution tests (cc45 vs host gcc)..."
	@bash src/test/test_shadow_ir.sh
	@echo "Running clobber tracking tests..."
	@bash src/test/test_clobber.sh
	@echo "Running library call convention tests..."
	@bash src/test/test_libcall_conv.sh
	@echo "Running xemu runtime tests..."
	@bash src/test/test_xemu.sh
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
	@echo "Running example tests..."
	@bash src/test/test_examples.sh

test-examples: all
	@bash src/test/test_examples.sh

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

test-inline: all
	@bash src/test/test_inline_control.sh

# Temporary: known failing tests being worked down (see src/test/test_failures.sh)
test-failures: all lib
	@bash src/test/test_failures.sh

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

bench-optimizer: all
	@bash src/test/bench_optimizer.sh

test-objdump45: all
	@bash src/test/test_objdump45.sh

test-opcodes: all
	@echo "Validating opcodes and addressing modes..."
	@bash src/test/test_opcodes.sh

# O45 format unit test
TEST_O45_TARGET = $(BIN_DIR)/test_o45
TEST_O45_OBJECTS = $(OBJ_DIR)/test_o45.o $(OBJ_DIR)/O45Writer.o $(OBJ_DIR)/O45DebugSections.o $(OBJ_DIR)/O45Reader.o $(OBJ_DIR)/O45IRSerializer.o $(OBJ_DIR)/O45Linker.o $(OBJ_DIR)/Logger.o $(OBJ_DIR)/O45Archive.o

$(TEST_O45_TARGET): $(TEST_O45_OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJ_DIR)/test_o45.o: src/test/test_o45.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

test-o45: $(TEST_O45_TARGET)
	@$(TEST_O45_TARGET)

# ASTWalker unit test: it reaches a call in every place one can be (#94)
TEST_AST_WALKER_TARGET = $(BIN_DIR)/test_ast_walker
$(TEST_AST_WALKER_TARGET): $(OBJ_DIR)/test_ast_walker.o $(LIB_DIR)/lib45-c-compile.a $(LIB_DIR)/lib45-common.a
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ_DIR)/test_ast_walker.o $(LIB_DIR)/lib45-c-compile.a $(LIB_DIR)/lib45-common.a

$(OBJ_DIR)/test_ast_walker.o: src/test/test_ast_walker.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# ConstEval unit test: constant expressions and their values (#93)
TEST_CONST_EVAL_TARGET = $(BIN_DIR)/test_const_eval
$(TEST_CONST_EVAL_TARGET): $(OBJ_DIR)/test_const_eval.o $(LIB_DIR)/lib45-c-compile.a $(LIB_DIR)/lib45-common.a
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ_DIR)/test_const_eval.o $(LIB_DIR)/lib45-c-compile.a $(LIB_DIR)/lib45-common.a

$(OBJ_DIR)/test_const_eval.o: src/test/test_const_eval.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

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
TEST_VALIDATION_LINKER_OBJECTS = $(OBJ_DIR)/test_validation_linker.o $(OBJ_DIR)/O45Reader.o $(OBJ_DIR)/O45Writer.o $(OBJ_DIR)/O45DebugSections.o $(OBJ_DIR)/O45IRSerializer.o $(OBJ_DIR)/O45Linker.o $(OBJ_DIR)/Logger.o $(OBJ_DIR)/O45Archive.o $(OBJ_DIR)/AssemblerOpcodeDatabase.o

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
TEST_SEGMENT_EMISSION_OBJECTS = $(OBJ_DIR)/test_segment_emission.o $(OBJ_DIR)/O45Reader.o $(OBJ_DIR)/O45Writer.o $(OBJ_DIR)/O45DebugSections.o $(OBJ_DIR)/O45IRSerializer.o $(OBJ_DIR)/O45Linker.o $(OBJ_DIR)/Logger.o $(OBJ_DIR)/O45Archive.o $(OBJ_DIR)/AssemblerOpcodeDatabase.o

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
	install -m 755 $(CC_TARGET) $(CA_TARGET) $(NM_TARGET) $(LN_TARGET) $(AR_TARGET) $(OD_TARGET) $(DISK_TARGET) $(CVT_ASM_TARGET) $(BASIC_TARGET) $(TUNE_TARGET) $(DBG45_TARGET) $(DESTDIR)$(BINDIR)
	ln -sf cc45 $(DESTDIR)$(BINDIR)/cp45
	install -d $(DESTDIR)$(LIBDIR)
	install -m 644 lib/build/c45.lib lib/build/c45_zp.lib lib/build/c45_stack.lib $(DESTDIR)$(LIBDIR)
	install -m 644 lib/build/crt0*.o45 lib/build/tuneplay.o45 $(DESTDIR)$(LIBDIR)
	install -d $(DESTDIR)$(INCDIR) $(DESTDIR)$(INCDIR)/sys
	install -m 644 lib/include/*.h $(DESTDIR)$(INCDIR)
	install -m 644 lib/include/sys/*.h $(DESTDIR)$(INCDIR)/sys
	install -d $(DESTDIR)$(MANDIR)
	if [ -d $(MAN_DIR) ] && ls $(MAN_DIR)/*.1 >/dev/null 2>&1; then \
		install -m 644 $(MAN_DIR)/*.1 $(DESTDIR)$(MANDIR); \
	fi

install_local:
	@$(MAKE) install PREFIX=$(HOME)/.local

uninstall_local:
	@$(MAKE) uninstall PREFIX=$(HOME)/.local

# Audio Phase Tests (Phases 12-16)
TEST_PHASE12_TARGET = $(BIN_DIR)/test_phase12_sequencer
TEST_PHASE12_OBJECTS = $(OBJ_DIR)/test_phase12_sequencer.o $(OBJ_DIR)/Song.o $(OBJ_DIR)/Track.o $(OBJ_DIR)/Pattern.o $(OBJ_DIR)/Sequencer.o $(OBJ_DIR)/PlaybackEngine.o

$(TEST_PHASE12_TARGET): $(TEST_PHASE12_OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJ_DIR)/test_phase12_sequencer.o: src/test-resources/test_phase12_sequencer.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

test-phase12: $(TEST_PHASE12_TARGET) lib
	@$(TEST_PHASE12_TARGET)

TEST_PHASE13_TARGET = $(BIN_DIR)/test_phase13_procedural
TEST_PHASE13_OBJECTS = $(OBJ_DIR)/test_phase13_procedural.o $(OBJ_DIR)/Scale.o $(OBJ_DIR)/Chord.o $(OBJ_DIR)/ChordProgression.o $(OBJ_DIR)/MelodicGenerator.o $(OBJ_DIR)/HarmonicGenerator.o $(OBJ_DIR)/RhythmicGenerator.o

$(TEST_PHASE13_TARGET): $(TEST_PHASE13_OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJ_DIR)/test_phase13_procedural.o: src/test-resources/test_phase13_procedural.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

test-phase13: $(TEST_PHASE13_TARGET) lib
	@$(TEST_PHASE13_TARGET)

TEST_PHASE14_TARGET = $(BIN_DIR)/test_phase14_genres
TEST_PHASE14_OBJECTS = $(OBJ_DIR)/test_phase14_genres.o $(OBJ_DIR)/ProceduralComposer.o $(OBJ_DIR)/JazzComposer.o $(OBJ_DIR)/AmbientComposer.o $(OBJ_DIR)/TechnoComposer.o $(OBJ_DIR)/FolkComposer.o $(OBJ_DIR)/ClassicalComposer.o $(OBJ_DIR)/ChiptureComposer.o $(OBJ_DIR)/RagtimeComposer.o $(OBJ_DIR)/Song.o $(OBJ_DIR)/Track.o $(OBJ_DIR)/Pattern.o $(OBJ_DIR)/Scale.o $(OBJ_DIR)/Chord.o $(OBJ_DIR)/ChordProgression.o $(OBJ_DIR)/MelodicGenerator.o $(OBJ_DIR)/HarmonicGenerator.o $(OBJ_DIR)/RhythmicGenerator.o

$(TEST_PHASE14_TARGET): $(TEST_PHASE14_OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJ_DIR)/test_phase14_genres.o: src/test-resources/test_phase14_genres.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

test-phase14: $(TEST_PHASE14_TARGET) lib
	@$(TEST_PHASE14_TARGET)

TEST_PHASE15_TARGET = $(BIN_DIR)/test_phase15_playback
TEST_PHASE15_OBJECTS = $(OBJ_DIR)/test_phase15_playback.o $(OBJ_DIR)/PlaybackEngine.o $(OBJ_DIR)/Song.o $(OBJ_DIR)/Track.o $(OBJ_DIR)/Pattern.o $(OBJ_DIR)/Sequencer.o

$(TEST_PHASE15_TARGET): $(TEST_PHASE15_OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJ_DIR)/test_phase15_playback.o: src/test-resources/test_phase15_playback.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

test-phase15: $(TEST_PHASE15_TARGET) lib
	@$(TEST_PHASE15_TARGET)

TEST_PHASE16_TARGET = $(BIN_DIR)/test_phase16_hardware
TEST_PHASE16_OBJECTS = $(OBJ_DIR)/test_phase16_hardware.o $(OBJ_DIR)/SIDChip.o $(OBJ_DIR)/AudioDriver.o $(OBJ_DIR)/DIGIAudio.o $(OBJ_DIR)/PlaybackEngine.o $(OBJ_DIR)/Song.o $(OBJ_DIR)/Track.o $(OBJ_DIR)/Pattern.o $(OBJ_DIR)/Sequencer.o

$(TEST_PHASE16_TARGET): $(TEST_PHASE16_OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJ_DIR)/test_phase16_hardware.o: src/test-resources/test_phase16_hardware.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

test-phase16: $(TEST_PHASE16_TARGET) lib
	@$(TEST_PHASE16_TARGET)

TEST_PHASE17_TARGET = $(BIN_DIR)/test_phase17_realtime_control
TEST_PHASE17_OBJECTS = $(OBJ_DIR)/test_phase17_realtime_control.o $(OBJ_DIR)/KeyboardController.o $(OBJ_DIR)/SynthesizerUI.o $(OBJ_DIR)/AudioDriver.o $(OBJ_DIR)/SIDChip.o $(OBJ_DIR)/DIGIAudio.o

$(TEST_PHASE17_TARGET): $(TEST_PHASE17_OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJ_DIR)/test_phase17_realtime_control.o: src/test-resources/test_phase17_realtime_control.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

test-phase17: $(TEST_PHASE17_TARGET) lib
	@$(TEST_PHASE17_TARGET)

TEST_PHASE18_TARGET = $(BIN_DIR)/test_phase18_effects
TEST_PHASE18_OBJECTS = $(OBJ_DIR)/test_phase18_effects.o $(OBJ_DIR)/Effect.o $(OBJ_DIR)/EffectsChain.o

$(TEST_PHASE18_TARGET): $(TEST_PHASE18_OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJ_DIR)/test_phase18_effects.o: src/test-resources/test_phase18_effects.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

test-phase18: $(TEST_PHASE18_TARGET) lib
	@$(TEST_PHASE18_TARGET)

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
	rm -f $(DESTDIR)$(BINDIR)/basic45 $(DESTDIR)$(BINDIR)/tune45 $(DESTDIR)$(BINDIR)/dbg45
	rm -rf $(DESTDIR)$(LIBDIR)
	rm -rf $(DESTDIR)$(INCDIR)
	rm -f $(DESTDIR)$(MANDIR)/cc45.1 $(DESTDIR)$(MANDIR)/ca45.1 $(DESTDIR)$(MANDIR)/cp45.1
	rm -f $(DESTDIR)$(MANDIR)/ln45.1 $(DESTDIR)$(MANDIR)/nm45.1 $(DESTDIR)$(MANDIR)/ar45.1
	rm -f $(DESTDIR)$(MANDIR)/objdump45.1 $(DESTDIR)$(MANDIR)/disk45.1
	rm -f $(DESTDIR)$(MANDIR)/basic45.1 $(DESTDIR)$(MANDIR)/cvt_asm.1 $(DESTDIR)$(MANDIR)/tune45.1 $(DESTDIR)$(MANDIR)/dbg45.1
