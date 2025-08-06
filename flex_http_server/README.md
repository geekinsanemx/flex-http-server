# FLEX HTTP/TCP Server with AT Commands

A dual-protocol HTTP/TCP server for transmitting FLEX paging messages using devices with flex-fsk-tx firmware and AT command support.

## Features

- **AT Command Protocol**: Modern communication with flex-fsk-tx devices
- **Dual Protocol Support**: HTTP JSON API + legacy TCP protocol  
- **Device Independence**: Works with ESP32, Arduino, or any AT command compatible device
- **Authentication**: HTTP Basic Auth with htpasswd-compatible password files
- **Comprehensive Logging**: Verbose mode with detailed AT command visibility
- **Debug Mode**: Test without transmission for development
- **AWS Lambda Compatible**: Standard HTTP response codes
- **Emergency Message Resynchronization (EMR)**: Automatic synchronization for reliable paging

## Hardware Requirements

### Supported Devices
Any device running **flex-fsk-tx firmware** with AT command support:
- ESP32 modules (TTGO, Heltec, generic)
- Arduino with SX127x/SX126x LoRa modules
- Custom hardware with AT command interface

### Firmware Dependency
The device **must** be flashed with **flex-fsk-tx firmware**:
- Repository: https://github.com/geekinsanemx/flex-fsk-tx/
- **This is a required dependency** - the server will not work without it
- Provides AT command interface for FLEX protocol transmission
- Supports frequency/power control via AT commands

## AT Command Protocol

The server communicates with devices using these AT commands:

| Command | Purpose | Example | Response |
|---------|---------|---------|----------|
| `AT` | Test communication | `AT` | `OK` |
| `AT+FREQ=<MHz>` | Set frequency | `AT+FREQ=916.0000` | `OK` |
| `AT+POWER=<level>` | Set TX power (2-20) | `AT+POWER=10` | `OK` |
| `AT+SEND=<bytes>` | Prepare for binary data | `AT+SEND=256` | `+SEND: READY` |
| Binary data | Send FLEX message | (binary data) | `OK` |

## Prerequisites

### 1. Firmware Installation

**CRITICAL**: Flash your device with flex-fsk-tx firmware before using this server.

```bash
# Clone the flex-fsk-tx firmware repository
git clone https://github.com/geekinsanemx/flex-fsk-tx.git
cd flex-fsk-tx

# Follow firmware installation instructions
# This typically involves:
# 1. Installing PlatformIO or Arduino IDE
# 2. Configuring for your specific board type  
# 3. Compiling and uploading the firmware
# 4. Testing the AT command interface

# Verify firmware installation
# Connect to your device and test AT commands:
screen /dev/ttyUSB0 115200
# Type: AT (should respond with OK)
# Type: AT+FREQ=916.0000 (should respond with OK)
# Exit: Ctrl+A, K
```

### 2. Build Dependencies

Install required build tools:

```bash
# Ubuntu/Debian
sudo apt update
sudo apt install build-essential libcrypt-dev

# Check dependencies
make check-deps
```

### 3. User Permissions

Add your user to the dialout group for serial device access:

```bash
sudo usermod -a -G dialout $USER
newgrp dialout
```

### 4. Build the Server

```bash
# Build the server
make

# Test device connection (verify firmware is working)
make test-flex

# Test build
make test-build
```

## Configuration

Edit `config.ini`:

```ini
# Network
BIND_ADDRESS=127.0.0.1
HTTP_LISTEN_PORT=16180
SERIAL_LISTEN_PORT=16175

# FLEX Hardware (AT Commands)
FLEX_DEVICE=/dev/ttyUSB0
FLEX_BAUDRATE=115200
FLEX_POWER=2

# Frequency (adjust for your region)
DEFAULT_FREQUENCY=916000000
```

## Quick Start

```bash
# Test mode (no transmission)
./flex_http_server --debug --verbose

# Production mode
./flex_http_server --verbose
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

## AT Command Logging

When using `--verbose` mode, you can see all AT command communication:

```
=== Message Processing Started ===
Input Parameters:
  CAPCODE: 1122334
  MESSAGE: 'Test Message' (12 characters)
  FREQUENCY: 916000000 Hz (916.000000 MHz)

AT Command Transmission Details:
  Frequency: 916.0000 MHz
  Power: 2
  Data size: 256 bytes
  Configuring radio parameters...
  Sending AT: AT+FREQ=916.0000
  Received AT: 'OK'
  Sending AT: AT+POWER=2
  Received AT: 'OK'
  Attempting to send data (attempt 1/3)...
  Sending AT: AT+SEND=256
  Received AT: '+SEND: READY'
  Device ready! Sending 256 bytes of binary data...
  Sent 256/256 bytes (100.0%)
  Binary data sent successfully. Waiting for transmission completion...
  Received AT: 'OK'
  Transmission completed successfully!
