CX = g++
GIT_HASH := $(shell git rev-parse --short HEAD 2>/dev/null || echo "unknown")
CXXFLAGS = -std=c++17 -Wall -Wextra -Iinclude -MMD -MP -DGIT_HASH='"$(GIT_HASH)"'
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

CC_SOURCES = $(SRC_DIR)/cc45_main.cpp
CA_SOURCES = $(SRC_DIR)/ca45_main.cpp

# Common objects
COMMON_SOURCES = $(SRC_DIR)/Lexer.cpp $(SRC_DIR)/Parser.cpp $(SRC_DIR)/AST.cpp $(SRC_DIR)/CodeGenerator.cpp $(SRC_DIR)/M65Emitter.cpp $(SRC_DIR)/Preprocessor.cpp $(SRC_DIR)/ConstantFolder.cpp $(SRC_DIR)/AssemblerOpcodeDatabase.cpp $(SRC_DIR)/O45Writer.cpp $(SRC_DIR)/O45Emitter.cpp
COMMON_OBJECTS = $(OBJ_DIR)/Lexer.o $(OBJ_DIR)/Parser.o $(OBJ_DIR)/AST.o $(OBJ_DIR)/CodeGenerator.o $(OBJ_DIR)/M65Emitter.o $(OBJ_DIR)/Preprocessor.o $(OBJ_DIR)/ConstantFolder.o $(OBJ_DIR)/AssemblerOpcodeDatabase.o $(OBJ_DIR)/O45Writer.o $(OBJ_DIR)/O45Emitter.o

CC_OBJECTS = $(OBJ_DIR)/cc45_main.o $(OBJ_DIR)/AssemblerLexer.o $(OBJ_DIR)/AssemblerParser.o $(OBJ_DIR)/AssemblerExpression.o $(OBJ_DIR)/AssemblerOptimizer.o $(OBJ_DIR)/AssemblerSimulatedOps.o $(OBJ_DIR)/AssemblerGenerator.o $(COMMON_OBJECTS)
CA_OBJECTS = $(OBJ_DIR)/ca45_main.o $(OBJ_DIR)/AssemblerLexer.o $(OBJ_DIR)/AssemblerParser.o $(OBJ_DIR)/AssemblerExpression.o $(OBJ_DIR)/AssemblerOptimizer.o $(OBJ_DIR)/AssemblerSimulatedOps.o $(OBJ_DIR)/AssemblerGenerator.o $(COMMON_OBJECTS)

MAN_DIR = man

.PHONY: all clean test man test-mmemu test-zpcall lib

NM_OBJECTS = $(OBJ_DIR)/nm45_main.o $(OBJ_DIR)/O45Reader.o $(OBJ_DIR)/O45Writer.o $(OBJ_DIR)/O45Linker.o $(OBJ_DIR)/O45Archive.o
LN_OBJECTS = $(OBJ_DIR)/ln45_main.o $(OBJ_DIR)/O45Reader.o $(OBJ_DIR)/O45Writer.o $(OBJ_DIR)/O45Linker.o $(OBJ_DIR)/O45Archive.o
AR_OBJECTS = $(OBJ_DIR)/ar45_main.o $(OBJ_DIR)/O45Reader.o $(OBJ_DIR)/O45Writer.o $(OBJ_DIR)/O45Archive.o
OD_OBJECTS = $(OBJ_DIR)/objdump45_main.o $(OBJ_DIR)/O45Reader.o $(OBJ_DIR)/O45Writer.o $(OBJ_DIR)/O45Linker.o $(OBJ_DIR)/O45Archive.o $(OBJ_DIR)/AssemblerOpcodeDatabase.o

all: $(CC_TARGET) $(CA_TARGET) $(CP_TARGET) $(NM_TARGET) $(LN_TARGET) $(AR_TARGET) $(OD_TARGET)

man: $(MAN_DIR)/cc45.1 $(MAN_DIR)/ca45.1

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

test: all
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
	@echo "Validating mmemu-cli integration..."
	@bash src/test/test_mmemu.sh
	@$(MAKE) test-opcodes
	@echo "Running objdump45 tests..."
	@bash src/test/test_objdump45.sh

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

test-mmemu: all
	@bash src/test/test_mmemu.sh

test-zpcall: all
	@bash src/test/test_zpcall.sh

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
