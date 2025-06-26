# Makefile for shtick C port

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99
TARGET = shtick
PREFIX = /usr/local

# Source files
SRCS = shtick.c
OBJS = $(SRCS:.c=.o)

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Compile source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Install the binary
install: $(TARGET)
	install -d $(PREFIX)/bin
	install -m 755 $(TARGET) $(PREFIX)/bin/

# Uninstall
uninstall:
	rm -f $(PREFIX)/bin/$(TARGET)

# Clean build artifacts
clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all install uninstall clean test