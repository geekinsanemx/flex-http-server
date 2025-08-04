#!/bin/bash
# Environment variables configuration for ttgo_http_server
# Usage: source vars.sh && ./ttgo_http_server [OPTIONS]

# Network Configuration
export BIND_ADDRESS="127.0.0.1"        # IP to bind servers (0.0.0.0 for all interfaces)
export SERIAL_LISTEN_PORT="16175"      # TCP serial protocol port (0 = disabled)
export HTTP_LISTEN_PORT="16180"        # HTTP JSON API port (0 = disabled)

# Authentication Configuration
export HTTP_AUTH_CREDENTIALS="passwords"  # Password file path (htpasswd format)

# TTGO Hardware Configuration
export TTGO_DEVICE="/dev/ttyACM0"      # Serial device path for TTGO module
export TTGO_BAUDRATE="115200"          # Serial communication baudrate
export TTGO_POWER="2"                  # TX power level (2-17)

# Default Parameters
export DEFAULT_FREQUENCY="916000000"   # Default frequency when not specified (916.0 MHz)

echo "=========================================="
echo "TTGO HTTP/TCP Server - Environment Setup"
echo "=========================================="
echo "Network Configuration:"
echo "  BIND_ADDRESS: $BIND_ADDRESS"
echo "  SERIAL_LISTEN_PORT: $SERIAL_LISTEN_PORT (TCP)"
echo "  HTTP_LISTEN_PORT: $HTTP_LISTEN_PORT (JSON API)"
echo "  HTTP_AUTH_CREDENTIALS: $HTTP_AUTH_CREDENTIALS"
echo ""
echo "TTGO Configuration:"
echo "  TTGO_DEVICE: $TTGO_DEVICE"
echo "  TTGO_BAUDRATE: $TTGO_BAUDRATE bps"
echo "  TTGO_POWER: $TTGO_POWER (range: 2-17)"
echo ""
echo "Default Parameters:"
echo "  DEFAULT_FREQUENCY: $DEFAULT_FREQUENCY Hz ($(echo "scale=6; $DEFAULT_FREQUENCY/1000000" | bc -l) MHz)"
echo ""
echo "=========================================="

# Check if TTGO device exists
if [ -c "$TTGO_DEVICE" ]; then
    echo "✓ TTGO device found: $TTGO_DEVICE"
    # Check permissions
    if [ -r "$TTGO_DEVICE" ] && [ -w "$TTGO_DEVICE" ]; then
        echo "✓ Device permissions: OK"
    else
        echo "⚠ Device permissions: Need read/write access"
        echo "  Fix with: sudo chmod 666 $TTGO_DEVICE"
        echo "  Or add user to dialout: sudo usermod -a -G dialout \$USER"
    fi
else
    echo "⚠ TTGO device not found: $TTGO_DEVICE"
    echo "  Available devices:"
    ls /dev/ttyACM* /dev/ttyUSB* 2>/dev/null | sed 's/^/    /' || echo "    (none found)"
fi

echo ""
echo "Environment variables loaded successfully!"
echo "Ready to start TTGO HTTP/TCP Server!"
echo ""
echo "Usage:"
echo "  ./ttgo_http_server [--verbose] [--debug] [--help]"
echo ""
echo "Quick Test:"
echo "  ./ttgo_http_server --debug --verbose  # Test without transmission"
echo ""
echo "HTTP API Test:"
echo "  curl -X POST http://localhost:$HTTP_LISTEN_PORT/ -u admin:passw0rd \\"
echo "    -H 'Content-Type: application/json' \\"
echo "    -d '{\"capcode\":1122334,\"message\":\"Test\",\"frequency\":$DEFAULT_FREQUENCY}'"
echo ""
echo "TCP Protocol Test:"
echo "  echo '1122334|Test Message|$DEFAULT_FREQUENCY' | nc localhost $SERIAL_LISTEN_PORT"
echo ""
echo "For detailed usage examples and documentation, see README.md"
echo "=========================================="
