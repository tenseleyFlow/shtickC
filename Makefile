# Makefile for shtick C port

# Compiler settings
# Note: -D_GNU_SOURCE enables additional features on GNU/Linux systems
# If you get warnings about format-truncation, they're handled in the code
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99 -D_GNU_SOURCE
TARGET = shtick

# Source files
SRCS = main.c config.c groups.c aliases.c env.c generator.c display.c utils.c escape.c
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

clean:
	rm -f $(TARGET) $(OBJS)

# Build with debug symbols
debug: CFLAGS += -g -DDEBUG
debug: clean $(TARGET)

# Static analysis
analyze:
	@echo "Running static analysis..."
	@command -v cppcheck >/dev/null 2>&1 && cppcheck --enable=all --suppress=missingIncludeSystem $(SRCS) $(HEADERS) || echo "cppcheck not found"
	@command -v clang-tidy >/dev/null 2>&1 && clang-tidy $(SRCS) -- $(CFLAGS) || echo "clang-tidy not found"

# Format code
format:
	@command -v clang-format >/dev/null 2>&1 && clang-format -i $(SRCS) $(HEADERS) || echo "clang-format not found"

.PHONY: all install uninstall clean test debug analyze format