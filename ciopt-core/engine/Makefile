# CiOpt Makefile (Engine - run from within ciopt-core/engine/ or ciopt/ root)
# (C) 2026 Asif Ahamed. MIT License.
#
# Usage (from repo root):
#   cd ciopt-core/engine && make
#
# On Windows (from repo root):
#   cd engine && mingw32-make

CC := gcc

# Source root is the ciopt/ repository root
ROOTDIR := ../..
SRCDIR := $(ROOTDIR)/src
BUILDDIR := build
VENDORDIR := $(ROOTDIR)/vendor

# Tree-sitter paths
TS_DIR := $(VENDORDIR)/tree-sitter
TS_C_DIR := $(VENDORDIR)/tree-sitter-c
TS_INCLUDE := -I$(TS_DIR)/lib/include -I$(TS_C_DIR)/src
TS_SRC := $(TS_DIR)/lib/src/lib.c
TS_C_SRC := $(TS_C_DIR)/src/parser.c

# Sources
CIOPT_SRCS := $(wildcard $(SRCDIR)/ciopt/utils/*.c) \
              $(wildcard $(SRCDIR)/ciopt/analyzer/*.c) \
              $(wildcard $(SRCDIR)/ciopt/reporting/*.c) \
              $(wildcard $(SRCDIR)/ciopt/parser/*.c) \
              $(SRCDIR)/ciopt/api.c \
              $(SRCDIR)/ciopt/config.c

CIOPT_OBJS := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(CIOPT_SRCS))

# Tree-sitter objects
TS_OBJ := $(BUILDDIR)/vendor/tree-sitter-lib.o
TS_C_OBJ := $(BUILDDIR)/vendor/tree-sitter-c.o

CLI_SRC := $(SRCDIR)/main.c
CLI_OBJ := $(BUILDDIR)/main.o
CLI_BIN := ciopt

# OS-specific commands and settings
ifeq ($(OS),Windows_NT)
    EXE := .exe
    RUN_TEST_VECTOR := $(BUILDDIR)\tests\test_vector$(EXE)
    RUN_TEST_ANALYZER := $(BUILDDIR)\tests\test_analyzer$(EXE)
    CLEAN_CMD = \
        if exist "$(BUILDDIR)" rmdir /s /q "$(BUILDDIR)" && \
        if exist "$(CLI_BIN)$(EXE)" del "$(CLI_BIN)$(EXE)" && \
        if exist "report.html" del "report.html"
    DIRS_CMD = \
        if not exist "$(BUILDDIR)\ciopt\utils" mkdir "$(BUILDDIR)\ciopt\utils" && \
        if not exist "$(BUILDDIR)\ciopt\analyzer" mkdir "$(BUILDDIR)\ciopt\analyzer" && \
        if not exist "$(BUILDDIR)\ciopt\reporting" mkdir "$(BUILDDIR)\ciopt\reporting" && \
        if not exist "$(BUILDDIR)\ciopt\parser" mkdir "$(BUILDDIR)\ciopt\parser" && \
        if not exist "$(BUILDDIR)\vendor" mkdir "$(BUILDDIR)\vendor" && \
        if not exist "$(BUILDDIR)\tests" mkdir "$(BUILDDIR)\tests"
else
    EXE :=
    RUN_TEST_VECTOR := ./$(BUILDDIR)/tests/test_vector
    RUN_TEST_ANALYZER := ./$(BUILDDIR)/tests/test_analyzer
    CLEAN_CMD = rm -rf $(BUILDDIR) $(CLI_BIN) report.html
    DIRS_CMD = mkdir -p $(BUILDDIR)/ciopt/utils $(BUILDDIR)/ciopt/analyzer $(BUILDDIR)/ciopt/reporting $(BUILDDIR)/ciopt/parser $(BUILDDIR)/vendor $(BUILDDIR)/tests
endif

# Compiler flags
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic
CFLAGS += -I$(SRCDIR) $(TS_INCLUDE)
CFLAGS += -D_POSIX_C_SOURCE=200809L
CFLAGS += -D_DEFAULT_SOURCE
CFLAGS += -DHAVE_TREE_SITTER=1

# Release flags
RELEASE_CFLAGS := -O2 -DNDEBUG

# Debug flags
DEBUG_CFLAGS := -O0 -g3 -DCIOPT_DEBUG

# Default build: release
CFLAGS += $(RELEASE_CFLAGS)

.PHONY: all debug test clean help

all: dirs $(CLI_BIN)$(EXE)
	@echo ""
	@echo "Build successful! Run 'make help' for a quick start guide."

# Debug build
debug: CFLAGS := $(filter-out $(RELEASE_CFLAGS),$(CFLAGS)) $(DEBUG_CFLAGS)
debug: dirs $(CLI_BIN)$(EXE)

# Test build & execute
test: dirs $(BUILDDIR)/tests/test_vector$(EXE) $(BUILDDIR)/tests/test_analyzer$(EXE)
	$(RUN_TEST_VECTOR)
	$(RUN_TEST_ANALYZER)

$(BUILDDIR)/tests/test_vector$(EXE): $(ROOTDIR)/tests/test_vector.c $(BUILDDIR)/ciopt/utils/vector.o
	$(CC) $(CFLAGS) $^ -o $@

$(BUILDDIR)/tests/test_analyzer$(EXE): $(ROOTDIR)/tests/test_analyzer.c $(CIOPT_OBJS) $(TS_OBJ) $(TS_C_OBJ)
	$(CC) $(CFLAGS) $^ -o $@

# Create build directories
dirs:
	$(DIRS_CMD)

# Build rules
$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Tree-sitter library
$(TS_OBJ): $(TS_SRC)
	$(CC) $(CFLAGS) -I$(TS_DIR)/lib/src -c $< -o $@

# Tree-sitter C grammar
$(TS_C_OBJ): $(TS_C_SRC)
	$(CC) $(CFLAGS) -I$(TS_C_DIR)/src -c $< -o $@

$(CLI_OBJ): $(CLI_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

$(CLI_BIN)$(EXE): $(CIOPT_OBJS) $(TS_OBJ) $(TS_C_OBJ) $(CLI_OBJ)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	$(CLEAN_CMD)

help:
	@echo "========================================="
	@echo "  Welcome to CiOpt - C Complexity Analyzer"
	@echo "========================================="
	@echo ""
	@echo "Usage:"
	@echo "  ./$(CLI_BIN)$(EXE) analyze <file_or_directory> [options]"
	@echo "  ./$(CLI_BIN)$(EXE) --help                      Show all CLI options"
	@echo ""
	@echo "Examples:"
	@echo "  ./$(CLI_BIN)$(EXE) analyze examples/Stress_again.c -v"
	@echo "  ./$(CLI_BIN)$(EXE) analyze src/ --verbose"
	@echo ""
	@echo "Make Targets:"
	@echo "  make clean   Remove build artifacts"
	@echo "  make debug   Build with debug symbols"
	@echo "  make test    Run unit tests"