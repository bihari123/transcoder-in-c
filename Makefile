# Project structure and build configuration

# Directories
SRC_DIR := src
INC_DIR := include
BUILD_DIR := build
BIN_DIR := .

# Compiler and flags
CC := gcc
CFLAGS := -Wall -Wextra -O2 -g -I$(INC_DIR)
LDFLAGS := -lavcodec -lavformat -lavutil -lavdevice -lavfilter -lswscale -lpthread

# Source files
SRCS := $(wildcard $(SRC_DIR)/*.c)

# Object files
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Header files for dependency tracking
DEPS := $(wildcard $(INC_DIR)/*.h)

# Output binary
TARGET := $(BIN_DIR)/transcoder

# Default target
all: directories $(TARGET)

# Create necessary directories
directories:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BIN_DIR)

# Linking
$(TARGET): $(OBJS)
	@echo "Linking $(TARGET)..."
	@$(CC) -o $@ $^ $(LDFLAGS)
	@echo "Build complete!"

# Compilation
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(DEPS)
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

# Clean built files
clean:
	@echo "Cleaning..."
	@rm -rf $(BUILD_DIR)
	@rm -f $(TARGET)
	@echo "Clean complete!"

# Install dependencies (Ubuntu/Debian)
deps:
	@echo "Installing dependencies..."
	@sudo apt-get update && sudo apt-get install -y \
		build-essential \
		libavcodec-dev \
		libavformat-dev \
		libavutil-dev \
		libavdevice-dev \
		libavfilter-dev \
		libswscale-dev
	@echo "Dependencies installed!"

# Debug build
debug: CFLAGS += -DDEBUG_MODE=1 -g
debug: all

# Help target
help:
	@echo "Available targets:"
	@echo "  all      - Build the project (default)"
	@echo "  debug    - Build with debug flags"
	@echo "  clean    - Remove built files"
	@echo "  deps     - Install dependencies (Ubuntu/Debian)"
	@echo "  help     - Show this help message"

.PHONY: all directories clean deps debug help
