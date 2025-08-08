# Aurora Language Compiler Makefile
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11 -g
LDFLAGS = 
TARGET = aurora
INSTALL_DIR = /usr/local/bin

# Source files
SRCS = main.c lexer.c parser.c ast.c codegen_wasm.c
OBJS = $(SRCS:.c=.o)
HEADERS = aurora_types.h lexer.h parser.h ast.h codegen_wasm.h

# Detect OS for platform-specific settings
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    CFLAGS += -D_GNU_SOURCE
endif
ifeq ($(UNAME_S),Darwin)
    CFLAGS += -D_DARWIN_C_SOURCE
endif

# Default target
all: $(TARGET)

# Build the compiler
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)
	@echo "✓ Built $(TARGET) successfully"

# Compile source files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Debug build
debug: CFLAGS += -DDEBUG -g3 -O0
debug: clean $(TARGET)

# Run tests
test: $(TARGET)
	@echo "Running tests..."
	./$(TARGET) lex examples/test.aur
	@echo "✓ Tests passed"

# Install the compiler
install: $(TARGET)
	@echo "Installing $(TARGET) to $(INSTALL_DIR)..."
	@mkdir -p $(INSTALL_DIR)
	@cp $(TARGET) $(INSTALL_DIR)/
	@chmod +x $(INSTALL_DIR)/$(TARGET)
	@echo "✓ Installed successfully"

# Uninstall
uninstall:
	@echo "Removing $(TARGET) from $(INSTALL_DIR)..."
	@rm -f $(INSTALL_DIR)/$(TARGET)
	@echo "✓ Uninstalled successfully"

# Clean build artifacts
clean:
	rm -f $(OBJS) $(TARGET)
	rm -f *.wasm *.wat
	@echo "✓ Cleaned build artifacts"

# Format source code
format:
	@command -v clang-format >/dev/null 2>&1 || { echo "clang-format not found"; exit 1; }
	clang-format -i *.c *.h
	@echo "✓ Formatted source files"

# Static analysis
analyze:
	@command -v clang-tidy >/dev/null 2>&1 || { echo "clang-tidy not found"; exit 1; }
	clang-tidy *.c -- $(CFLAGS)

# Memory check with valgrind
memcheck: $(TARGET)
	@command -v valgrind >/dev/null 2>&1 || { echo "valgrind not found"; exit 1; }
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET) lex examples/test.aur

# Help
help:
	@echo "Aurora Compiler Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build the compiler (default)"
	@echo "  debug     - Build with debug symbols"
	@echo "  test      - Run tests"
	@echo "  install   - Install to system"
	@echo "  uninstall - Remove from system"
	@echo "  clean     - Remove build artifacts"
	@echo "  format    - Format source code"
	@echo "  analyze   - Run static analysis"
	@echo "  memcheck  - Check for memory leaks"
	@echo "  help      - Show this help"

.PHONY: all debug test install uninstall clean format analyze memcheck help