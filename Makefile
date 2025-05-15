# Makefile for httpfileserv

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -I./include

# Target binary
TARGET = httpfileserv

# Directories
SRC_DIR = src
INCLUDE_DIR = include
OBJ_DIR = obj
BIN_DIR = bin

# Object files
OBJS = $(OBJ_DIR)/httpfileserv.o \
       $(OBJ_DIR)/utils.o \
       $(OBJ_DIR)/httpfileserv_lib.o \
       $(OBJ_DIR)/http_response.o

# Platform-specific objects
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    PLATFORM_OBJ = $(OBJ_DIR)/platform/unix/platform_unix.o
    PLATFORM_DIR = $(SRC_DIR)/platform/unix
else ifeq ($(UNAME_S),Darwin)  # macOS
    PLATFORM_OBJ = $(OBJ_DIR)/platform/unix/platform_unix.o
    PLATFORM_DIR = $(SRC_DIR)/platform/unix
else ifeq ($(OS),Windows_NT)
    PLATFORM_OBJ = $(OBJ_DIR)/platform/windows/platform_windows.o
    PLATFORM_DIR = $(SRC_DIR)/platform/windows
    CFLAGS += -D_WIN32
    LIBS = -lws2_32
else
    PLATFORM_OBJ = $(OBJ_DIR)/platform/unix/platform_unix.o
    PLATFORM_DIR = $(SRC_DIR)/platform/unix
endif

# Create directories
$(shell mkdir -p $(BIN_DIR) $(OBJ_DIR) $(OBJ_DIR)/platform/unix $(OBJ_DIR)/platform/windows)

# Default target
all: $(BIN_DIR)/$(TARGET)

# Linking
$(BIN_DIR)/$(TARGET): $(OBJS) $(PLATFORM_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

# Compiling main source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compiling platform-specific files
$(OBJ_DIR)/platform/unix/platform_unix.o: $(SRC_DIR)/platform/unix/platform_unix.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/platform/windows/platform_windows.o: $(SRC_DIR)/platform/windows/platform_windows.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean
clean:
	rm -rf $(OBJ_DIR)/* $(BIN_DIR)/$(TARGET)

# Install (Unix-like systems)
install: $(BIN_DIR)/$(TARGET)
	install -m 755 $(BIN_DIR)/$(TARGET) /usr/local/bin/

# Running
run: $(BIN_DIR)/$(TARGET)
	$(BIN_DIR)/$(TARGET) ./

# Help
help:
	@echo "Makefile for httpfileserv"
	@echo ""
	@echo "Targets:"
	@echo "  all     - Build the executable"
	@echo "  clean   - Remove the executable"
	@echo "  install - Install the executable to /usr/local/bin"
	@echo "  run     - Run the executable serving the current directory"
	@echo ""
	@echo "Usage:"
	@echo "  make              - Build the executable"
	@echo "  make clean        - Remove the executable"
	@echo "  make install      - Install the executable"
	@echo "  make run          - Run the executable"
	@echo ""
	@echo "Runtime usage:"
	@echo "  $(TARGET) <directory_path>    - Serve the specified directory"

# Phony targets
.PHONY: all clean install run help