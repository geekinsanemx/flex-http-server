CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2
LDFLAGS = -lhackrf -lcrypt -lssl -lcrypto

# Target executable
TARGET = hackrf_http_server

# Source files
SOURCES = main.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Default target
all: $(TARGET)

# Build the main executable
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

# Compile source files to object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET) flexserver_output.iq

# Install dependencies (Ubuntu/Debian)
deps:
	sudo apt-get update
	sudo apt-get install -y libhackrf-dev libssl-dev apache2-utils

# Create default passwords file
passwords:
	@if [ ! -f passwords ]; then \
		echo "Creating default passwords file..."; \
		htpasswd -cm passwords admin; \
	else \
		echo "Passwords file already exists."; \
	fi

# Help target
help:
	@echo "Available targets:"
	@echo "  all       - Build the hackrf_http_server executable"
	@echo "  clean     - Remove build artifacts"
	@echo "  deps      - Install required dependencies (Ubuntu/Debian)"
	@echo "  passwords - Create passwords file with htpasswd"
	@echo "  help      - Show this help message"

.PHONY: all clean deps passwords help
