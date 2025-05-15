# Makefile for httpfileserv

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS =

# Platform-specific settings
ifeq ($(OS),Windows_NT)
    # Windows settings
    PLATFORM_SRC = src/platform/windows/platform_win.c
    PLATFORM_OBJ = obj/platform/windows/platform_win.o
    LDFLAGS += -lws2_32
    EXE = bin/httpfileserv.exe
    MKDIR = mkdir -p
    RM = rm -f
else
    # Unix settings
    PLATFORM_SRC = src/platform/unix/platform_unix.c
    PLATFORM_OBJ = obj/platform/unix/platform_unix.o
    EXE = bin/httpfileserv
    MKDIR = mkdir -p
    RM = rm -f
endif

# Source files
SRC = src/httpfileserv.c src/http_response.c src/template.c
OBJ = $(SRC:src/%.c=obj/%.o)

# Include directories
INCLUDES = -Isrc

# Default target
all: $(EXE)

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
.PHONY: all clean run help