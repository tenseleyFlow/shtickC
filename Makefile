# Makefile for shtick C port

# Compiler settings
# Note: -D_GNU_SOURCE enables additional features on GNU/Linux systems
# If you get warnings about format-truncation, they're handled in the code
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99 -D_GNU_SOURCE
TARGET = shtick

# Source files - Added functions.c
SRCS = main.c config.c groups.c aliases.c env.c functions.c generator.c display.c utils.c escape.c
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

# Run tests (if you add tests later)
test: $(TARGET)
	@echo "Running tests..."
	@./$(TARGET) status
	@echo "✓ Basic test passed"

# Static analysis
analyze:
	@echo "Running static analysis..."
	@command -v cppcheck >/dev/null 2>&1 && cppcheck --enable=all --suppress=missingIncludeSystem $(SRCS) $(HEADERS) || echo "cppcheck not found"
	@command -v clang-tidy >/dev/null 2>&1 && clang-tidy $(SRCS) -- $(CFLAGS) || echo "clang-tidy not found"

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
	@echo "  make test    - Run basic tests"
	@echo "  make analyze - Run static analysis"
	@echo "  make format  - Format code with clang-format"

.PHONY: all install uninstall clean test debug analyze format help