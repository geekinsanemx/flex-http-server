# TTGO HTTP/TCP Server Makefile
# ==============================

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2
LDFLAGS = -lcrypt

# Source files
SOURCES = main.cpp
HEADERS = include/config.hpp include/tcp_util.hpp include/http_util.hpp include/ttgo_util.hpp ../../tinyflex.h

# Target executable
TARGET = ttgo_http_server

# Default target
all: $(TARGET)

# Build the main executable
$(TARGET): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES) $(LDFLAGS)

# Clean build artifacts
clean:
	rm -f $(TARGET)

# Test build (debug mode)
debug: CXXFLAGS += -g -DDEBUG
debug: $(TARGET)

# Check dependencies
check-deps:
	@echo "Checking dependencies..."
	@which g++ > /dev/null || (echo "ERROR: g++ not found. Install: sudo apt install build-essential" && exit 1)
	@echo "✓ g++ found"
	@ldconfig -p | grep -q libcrypt || (echo "ERROR: libcrypt not found. Install: sudo apt install libcrypt-dev" && exit 1)
	@echo "✓ libcrypt found"
	@ls /dev/ttyACM* /dev/ttyUSB* 2>/dev/null || echo "WARNING: No USB serial devices found"
	@echo "Dependencies check complete."

# Test TTGO connection
test-ttgo:
	@echo "Testing TTGO device connection..."
	@if [ ! -c "$(shell grep TTGO_DEVICE config.ini | cut -d= -f2)" ]; then \
		echo "ERROR: TTGO device not found. Check TTGO_DEVICE in config.ini"; \
		exit 1; \
	fi
	@echo "✓ TTGO device found"
	@echo "Run './$(TARGET) --debug --verbose' to test without transmission"

# Development help
help:
	@echo "TTGO HTTP/TCP Server - Makefile targets:"
	@echo ""
	@echo "Building:"
	@echo "  all          - Build the server (default)"
	@echo "  debug        - Build with debug symbols"
	@echo "  clean        - Remove build artifacts"
	@echo ""
	@echo "Testing:"
	@echo "  check-deps   - Check build dependencies"
	@echo "  test-ttgo    - Test TTGO device connection"
	@echo ""
	@echo "Usage Examples:"
	@echo "  ./$(TARGET) --help"
	@echo "  ./$(TARGET) --verbose"
	@echo "  ./$(TARGET) --debug --verbose"
	@echo ""
	@echo "HTTP API Test:"
	@echo "  curl -X POST http://localhost:16180/ -u admin:passw0rd \\"
	@echo "    -H 'Content-Type: application/json' \\"
	@echo "    -d '{\"capcode\":1122334,\"message\":\"Test\",\"frequency\":916000000}'"
	@echo ""
	@echo "TCP Protocol Test:"
	@echo "  echo '1122334|Test Message|916000000' | nc localhost 16175"

# Phony targets
.PHONY: all clean debug check-deps test-ttgo help