```

## Authentication

### Password Management

The server uses htpasswd-compatible password files:

```bash
# Create/update user with bcrypt (recommended)
htpasswd -B passwords username

# Create/update user with SHA512
htpasswd -6 passwords username

# Delete user
htpasswd -D passwords username

# Verify password file
cat passwords
```

### Default Credentials
- Username: `admin`
- Password: `passw0rd`
- File: `passwords` (auto-created if missing)

**⚠️ Change default credentials for production use!**

## Command Line Options

```bash
./flex_http_server [OPTIONS]

Options:
  --help, -h     Show help message
  --debug, -d    Debug mode (show AT commands, skip transmission)
  --verbose, -v  Verbose logging (detailed AT command pipeline)
```

## Configuration Reference

### Network Settings
- `BIND_ADDRESS`: IP to bind (127.0.0.1 = localhost, 0.0.0.0 = all interfaces)
- `SERIAL_LISTEN_PORT`: TCP port for legacy protocol (0 = disabled)
- `HTTP_LISTEN_PORT`: HTTP port for JSON API (0 = disabled)
- `HTTP_AUTH_CREDENTIALS`: Password file path

### FLEX Settings (AT Commands)
- `FLEX_DEVICE`: Serial device path (/dev/ttyUSB0, /dev/ttyACM0, etc.)
- `FLEX_BAUDRATE`: Serial baudrate (115200 standard for flex-fsk-tx)
- `FLEX_POWER`: TX power level (2-20, start with low values)
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
| 11-15 | Long | Outdoor, extended range |
| 16-20 | Maximum | Long range (check regulations) |

**Start with low power and increase as needed.**

## Troubleshooting

### Common Issues

#### Serial Device Not Found
```bash
# Check available devices
ls /dev/ttyUSB* /dev/ttyACM*

# Check device permissions
ls -l /dev/ttyUSB0

# Add user to dialout group
sudo usermod -a -G dialout $USER
newgrp dialout
```

#### AT Commands Not Working
```bash
# Test manual AT command communication
screen /dev/ttyUSB0 115200
# Type: AT (should respond with OK)
# Type: AT+FREQ=916.0000 (should respond with OK)
# Type: AT+POWER=5 (should respond with OK)
# Exit: Ctrl+A, K

# Check if flex-fsk-tx firmware is loaded
# Device should respond to AT commands
# If no response, firmware may not be installed
```

#### Permission Denied
```bash
# Fix device permissions temporarily
sudo chmod 666 /dev/ttyUSB0

# Or add permanent udev rule
echo 'SUBSYSTEM=="tty", ATTRS{idVendor}=="1a86", MODE="0666"' | sudo tee /etc/udev/rules.d/99-flex.rules
sudo udevadm control --reload-rules

# For CP2102/ESP32 devices
echo 'SUBSYSTEM=="tty", ATTRS{idVendor}=="10c4", MODE="0666"' | sudo tee -a /etc/udev/rules.d/99-flex.rules
```

#### Device Not Responding to AT Commands
```bash
# 1. Verify firmware is loaded
# 2. Check baudrate matches firmware (usually 115200)
# 3. Try different USB port
# 4. Check USB cable (data + power, not just power)
# 5. Reset device and try again
```

### Debug Mode

Use debug mode for testing without transmission:

```bash
./flex_http_server --debug --verbose
```

This will:
- Show all AT commands that would be sent
- Skip actual RF transmission  
- Validate message encoding
- Test API functionality
- Display full AT command protocol flow

### Verbose Logging

Enable comprehensive logging to see AT command communication:

```bash
./flex_http_server --verbose 2>&1 | tee server.log
```

Shows:
- HTTP request/response details
- Complete AT command sequences  
- Device responses and error handling
- FLEX encoding hex dumps
- Message processing pipeline
- Transmission progress with retry logic

## System Service Installation

For production deployments, install as a systemd service:

### Quick Installation

```bash
# 1. Build and install binary
make
sudo cp flex_http_server /usr/local/bin/
sudo chmod +x /usr/local/bin/flex_http_server

# 2. Create system directories and user
sudo mkdir -p /opt/flex-server
sudo useradd -r -s /bin/false -d /opt/flex-server flex
sudo usermod -a -G dialout flex
sudo chown flex:flex /opt/flex-server

# 3. Copy configuration  
sudo cp config.ini /opt/flex-server/
sudo chown flex:flex /opt/flex-server/config.ini

