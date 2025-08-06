# Makefile for FLEX HTTP/TCP Server
# =================================

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -pthread
DEBUG_CXXFLAGS = -std=c++17 -Wall -Wextra -g -pthread -DDEBUG

# Directories
SRC_DIR = .
INC_DIR = include
TINYFLEX_DIR = ../tinyflex
BUILD_DIR = build

# Target executable
TARGET = flex_http_server
DEBUG_TARGET = flex_http_server_debug

# Source files
SOURCES = main.cpp

# Header files (for dependency tracking)
HEADERS = $(INC_DIR)/config.hpp \
          $(INC_DIR)/tcp_util.hpp \
          $(INC_DIR)/http_util.hpp \
          $(INC_DIR)/flex_at_util.hpp

# TinyFlex library
TINYFLEX_DIR = ../tinyflex
TINYFLEX_INC = -I$(TINYFLEX_DIR)

# Libraries
LIBS = -lcrypt

# Object files
OBJECTS = $(BUILD_DIR)/main.o

# Default target
all: $(TARGET)

# Debug build
debug: $(DEBUG_TARGET)

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Build TinyFlex library if it doesn't exist
$(TINYFLEX_LIB):
	@echo "Building TinyFlex library..."
	@if [ ! -d "$(TINYFLEX_DIR)" ]; then \
		echo "Error: TinyFlex directory not found at $(TINYFLEX_DIR)"; \
		echo "Please ensure tinyflex is available in the parent directory"; \
		echo "Or update TINYFLEX_DIR in the Makefile"; \
		exit 1; \
	fi
	@$(MAKE) -C $(TINYFLEX_DIR)

# Main target
$(TARGET): $(BUILD_DIR) $(TINYFLEX_LIB) $(OBJECTS)
	@echo "Linking $(TARGET)..."
	$(CXX) $(CXXFLAGS) $(OBJECTS) $(TINYFLEX_LIB) $(LIBS) -o $(TARGET)
	@echo "Build complete: $(TARGET)"

# Debug target
$(DEBUG_TARGET): $(BUILD_DIR) $(TINYFLEX_LIB) $(BUILD_DIR)/main_debug.o
	@echo "Linking $(DEBUG_TARGET)..."
	$(CXX) $(DEBUG_CXXFLAGS) $(BUILD_DIR)/main_debug.o $(TINYFLEX_LIB) $(LIBS) -o $(DEBUG_TARGET)
	@echo "Debug build complete: $(DEBUG_TARGET)"

# Compile main.cpp
$(BUILD_DIR)/main.o: $(SRC_DIR)/main.cpp $(HEADERS) | $(BUILD_DIR)
	@echo "Compiling main.cpp..."
	$(CXX) $(CXXFLAGS) $(TINYFLEX_INC) -I$(INC_DIR) -c $(SRC_DIR)/main.cpp -o $@

# Compile main.cpp for debug
$(BUILD_DIR)/main_debug.o: $(SRC_DIR)/main.cpp $(HEADERS) | $(BUILD_DIR)
	@echo "Compiling main.cpp (debug)..."
	$(CXX) $(DEBUG_CXXFLAGS) $(TINYFLEX_INC) -I$(INC_DIR) -c $(SRC_DIR)/main.cpp -o $@

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR)
	rm -f $(TARGET) $(DEBUG_TARGET)
	@echo "Clean complete."

# Clean everything including TinyFlex
distclean: clean
	@echo "Cleaning TinyFlex library..."
	@if [ -d "$(TINYFLEX_DIR)" ]; then \
		$(MAKE) -C $(TINYFLEX_DIR) clean; \
	fi
	@echo "Deep clean complete."

# Install target (system-wide installation)
install: $(TARGET)
	@echo "Installing $(TARGET)..."
	sudo cp $(TARGET) /usr/local/bin/
	sudo chmod +x /usr/local/bin/$(TARGET)
	@echo "Installation complete. You can now run: $(TARGET)"

# Uninstall target
uninstall:
	@echo "Uninstalling $(TARGET)..."
	sudo rm -f /usr/local/bin/$(TARGET)
	@echo "Uninstall complete."

