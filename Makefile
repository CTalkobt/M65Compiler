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

CC_SOURCES = $(SRC_DIR)/cc45_main.cpp
CA_SOURCES = $(SRC_DIR)/ca45_main.cpp

# Common objects
COMMON_SOURCES = $(SRC_DIR)/Lexer.cpp $(SRC_DIR)/Parser.cpp $(SRC_DIR)/AST.cpp $(SRC_DIR)/CodeGenerator.cpp $(SRC_DIR)/M65Emitter.cpp $(SRC_DIR)/Preprocessor.cpp $(SRC_DIR)/ConstantFolder.cpp $(SRC_DIR)/LoopOptimizer.cpp $(SRC_DIR)/AssemblerOpcodeDatabase.cpp $(SRC_DIR)/O45Writer.cpp $(SRC_DIR)/O45Emitter.cpp
COMMON_OBJECTS = $(OBJ_DIR)/Lexer.o $(OBJ_DIR)/Parser.o $(OBJ_DIR)/AST.o $(OBJ_DIR)/CodeGenerator.o $(OBJ_DIR)/M65Emitter.o $(OBJ_DIR)/Preprocessor.o $(OBJ_DIR)/ConstantFolder.o $(OBJ_DIR)/LoopOptimizer.o $(OBJ_DIR)/AssemblerOpcodeDatabase.o $(OBJ_DIR)/O45Writer.o $(OBJ_DIR)/O45Emitter.o

CC_OBJECTS = $(OBJ_DIR)/cc45_main.o $(OBJ_DIR)/AssemblerLexer.o $(OBJ_DIR)/AssemblerParser.o $(OBJ_DIR)/AssemblerExpression.o $(OBJ_DIR)/AssemblerOptimizer.o $(OBJ_DIR)/AssemblerSimulatedOps.o $(OBJ_DIR)/AssemblerGenerator.o $(OBJ_DIR)/IRBuilder.o $(OBJ_DIR)/IRPrinter.o $(OBJ_DIR)/IRCodeGen.o $(OBJ_DIR)/IROptimizer.o $(OBJ_DIR)/VRegAllocator.o $(COMMON_OBJECTS)
CA_OBJECTS = $(OBJ_DIR)/ca45_main.o $(OBJ_DIR)/AssemblerLexer.o $(OBJ_DIR)/AssemblerParser.o $(OBJ_DIR)/AssemblerExpression.o $(OBJ_DIR)/AssemblerOptimizer.o $(OBJ_DIR)/AssemblerSimulatedOps.o $(OBJ_DIR)/AssemblerGenerator.o $(COMMON_OBJECTS)

MAN_DIR = man

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
LIBDIR ?= $(PREFIX)/lib/cc45
INCDIR ?= $(PREFIX)/include/cc45
MANDIR ?= $(PREFIX)/share/man/man1

.PHONY: all clean test man test-mmemu test-stdlib test-regression test-zpcall test-integration bench bench-save lib install install_local uninstall cppcheck coverage coverage-build coverage-clean coverage-report docker

cppcheck:
	cppcheck --enable=warning,performance,portability --inline-suppr -I include/ src/main/

