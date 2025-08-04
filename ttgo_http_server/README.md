# TTGO HTTP/TCP FLEX Paging Server

A dual-protocol HTTP/TCP server for transmitting FLEX paging messages using TTGO ESP32 + SX127x modules with the ttgo-fsk-tx firmware.

## Features

- **Dual Protocol Support**: HTTP JSON API + legacy TCP protocol
- **TTGO Hardware Integration**: Direct communication with TTGO ESP32 + SX127x modules
- **Authentication**: HTTP Basic Auth with htpasswd-compatible password files
- **Comprehensive Logging**: Verbose mode with detailed pipeline visibility
- **Debug Mode**: Test without transmission for development
- **AWS Lambda Compatible**: Standard HTTP response codes
- **Emergency Message Resynchronization (EMR)**: Automatic synchronization for reliable paging

## Hardware Requirements

### TTGO ESP32 + SX127x Module
- TTGO LoRa32 V1/V2
- TTGO T-Beam

### Firmware Dependency
The TTGO module **must** be flashed with the **ttgo-fsk-tx** firmware:
- Repository: https://github.com/rlaneth/ttgo-fsk-tx/
- **This is a required dependency** - the server will not work without it
- Supports FLEX protocol transmission over FSK modulation
- Provides serial command interface for frequency/power control

## Prerequisites

### 1. TTGO Firmware Installation

**CRITICAL**: Before using this server, you must flash your TTGO device with the required firmware.

```bash
# Clone the ttgo-fsk-tx firmware repository
git clone https://github.com/rlaneth/ttgo-fsk-tx.git
cd ttgo-fsk-tx

# Follow the firmware installation instructions in that repository
# This typically involves:
# 1. Installing PlatformIO or Arduino IDE
# 2. Configuring for your specific TTGO board type
# 3. Compiling and uploading the firmware
# 4. Testing the serial interface

# Verify firmware installation
# Connect to your TTGO device and test:
screen /dev/ttyACM0 115200
# Type: f 916.0000
# Expected response: CONSOLE:0
# Exit: Ctrl+A, K
```

**Supported TTGO boards** (verified with ttgo-fsk-tx firmware):
- TTGO LoRa32 V1/V2
- TTGO T-Beam  
- LILYGO TTGO LoRa32 V2.1_1.6

**Note**: The ttgo-fsk-tx firmware is specifically designed for "TTGO LoRa32-OLED v2.1.6 board". Other ESP32 + SX127x boards may work but are not officially verified. If you have a different board, you may need to modify the firmware or use alternative ESP32 + SX127x compatible firmware.

### 2. Build Dependencies

Install the required build tools:

```bash
# Ubuntu/Debian
sudo apt update
sudo apt install build-essential libcrypt-dev

# Check dependencies
make check-deps
```

```bash
# Build the server (dependencies should already be installed)
make

# Test TTGO connection (verify firmware is working)
make test-ttgo
```

### 2. User Permissions

Add your user to the dialout group for serial device access:

```bash
sudo usermod -a -G dialout $USER
newgrp dialout
```

### 3. Configuration

Edit `config.ini`:

```ini
# Network
BIND_ADDRESS=127.0.0.1
HTTP_LISTEN_PORT=16180
SERIAL_LISTEN_PORT=16175

# TTGO Hardware
TTGO_DEVICE=/dev/ttyACM0
TTGO_BAUDRATE=115200
TTGO_POWER=2

# Frequency (adjust for your region)
DEFAULT_FREQUENCY=916000000
```

### 4. Start the Server

```bash
# Test mode (no transmission)
./ttgo_http_server --debug --verbose

# Production mode
./ttgo_http_server --verbose
```

## API Documentation

### HTTP JSON API

**Endpoint**: `POST http://localhost:16180/`  
**Authentication**: HTTP Basic Auth (admin/passw0rd by default)  
**Content-Type**: application/json

