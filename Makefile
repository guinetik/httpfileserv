# Makefile for httpfileserv

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS =

# Check if running on WSL
ifeq ($(shell uname -a | grep -i microsoft),)
    ON_WSL = 0
else
    ON_WSL = 1
    CFLAGS += -D_GNU_SOURCE -DWSL
endif

# Platform-specific settings
ifeq ($(OS),Windows_NT)
    # Windows settings
    PLATFORM_SRC = src/platform/windows/platform_windows.c
    PLATFORM_OBJ = obj/platform/windows/platform_windows.o
    LDFLAGS += -lws2_32
    EXE = bin/httpfileserv.exe
    MKDIR = mkdir -p
    RM = rm -f
else
    # Unix settings
    PLATFORM_SRC = src/platform/unix/platform_unix.c
    PLATFORM_OBJ = obj/platform/unix/platform_unix.o
    CFLAGS += -D_XOPEN_SOURCE=700 -D_GNU_SOURCE
    EXE = bin/httpfileserv
    MKDIR = mkdir -p
    RM = rm -f
endif

# Source files
SRC = src/httpfileserv.c src/http_response.c src/template.c src/utils.c src/platform/platform.c src/httpfileserv_lib.c
OBJ = $(SRC:src/%.c=obj/%.o)

# Include directories
INCLUDES = -Iinclude

# Default target
all: setup $(EXE)

# Setup directories
setup:
	$(MKDIR) bin
	$(MKDIR) obj/platform/windows
	$(MKDIR) obj/platform/unix

# Link the executable
$(EXE): $(OBJ) $(PLATFORM_OBJ)
	$(MKDIR) bin
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile object files
obj/%.o: src/%.c
	$(MKDIR) $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Compile platform-specific object files
$(PLATFORM_OBJ): $(PLATFORM_SRC)
	$(MKDIR) $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Clean up
clean:
	$(RM) $(OBJ) $(PLATFORM_OBJ) $(EXE)
	$(RM) -r obj bin

# Run the server
run: $(EXE)
	$(EXE) .

# Help
help:
	@echo "Makefile for httpfileserv"
	@echo ""
	@echo "Targets:"
	@echo "  all     - Build the executable"
	@echo "  clean   - Remove the executable"
	@echo "  run     - Run the executable serving the current directory"
	@echo ""
	@echo "Usage:"
	@echo "  make              - Build the executable"
	@echo "  make clean        - Remove the executable"
	@echo "  make run          - Run the executable"
	@echo ""
	@echo "Runtime usage:"
	@echo "  $(EXE) <directory_path>    - Serve the specified directory"

# Phony targets
.PHONY: all clean run help setup