# Check dependencies
check-deps:
	@echo "Checking build dependencies..."
	@command -v $(CXX) >/dev/null 2>&1 || { echo "Error: g++ compiler not found. Install: sudo apt install build-essential"; exit 1; }
	@echo "✓ g++ compiler found"
	@echo "✓ Standard libraries should be available"
	@echo "Checking libcrypt..."
	@echo '#include <crypt.h>' | $(CXX) -x c++ -c - -o /tmp/test_crypt.o 2>/dev/null || { echo "Error: libcrypt-dev not found. Install: sudo apt install libcrypt-dev"; rm -f /tmp/test_crypt.o; exit 1; }
	@rm -f /tmp/test_crypt.o
	@echo "✓ libcrypt found"
	@echo "All dependencies satisfied!"

# Test FLEX device connection
test-flex:
	@echo "Testing FLEX device connection..."
	@if [ ! -c "$(FLEX_DEVICE)" ]; then \
		echo "Error: FLEX device not found: $(FLEX_DEVICE)"; \
		echo "Available devices:"; \
		ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null || echo "  (no USB/ACM devices found)"; \
		exit 1; \
	fi
	@echo "✓ FLEX device found: $(FLEX_DEVICE)"
	@if [ ! -r "$(FLEX_DEVICE)" ] || [ ! -w "$(FLEX_DEVICE)" ]; then \
		echo "⚠ Warning: Insufficient permissions for $(FLEX_DEVICE)"; \
		echo "  Fix with: sudo chmod 666 $(FLEX_DEVICE)"; \
		echo "  Or add user to dialout: sudo usermod -a -G dialout $$USER"; \
	else \
		echo "✓ Device permissions OK"; \
	fi
	@echo "FLEX device test complete."

# Test build (compile only, don't run)
test-build: $(TARGET)
	@echo "Build test successful!"

# Run with debug mode
run-debug: $(TARGET)
	@echo "Running $(TARGET) in debug mode..."
	./$(TARGET) --debug --verbose

# Run with verbose mode
run-verbose: $(TARGET)
	@echo "Running $(TARGET) in verbose mode..."
	./$(TARGET) --verbose

# Display configuration
show-config:
	@echo "FLEX HTTP Server Build Configuration:"
	@echo "  Compiler: $(CXX)"
	@echo "  Flags: $(CXXFLAGS)"
	@echo "  Target: $(TARGET)"
	@echo "  TinyFlex: $(TINYFLEX_DIR)"
	@echo "  Libraries: $(LIBS)"

# Read configuration from environment or use defaults
FLEX_DEVICE ?= /dev/ttyUSB0

# Help target
help:
	@echo "FLEX HTTP/TCP Server - Build System"
	@echo "==================================="
	@echo ""
	@echo "Available targets:"
	@echo "  all          - Build the server (default)"
	@echo "  debug        - Build debug version with symbols"
	@echo "  clean        - Remove build artifacts"
	@echo "  distclean    - Remove all build artifacts including TinyFlex"
	@echo "  install      - Install to /usr/local/bin (requires sudo)"
	@echo "  uninstall    - Remove from /usr/local/bin (requires sudo)"
	@echo ""
	@echo "Testing targets:"
	@echo "  check-deps   - Check build dependencies"
	@echo "  test-flex    - Test FLEX device connection"
	@echo "  test-build   - Test compilation only"
	@echo "  run-debug    - Run server in debug mode"
	@echo "  run-verbose  - Run server in verbose mode"
	@echo ""
	@echo "Information targets:"
	@echo "  show-config  - Display build configuration"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Quick start:"
	@echo "  make check-deps    # Check dependencies"
	@echo "  make              # Build the server"
	@echo "  make test-flex     # Test device connection"
	@echo "  make run-debug     # Test run without transmission"
	@echo ""
	@echo "Environment variables:"
	@echo "  FLEX_DEVICE=/dev/ttyUSB0  # Override device for testing"

# Mark targets that don't create files
.PHONY: all debug clean distclean install uninstall check-deps test-flex test-build run-debug run-verbose show-config help

# Default target
.DEFAULT_GOAL := all