#### Request Format
```json
{
  "capcode": 1122334,           // REQUIRED: Target pager capcode
  "message": "Hello World",     // REQUIRED: Message text
  "frequency": 916000000        // OPTIONAL: Frequency in Hz
}
```

#### Response Codes
- `200 OK` - Message transmitted successfully
- `400 Bad Request` - Invalid JSON or missing fields
- `401 Unauthorized` - Authentication failed
- `405 Method Not Allowed` - Only POST supported
- `500 Internal Server Error` - Transmission failure

#### Examples

```bash
# Send message with all parameters
curl -X POST http://localhost:16180/ \
  -u admin:passw0rd \
  -H 'Content-Type: application/json' \
  -d '{"capcode":1122334,"message":"Test Message","frequency":916000000}'

# Send message with default frequency
curl -X POST http://localhost:16180/ \
  -u admin:passw0rd \
  -H 'Content-Type: application/json' \
  -d '{"capcode":1122334,"message":"Default Frequency Test"}'
```

### TCP Protocol (Legacy)

**Format**: `CAPCODE|MESSAGE|FREQUENCY_HZ`

```bash
# Send via netcat
echo '1122334|Hello World|916000000' | nc localhost 16175

# Send multiple messages
printf '1122334|Message 1|916000000\n5566778|Message 2|916000000\n' | nc localhost 16175
```

## Authentication

### Password Management

The server uses htpasswd-compatible password files:

```bash
# Create/update user with bcrypt (recommended)
htpasswd -B passwords username

# Create/update user with MD5 (compatible)
htpasswd -m passwords username

# Delete user
htpasswd -D passwords username

# Verify password
htpasswd -v passwords username
```

### Default Credentials
- Username: `admin`
- Password: `passw0rd`
- File: `passwords` (auto-created if missing)

**⚠️ Change default credentials for production use!**

## Command Line Options

```bash
./ttgo_http_server [OPTIONS]

Options:
  --help, -h     Show help message
  --debug, -d    Debug mode (show commands, skip transmission)
  --verbose, -v  Verbose logging (detailed pipeline info)
```

## Configuration Reference

### Network Settings
- `BIND_ADDRESS`: IP to bind (127.0.0.1 = localhost, 0.0.0.0 = all interfaces)
- `SERIAL_LISTEN_PORT`: TCP port for legacy protocol (0 = disabled)
- `HTTP_LISTEN_PORT`: HTTP port for JSON API (0 = disabled)
- `HTTP_AUTH_CREDENTIALS`: Password file path

### TTGO Settings
- `TTGO_DEVICE`: Serial device path (/dev/ttyACM0, /dev/ttyUSB0, etc.)
- `TTGO_BAUDRATE`: Serial baudrate (115200 standard)
- `TTGO_POWER`: TX power level (2-17, start with low values)
- `DEFAULT_FREQUENCY`: Default frequency in Hz

## Frequency Bands

### Common ISM Bands
- **433 MHz**: 433.050-434.790 MHz (Europe, Asia)
- **868 MHz**: 868.000-868.600 MHz (Europe)
- **915 MHz**: 902.000-928.000 MHz (Americas)
- **2.4 GHz**: 2400-2500 MHz (Global)

### Regional Examples
```ini
# US/Canada (915 MHz ISM)
DEFAULT_FREQUENCY=915000000

# Europe (868 MHz ISM)  
DEFAULT_FREQUENCY=868000000

# Global (433 MHz ISM)
DEFAULT_FREQUENCY=433500000
```

**⚠️ Always check local regulations before transmitting!**

## Power Level Guidelines

| Power | Range | Use Case |
|-------|-------|----------|
| 2-5   | Short | Indoor, minimal interference |
| 6-10  | Medium | Indoor/outdoor, general use |
| 11-17 | Long | Outdoor, maximum range |

**Start with low power and increase as needed.**

## Troubleshooting

### Common Issues