# 4. Create systemd service file
sudo tee /etc/systemd/system/flex-http-server.service > /dev/null << 'EOF'
[Unit]
Description=FLEX HTTP/TCP Paging Server with AT Commands
After=network.target
Wants=network.target

[Service]
Type=simple
User=flex
Group=flex
WorkingDirectory=/opt/flex-server
ExecStart=/usr/local/bin/flex_http_server --verbose
Restart=always
RestartSec=5
StandardOutput=journal
StandardError=journal

# Security settings
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=strict
ProtectHome=true
ReadWritePaths=/opt/flex-server

[Install]
WantedBy=multi-user.target
EOF

# 5. Enable and start service
sudo systemctl daemon-reload
sudo systemctl enable flex-http-server
sudo systemctl start flex-http-server
```

### Service Management

```bash
# Check service status
sudo systemctl status flex-http-server

# View logs  
sudo journalctl -u flex-http-server -f

# Restart service
sudo systemctl restart flex-http-server

# Stop service
sudo systemctl stop flex-http-server

# Disable service
sudo systemctl disable flex-http-server
```

## Development

### Building from Source

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt update
sudo apt install build-essential libcrypt-dev

# Clone and build
git clone <repository>
cd flex-http-server
make

# Run dependency and device tests
make check-deps
make test-flex
```

### Makefile Targets

```bash
# Building
make          # Build the server (default)
make debug    # Build with debug symbols
make clean    # Remove build artifacts

# Testing  
make check-deps  # Check build dependencies
make test-flex   # Test FLEX device AT commands
make test-build  # Test compilation only
make run-debug   # Run in debug mode
make run-verbose # Run in verbose mode

# Installation
make install     # Install to /usr/local/bin
make uninstall   # Remove from system

make help        # Show all available targets
```

### Code Structure

```
├── main.cpp                    # Main server implementation  
├── include/
│   ├── config.hpp             # Configuration management (FLEX_* settings)
│   ├── http_util.hpp          # HTTP protocol utilities
│   ├── tcp_util.hpp           # TCP server utilities
│   └── flex_at_util.hpp       # AT command communication utilities
├── ../tinyflex/tinyflex.h     # FLEX protocol encoder
├── config.ini                 # Configuration file (FLEX device settings)
├── Makefile                   # Build system
└── README.md                  # This file
```

## Differences from TTGO Version

This FLEX server uses **AT commands** instead of the custom TTGO protocol:

### Protocol Comparison

| Aspect | TTGO Version | FLEX Version |
|--------|--------------|--------------|
| **Communication** | Custom binary protocol | Standard AT commands |
| **Commands** | `f 916.0000\n` | `AT+FREQ=916.0000\r\n` |
| **Power** | `p 10\n` | `AT+POWER=10\r\n` |
| **Data Send** | `m 256\n` + data | `AT+SEND=256\r\n` + data |
| **Responses** | `CONSOLE:0` | `OK` / `ERROR` |
| **Device Support** | TTGO specific | Universal AT compatible |
| **Firmware** | ttgo-fsk-tx | flex-fsk-tx (AT commands) |

### Migration from TTGO

If migrating from the TTGO version:

1. **Flash new firmware**: Replace ttgo-fsk-tx with flex-fsk-tx (AT commands)
2. **Update config**: Change `TTGO_*` to `FLEX_*` (backward compatible)
3. **Test AT commands**: Verify device responds to AT commands
4. **Update scripts**: API remains the same, only internal communication changed

### Configuration Compatibility

The server supports both old and new configuration keys:

```ini
# New format (preferred)
FLEX_DEVICE=/dev/ttyUSB0
FLEX_BAUDRATE=115200  
FLEX_POWER=2

# Legacy format (still supported)
TTGO_DEVICE=/dev/ttyUSB0
TTGO_BAUDRATE=115200
TTGO_POWER=2
```

## Related Projects

- **flex-fsk-tx**: https://github.com/geekinsanemx/flex-fsk-tx/ (Required AT command firmware)
- **Original TTGO version**: Uses custom protocol instead of AT commands
- **TinyFlex**: FLEX protocol encoding library (dependency)

## License

This project is released into the public domain.

## Contributing

1. Fork the repository
2. Create your feature branch
3. Test with `make test-flex` and `make run-debug`
4. Submit a pull request

## Support

- Check device compatibility with flex-fsk-tx firmware
- Verify AT command communication with `screen /dev/ttyUSB0 115200`
- Use `--debug --verbose` for troubleshooting
- Check logs with `journalctl -u flex-http-server -f` for service installations
