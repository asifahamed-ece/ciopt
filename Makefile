# CiOpt Makefile
# (C) 2026 Asif Ahamed. MIT License.
#
# Usage:
#   make          - Build release
#   make debug    - Build with debug symbols and sanitizers
#   make clean    - Remove build artifacts

CC := gcc

SRCDIR := src
BUILDDIR := build
VENDORDIR := vendor

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

all: dirs $(CLI_BIN)

# Debug build
debug: CFLAGS := $(filter-out $(RELEASE_CFLAGS),$(CFLAGS)) $(DEBUG_CFLAGS)
debug: dirs $(CLI_BIN)

# Test build & execute
test: dirs $(BUILDDIR)/tests/test_vector.exe $(BUILDDIR)/tests/test_analyzer.exe
	$(BUILDDIR)\tests\test_vector.exe
	$(BUILDDIR)\tests\test_analyzer.exe

$(BUILDDIR)/tests/test_vector.exe: tests/test_vector.c $(BUILDDIR)/ciopt/utils/vector.o
	$(CC) $(CFLAGS) $^ -o $@

$(BUILDDIR)/tests/test_analyzer.exe: tests/test_analyzer.c $(CIOPT_OBJS) $(TS_OBJ) $(TS_C_OBJ)
	$(CC) $(CFLAGS) $^ -o $@

# Create build directories (cmd.exe compatible - works with MinGW make)
dirs:
	if not exist "$(BUILDDIR)\ciopt\utils" mkdir "$(BUILDDIR)\ciopt\utils"
	if not exist "$(BUILDDIR)\ciopt\analyzer" mkdir "$(BUILDDIR)\ciopt\analyzer"
	if not exist "$(BUILDDIR)\ciopt\reporting" mkdir "$(BUILDDIR)\ciopt\reporting"
	if not exist "$(BUILDDIR)\ciopt\parser" mkdir "$(BUILDDIR)\ciopt\parser"
	if not exist "$(BUILDDIR)\vendor" mkdir "$(BUILDDIR)\vendor"
	if not exist "$(BUILDDIR)\tests" mkdir "$(BUILDDIR)\tests"

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

$(CLI_BIN): $(CIOPT_OBJS) $(TS_OBJ) $(TS_C_OBJ) $(CLI_OBJ)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	if exist "$(BUILDDIR)" rmdir /s /q "$(BUILDDIR)"
	if exist "$(CLI_BIN).exe" del "$(CLI_BIN).exe"
	if exist "report.html" del "report.html"

help:
	@echo CiOpt - C Code Complexity Analysis Engine
	@echo.
	@echo Targets:
	@echo   make         - Build release binary
	@echo   make debug   - Build with debug symbols
	@echo   make test    - Build and run unit/integration tests
	@echo   make clean   - Remove build artifacts
	@echo   make help    - Show this help