#### Serial Device Not Found
```bash
# Check available devices
ls /dev/ttyACM* /dev/ttyUSB*

# Check device permissions
ls -l /dev/ttyACM0

# Add user to dialout group
sudo usermod -a -G dialout $USER
newgrp dialout
```

#### TTGO Not Responding
```bash
# Test manual connection
screen /dev/ttyACM0 115200
# Type: f 916.0000 (should respond with CONSOLE:0)
# Exit: Ctrl+A, K

# Check firmware
# Ensure ttgo-fsk-tx firmware is loaded
```

#### Permission Denied
```bash
# Fix device permissions
sudo chmod 666 /dev/ttyACM0

# Or add permanent udev rule
echo 'SUBSYSTEM=="tty", ATTRS{idVendor}=="1a86", MODE="0666"' | sudo tee /etc/udev/rules.d/99-ttgo.rules
sudo udevadm control --reload-rules
```

### Debug Mode

Use debug mode for testing without transmission:

```bash
./ttgo_http_server --debug --verbose
```

This will:
- Show all TTGO commands that would be sent
- Skip actual RF transmission
- Validate message encoding
- Test API functionality

### Verbose Logging

Enable comprehensive logging:

```bash
./ttgo_http_server --verbose 2>&1 | tee server.log
```

Shows:
- HTTP request/response details
- TTGO command/response sequences
- FLEX encoding hex dumps
- Message processing pipeline
- Transmission progress

## System Service Installation

For production deployments, you can install the server as a systemd service. The `ttgo-http-server.service` file contains complete installation and management instructions.

### Quick Installation

```bash
# 1. Build and install binary
make
sudo cp ttgo_http_server /usr/local/bin/
sudo chmod +x /usr/local/bin/ttgo_http_server

# 2. Create system directories and user
sudo mkdir -p /opt/ttgo-server
sudo useradd -r -s /bin/false -d /opt/ttgo-server ttgo
sudo usermod -a -G dialout ttgo
sudo chown ttgo:ttgo /opt/ttgo-server

# 3. Copy configuration
sudo cp config.ini /opt/ttgo-server/
sudo chown ttgo:ttgo /opt/ttgo-server/config.ini

# 4. Install and start service
sudo cp ttgo-http-server.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable ttgo-http-server
sudo systemctl start ttgo-http-server
```

### Service Management

```bash
# Check service status
sudo systemctl status ttgo-http-server

# View logs
sudo journalctl -u ttgo-http-server -f

# Restart service
sudo systemctl restart ttgo-http-server

# Stop service
sudo systemctl stop ttgo-http-server
```

### Manual Testing Before Service Installation

Test the server manually with the service user before installing:

```bash
sudo -u ttgo /usr/local/bin/ttgo_http_server --verbose --debug
```

See the `ttgo-http-server.service` file for complete installation instructions, troubleshooting tips, and uninstallation procedures.

## Development

### Building from Source

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt update
sudo apt install build-essential libcrypt-dev

# Clone and build
git clone <repository>
cd ttgo-http-server
make

# Run dependency and connection tests
make check-deps
make test-ttgo
```

### Makefile Targets

```bash
# Building
make          # Build the server (default)
make debug    # Build with debug symbols
make clean    # Remove build artifacts

# Testing
make check-deps  # Check build dependencies
make test-ttgo   # Test TTGO device connection
make help        # Show all available targets
```

### Code Structure

```
├── main.cpp                    # Main server implementation
├── include/
│   ├── config.hpp             # Configuration management
│   ├── http_util.hpp          # HTTP protocol utilities
│   ├── tcp_util.hpp           # TCP server utilities
│   └── ttgo_util.hpp          # TTGO communication utilities
├── ../../tinyflex.h           # FLEX protocol encoder
├── config.ini                 # Configuration file
├── Makefile                   # Build system
├── ttgo-http-server.service   # Systemd service file
└── README.md                  # This file
```

## Related Projects

- **ttgo-fsk-tx**: https://github.com/rlaneth/ttgo-fsk-tx/ (Required firmware)
