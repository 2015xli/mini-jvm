# =========================
# Makefile for kORP Project
# =========================

# Compiler and flags
CC       := clang
#CFLAGS   := -Wall -Wextra -O2
CFLAGS   :=  -w -O2
LDFLAGS  :=

# Project target
TARGET   := korpvm

# Source and build directories
SRC_DIR  := .
BUILD_DIR := build

# Automatically detect all subdirectories for includes
INC_DIRS := $(shell find $(SRC_DIR) -type d)
CFLAGS   += $(addprefix -I, $(INC_DIRS))

# Collect all .c source files recursively
SRC_FILES := $(shell find $(SRC_DIR) -type f -name "*.c")

# Generate parallel object file structure under build/
OBJ_FILES := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRC_FILES))

# Default rule
all: $(TARGET)

# Link all object files into final binary
$(TARGET): $(OBJ_FILES)
	@echo "Linking -> $@"
	$(CC) $(OBJ_FILES) -o $@ $(LDFLAGS)

# Compile each .c to .o under build/
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "Compiling $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build and binary
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

# Rebuild everything
rebuild: clean all

# Show what’s being built (debugging)
info:
	@echo "Include dirs:"; echo "$(INC_DIRS)" | tr ' ' '\n'
	@echo ""
	@echo "Source files:"; echo "$(SRC_FILES)" | tr ' ' '\n'

.PHONY: all clean rebuild info