NM_OBJECTS = $(OBJ_DIR)/nm45_main.o $(OBJ_DIR)/O45Reader.o $(OBJ_DIR)/O45Writer.o $(OBJ_DIR)/O45Linker.o $(OBJ_DIR)/O45Archive.o
LN_OBJECTS = $(OBJ_DIR)/ln45_main.o $(OBJ_DIR)/O45Reader.o $(OBJ_DIR)/O45Writer.o $(OBJ_DIR)/O45Linker.o $(OBJ_DIR)/O45Archive.o
AR_OBJECTS = $(OBJ_DIR)/ar45_main.o $(OBJ_DIR)/O45Reader.o $(OBJ_DIR)/O45Writer.o $(OBJ_DIR)/O45Archive.o
OD_OBJECTS = $(OBJ_DIR)/objdump45_main.o $(OBJ_DIR)/O45Reader.o $(OBJ_DIR)/O45Writer.o $(OBJ_DIR)/O45Linker.o $(OBJ_DIR)/O45Archive.o $(OBJ_DIR)/AssemblerOpcodeDatabase.o
DISK_OBJECTS = $(OBJ_DIR)/disk45_main.o $(OBJ_DIR)/disk45_catalog.o $(OBJ_DIR)/DiskImage.o $(OBJ_DIR)/DiskImageFactory.o $(OBJ_DIR)/D64Image.o $(OBJ_DIR)/D71Image.o $(OBJ_DIR)/D81Image.o $(OBJ_DIR)/D65Image.o $(OBJ_DIR)/ArkImage.o $(OBJ_DIR)/ArcImage.o $(OBJ_DIR)/LnxImage.o $(OBJ_DIR)/TapImage.o $(OBJ_DIR)/T64Image.o $(OBJ_DIR)/G64Image.o $(OBJ_DIR)/D80Image.o $(OBJ_DIR)/GeosCvtImage.o $(OBJ_DIR)/P00Image.o $(OBJ_DIR)/X64Image.o $(OBJ_DIR)/ZipcodeImage.o $(OBJ_DIR)/D90Image.o $(OBJ_DIR)/CmdImage.o $(OBJ_DIR)/NibImage.o $(OBJ_DIR)/GzipHelper.o
ifeq ($(HAVE_FUSE3),1)
  DISK_OBJECTS += $(OBJ_DIR)/disk45_fuse.o
endif

all: $(CC_TARGET) $(CA_TARGET) $(CP_TARGET) $(NM_TARGET) $(LN_TARGET) $(AR_TARGET) $(OD_TARGET) $(DISK_TARGET)

man: $(MAN_DIR)/cc45.1 $(MAN_DIR)/ca45.1 $(MAN_DIR)/cp45.1 $(MAN_DIR)/ln45.1 $(MAN_DIR)/nm45.1 $(MAN_DIR)/ar45.1 $(MAN_DIR)/objdump45.1

$(MAN_DIR)/%.1: doc/%.md
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

# disk45 FUSE module needs FUSE3 headers
$(OBJ_DIR)/disk45_fuse.o: $(SRC_DIR)/disk45_fuse.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(FUSE3_CFLAGS) -c -o $@ $<

# disk45_main needs HAVE_FUSE3 define
$(OBJ_DIR)/disk45_main.o: $(SRC_DIR)/disk45_main.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(FUSE3_CFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

-include $(OBJ_DIR)/*.d

lib: all
	@$(MAKE) -C lib

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) build
	@$(MAKE) -C bug clean
	@$(MAKE) -C lib clean

test: all lib
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
TEST_O45_OBJECTS = $(OBJ_DIR)/test_o45.o $(OBJ_DIR)/O45Writer.o $(OBJ_DIR)/O45Reader.o $(OBJ_DIR)/O45Linker.o $(OBJ_DIR)/O45Archive.o

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
TEST_VALIDATION_LINKER_OBJECTS = $(OBJ_DIR)/test_validation_linker.o $(OBJ_DIR)/O45Reader.o $(OBJ_DIR)/O45Writer.o $(OBJ_DIR)/O45Linker.o $(OBJ_DIR)/O45Archive.o $(OBJ_DIR)/AssemblerOpcodeDatabase.o

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
TEST_SEGMENT_EMISSION_OBJECTS = $(OBJ_DIR)/test_segment_emission.o $(OBJ_DIR)/O45Reader.o $(OBJ_DIR)/O45Writer.o $(OBJ_DIR)/O45Linker.o $(OBJ_DIR)/O45Archive.o $(OBJ_DIR)/AssemblerOpcodeDatabase.o

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
	install -m 755 $(CC_TARGET) $(CA_TARGET) $(NM_TARGET) $(LN_TARGET) $(AR_TARGET) $(OD_TARGET) $(DESTDIR)$(BINDIR)
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
	rm -f $(DESTDIR)$(BINDIR)/objdump45
	rm -rf $(DESTDIR)$(LIBDIR)
	rm -rf $(DESTDIR)$(INCDIR)
	rm -f $(DESTDIR)$(MANDIR)/cc45.1 $(DESTDIR)$(MANDIR)/ca45.1 $(DESTDIR)$(MANDIR)/cp45.1
	rm -f $(DESTDIR)$(MANDIR)/ln45.1 $(DESTDIR)$(MANDIR)/nm45.1 $(DESTDIR)$(MANDIR)/ar45.1
	rm -f $(DESTDIR)$(MANDIR)/objdump45.1
