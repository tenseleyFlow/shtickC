# Makefile for shtick C port

# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99 -D_GNU_SOURCE
TARGET = shtick

# Source files - Including all required files
SRCS = main.c config.c groups.c aliases.c env.c functions.c generator.c display.c utils.c escape.c completions.c source_cmd.c settings.c backup.c batch.c
OBJS = $(SRCS:.c=.o)
HEADERS = shtick.h

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Compile source files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Installation
install: $(TARGET)
	@echo "Installing shtick..."
	@mkdir -p $(HOME)/.local/bin
	@cp $(TARGET) $(HOME)/.local/bin/
	@echo "✓ Installed to ~/.local/bin/shtick"
	@echo "Make sure ~/.local/bin is in your PATH"

uninstall:
	@echo "Uninstalling shtick..."
	@rm -f $(HOME)/.local/bin/shtick
	@rm -rf $(HOME)/.config/shtick
	@echo "✓ Uninstalled"

clean:
	rm -f $(TARGET) $(OBJS)

# Build with debug symbols
debug: CFLAGS += -g -DDEBUG
debug: clean $(TARGET)

# Run the test harness
test: $(TARGET)
	@echo "Running test suite..."
	@chmod +x test_harness.sh
	@./test_harness.sh

# Quick smoke test
smoke-test: $(TARGET)
	@echo "Running smoke test..."
	@./$(TARGET) status >/dev/null && echo "✓ Status command works"
	@./$(TARGET) init >/dev/null && echo "✓ Init command works"
	@./$(TARGET) groups >/dev/null && echo "✓ Groups command works"

# Generate shell completions
completions: $(TARGET)
	@echo "Generating shell completions..."
	@./$(TARGET) completions bash 2>/dev/null || echo "Add completions command to generate"
	@./$(TARGET) completions zsh 2>/dev/null || true
	@./$(TARGET) completions fish 2>/dev/null || true

# Static analysis
analyze:
	@echo "Running static analysis..."
	@command -v cppcheck >/dev/null 2>&1 && cppcheck --enable=all --suppress=missingIncludeSystem $(SRCS) $(HEADERS) || echo "cppcheck not found"

# Format code
format:
	@command -v clang-format >/dev/null 2>&1 && clang-format -i $(SRCS) $(HEADERS) || echo "clang-format not found"

# Help
help:
	@echo "shtick build targets:"
	@echo "  make         - Build shtick"
	@echo "  make install - Install to ~/.local/bin"
	@echo "  make clean   - Remove build artifacts"
	@echo "  make debug   - Build with debug symbols"
	@echo "  make test    - Run full test suite"
	@echo "  make smoke-test - Quick functionality check"
	@echo "  make analyze - Run static analysis"
	@echo "  make format  - Format code with clang-format"
	@echo "  make completions - Generate shell completions"

.PHONY: all install uninstall clean test smoke-test debug analyze format completions help