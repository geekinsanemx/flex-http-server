#!/bin/bash
# Environment variables configuration for hackrf_http_server
# Usage: source vars.sh && ./hackrf_http_server [OPTIONS]

# Network Configuration
export BIND_ADDRESS="127.0.0.1"        # IP to bind servers (0.0.0.0 for all interfaces)
export SERIAL_LISTEN_PORT="16175"      # TCP serial protocol port (0 = disabled)
export HTTP_LISTEN_PORT="16180"        # HTTP JSON API port (0 = disabled)

# HackRF Hardware Configuration
export SAMPLE_RATE="2000000"           # Sample rate (minimum 2M for stable operation)
export BITRATE="1600"                  # FSK bitrate (minimum 1600 for 2FSK Flex)
export AMPLITUDE="127"                 # Software amplification (-127 to 127)
export FREQ_DEV="2400"                 # Frequency deviation (±2400Hz = 4800Hz total)
export TX_GAIN="0"                     # Hardware TX gain in dB (0-47)

# Default Parameters
export DEFAULT_FREQUENCY="931937500"   # Default frequency when not specified (931.937500 MHz)

echo "=========================================="
echo "HackRF HTTP/TCP Server - Environment Setup"
echo "=========================================="
echo "Network Configuration:"
echo "  BIND_ADDRESS: $BIND_ADDRESS"
echo "  SERIAL_LISTEN_PORT: $SERIAL_LISTEN_PORT (TCP)"
echo "  HTTP_LISTEN_PORT: $HTTP_LISTEN_PORT (JSON API)"
echo ""
echo "HackRF Configuration:"
echo "  SAMPLE_RATE: $SAMPLE_RATE Hz ($(echo "scale=1; $SAMPLE_RATE/1000000" | bc -l) MSPS)"
echo "  BITRATE: $BITRATE bps"
echo "  AMPLITUDE: $AMPLITUDE ($(echo "scale=1; $AMPLITUDE*100/127" | bc -l)%)"
echo "  FREQ_DEV: ±$FREQ_DEV Hz"
echo "  TX_GAIN: $TX_GAIN dB"
echo ""
echo "Default Parameters:"
echo "  DEFAULT_FREQUENCY: $DEFAULT_FREQUENCY Hz ($(echo "scale=6; $DEFAULT_FREQUENCY/1000000" | bc -l) MHz)"
echo ""
echo "=========================================="
echo "Environment variables loaded successfully!"
echo "Ready to start HackRF HTTP/TCP Server!"
echo "Run: ./hackrf_http_server [--verbose] [--debug] [--help]"
echo ""
echo "For usage examples and documentation, see README.md"
echo "=========================================="
