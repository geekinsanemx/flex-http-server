# HackRF HTTP/TCP Server

A dual-protocol FLEX paging server for HackRF that supports both legacy TCP serial protocol and modern HTTP JSON API with authentication. Features comprehensive verbose logging and standard HTTP response codes for seamless integration with cloud services like AWS Lambda.

## Features

- **Dual Protocol Support**: Both TCP serial and HTTP JSON APIs
- **HTTP Basic Authentication**: Secure access with htpasswd-compatible password files
- **Emergency Message Resynchronization (EMR)**: Automatic sync for reliable transmission
- **Comprehensive Verbose Logging**: Detailed pipeline visibility for debugging and monitoring
- **Standard HTTP Response Codes**: AWS Lambda and cloud service compatible
- **Debug Mode**: Signal analysis with IQ file output without transmission
- **Flexible Configuration**: File-based or environment variable configuration
- **Port Control**: Independent enable/disable for each protocol
- **System Integration**: Full Systemd service support see hackrf-http-server.service file for its usage

## Configuration

The server reads configuration from `config.ini` (preferred) or environment variables as fallback. When installed as a system service, configuration is read from `/etc/default/hackrf_http_server`.

### config.ini
```ini
# Network Configuration
BIND_ADDRESS=127.0.0.1
SERIAL_LISTEN_PORT=16175    # TCP port for serial protocol (0 = disabled)
HTTP_LISTEN_PORT=16180      # HTTP port for JSON API (0 = disabled)

# HackRF Configuration  
SAMPLE_RATE=2000000
BITRATE=1600
AMPLITUDE=127
FREQ_DEV=2400
TX_GAIN=0

# Default frequency to use when not specified in HTTP requests
DEFAULT_FREQUENCY=931937500
```

### Configuration Parameters

- **BIND_ADDRESS**: IP address to bind servers to (default: 127.0.0.1)
- **SERIAL_LISTEN_PORT**: TCP port for legacy serial protocol (default: 16175, set to 0 to disable)
- **HTTP_LISTEN_PORT**: HTTP port for JSON API (default: 16180, set to 0 to disable)
- **HTTP_AUTH_CREDENTIALS**: Password file path (default: passwords)
- **SAMPLE_RATE**: HackRF sample rate (default: 2000000, minimum: 2M)
- **BITRATE**: FSK bitrate (default: 1600, minimum for 2FSK Flex)
- **AMPLITUDE**: Software amplification (default: 127, range: -127 to 127)
- **FREQ_DEV**: Frequency deviation in Hz (default: 2400, Flex 2FSK is ±2400Hz = 4800Hz total)
- **TX_GAIN**: Hardware TX gain in dB (default: 0, range: 0-47)
- **DEFAULT_FREQUENCY**: Default frequency when not specified in HTTP requests (default: 931937500)

## Building

### Dependencies
```bash
# Install dependencies (Ubuntu/Debian)
make deps

# Or manually:
sudo apt-get install libhackrf-dev libssl-dev apache2-utils
```

### Compile
```bash
make clean && make
```

## Usage

```bash
./hackrf_http_server [OPTIONS]

OPTIONS:
  --help, -h     Show help message and exit
  --debug, -d    Enable debug mode (prints raw bytes, creates IQ file, skips transmission)
  --verbose, -v  Enable verbose output (comprehensive pipeline logging)
```

### Exit Codes
- **0**: Success
- **1**: Invalid command line arguments
- **2**: Configuration errors
- **3**: Network setup errors (port binding failures)
- **4**: Authentication setup errors

## System Service Management

After building binary you can use `hackrf-http-server.service` systemd service file provided to make it a service

### Installation:
  1. Copy this file to: /etc/systemd/system/hackrf-http-server.service
  2. Create environment file: /etc/default/hackrf_http_server
  3. Create hackrf user: sudo useradd -r -s /bin/false -d /var/lib/hackrf-server hackrf
  4. Create working directory: sudo mkdir -p /var/lib/hackrf-server
  5. Set permissions: sudo chown hackrf:hackrf /var/lib/hackrf-server
  6. Install binary: sudo cp hackrf_http_server /usr/local/bin/
  7. Reload systemd: sudo systemctl daemon-reload
  8. Enable service: sudo systemctl enable hackrf-http-server
  9. Start service: sudo systemctl start hackrf-http-server

### Service Configuration

The system service reads configuration from `/etc/default/hackrf_http_server`:

```bash
# Edit system service configuration
sudo nano /etc/default/hackrf_http_server

# Restart service after configuration changes
sudo systemctl restart hackrf-http-server
```

## Protocols

### Serial Protocol (TCP)

Legacy protocol for backward compatibility. Send messages via TCP in format:
```
{CAPCODE}|{MESSAGE}|{FREQUENCY_HZ}
```

**Examples:**
```bash
# Using netcat
echo '001122334|Hello World|925516000' | nc localhost 16175

# Using netcat with timeout
echo '2223334|Emergency Alert|931937500' | timeout 5 nc localhost 16175

# Multiple messages
echo '1122334|First message|925516000' | nc localhost 16175
echo '5555555|Second message|931937500' | nc localhost 16175
```

### HTTP Protocol (JSON API)

Modern REST API with JSON format and HTTP basic authentication.

**Endpoint:** `POST http://localhost:16180/`

**Request Format:**
```json
{
    "capcode": 1122334,       // REQUIRED: target capcode
    "message": "Hello World", // REQUIRED: message text
    "frequency": 925516000    // OPTIONAL: uses DEFAULT_FREQUENCY if omitted
}
```

**Important Notes:**
- **capcode** field is **REQUIRED** - must be specified for all requests
- **message** field is **REQUIRED** - must be specified for all requests  
- **frequency** field is **OPTIONAL** - if omitted, `DEFAULT_FREQUENCY` from config is used
- All requests require HTTP Basic Authentication
- Content-Type should be `application/json`

**Examples:**
```bash
# Complete message with all parameters
curl -X POST http://localhost:16180/ \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{"capcode": 1122334, "message": "Hello from HTTP API", "frequency": 925516000}'

# Using frequency from DEFAULT_FREQUENCY config
curl -X POST http://localhost:16180/ \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{"capcode": 1122334, "message": "Using default frequency"}'

# Emergency message with high priority capcode
curl -X POST http://localhost:16180/ \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{"capcode": 911911, "message": "EMERGENCY: System down", "frequency": 931937500}'
```

### HTTP Response Codes (compatible with AWS Lambda Response codes)

Standard HTTP response codes for seamless cloud integration:

- **200 OK**: Message transmitted successfully
- **400 Bad Request**: Invalid JSON format, missing required fields (capcode/message), or malformed data
- **401 Unauthorized**: Authentication required or credentials invalid
- **405 Method Not Allowed**: Only POST requests are supported
- **500 Internal Server Error**: Message processing or transmission failure

**Response Format:**
```json
// Success (200 OK)
{"status": "success", "message": "Message transmitted successfully"}

// Error (400/401/405/500)  
{"error": "Error description", "code": 400}
```

## Authentication

HTTP requests require HTTP Basic Authentication. The password file location is configurable via the `HTTP_AUTH_CREDENTIALS` parameter in `config.ini` or environment variables.

### Password File Configuration

The password file path can be specified in `config.ini`:

```ini
# Authentication Configuration
HTTP_AUTH_CREDENTIALS=passwords              # Default: current directory
# HTTP_AUTH_CREDENTIALS=/etc/hackrf/passwords # Absolute path
# HTTP_AUTH_CREDENTIALS=auth/users.htpasswd   # Relative path
```

Or via environment variable:
```bash
export HTTP_AUTH_CREDENTIALS=/var/lib/hackrf-server/passwords
```

### Default Credentials

If the specified password file doesn't exist, it will be created automatically with default credentials:
- **Username:** `admin`
- **Password:** `passw0rd`

### Managing Users

Use the `htpasswd` tool to manage user accounts in your configured password file:

```bash
# For custom password file location
htpasswd -B /path/to/your/passwords username

# For default location (current directory)
htpasswd -B passwords username

# For system service installation
sudo -u hackrf htpasswd -B /var/lib/hackrf-server/passwords username

# Add or update a user with MD5 hash (compatible with most systems)
htpasswd -m /path/to/passwords username

# Create new passwords file and add first user
htpasswd -cm /path/to/passwords admin

# Delete a user
htpasswd -D /path/to/passwords username

# Verify a password
htpasswd -v /path/to/passwords username

# List users
cut -d: -f1 /path/to/passwords
```

### Password File Formats

The server supports multiple htpasswd hash formats:
- **bcrypt** (`-B`): Recommended for security
- **SHA512** (`-6`): Good security, widely supported
- **MD5** (`-m`): Maximum compatibility
- **Plain text**: For testing only (not recommended)

### Security Considerations

- **File Permissions**: Ensure password files have appropriate permissions (e.g., `chmod 600`)
- **Secure Locations**: Store password files in secure directories
- **Strong Passwords**: Use complex passwords for production environments
- **Regular Updates**: Rotate passwords periodically

Example secure setup:
```bash
# Create secure password file location
sudo mkdir -p /etc/hackrf
sudo touch /etc/hackrf/passwords
sudo chmod 600 /etc/hackrf/passwords
sudo chown hackrf:hackrf /etc/hackrf/passwords

# Add users with strong passwords
sudo htpasswd -B /etc/hackrf/passwords admin
sudo htpasswd -B /etc/hackrf/passwords operator

# Update config.ini
echo "HTTP_AUTH_CREDENTIALS=/etc/hackrf/passwords" >> config.ini
```

## Usage Examples

### Starting the Server

```bash
# Start with verbose logging
./hackrf_http_server --verbose

# Start in debug mode (no transmission)
./hackrf_http_server --debug --verbose

# Start with environment variables
source vars.sh && ./hackrf_http_server --verbose

# Monitor with full verbose output
./hackrf_http_server --verbose 2>&1 | tee server.log
```

### Complete Examples

```bash
# Test with required fields only
curl -X POST http://localhost:16180/ \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{"capcode": 1122334, "message": "Test message"}'

# Test with all fields
curl -X POST http://localhost:16180/ \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{
    "capcode": 1122334,
    "message": "Complete message with frequency",
    "frequency": 925516000
  }'

# Test missing capcode (should return 400)
curl -v -X POST http://localhost:16180/ \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{"message": "Missing capcode"}'

# Test missing message (should return 400)
curl -v -X POST http://localhost:16180/ \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{"capcode": 1122334}'
```

## Verbose Logging

The `--verbose` flag enables comprehensive pipeline logging that shows every step of message processing:

### Verbose Output Sections

1. **Configuration Display**: Shows all loaded configuration parameters
2. **HTTP Client Connection**: Client IP, port, and raw HTTP request data
3. **Request Parsing**: Parsed HTTP method, path, headers, and body
4. **JSON Message Processing**: Shows parsed message data with validation results
5. **Message Processing Pipeline**:
   - Input parameter validation (capcode and message required)
   - Capcode validation (SHORT/LONG format detection)
   - FLEX encoding with complete hex dump display
   - Binary analysis (total bits, transmission time)
6. **HackRF Setup**: Device configuration and status
7. **FSK Modulation**: Detailed modulation parameters and sample generation
8. **File Output**: IQ file creation status (debug mode)
9. **RF Transmission**: Real-time transmission progress
10. **HTTP Response**: Response codes and body content

### Monitoring and Debugging Examples

```bash
# Monitor with full verbose output
./hackrf_http_server --verbose 2>&1 | tee server.log

# Debug mode with IQ file generation
./hackrf_http_server --debug --verbose
# Creates flexserver_output.iq for analysis

# Monitor HTTP responses
tail -f server.log | grep 'HTTP Response'

# Count successful transmissions
grep -c 'Message Processing Completed' server.log

# Monitor validation failures
tail -f server.log | grep -E '(Missing required field|Invalid)'

# Watch for authentication failures
tail -f server.log | grep -i 'unauthorized\|authentication'
```

## Advanced Features

### Port Configuration
- Both serial and HTTP ports can be independently enabled/disabled
- Set port to `0` in configuration to disable that protocol
- At least one port must be enabled for the server to start

### Emergency Message Resynchronization (EMR)
- Automatically sends EMR messages before the first transmission
- Sends EMR if no messages have been sent for more than 10 minutes  
- Ensures proper synchronization with paging receivers
- EMR transmission is logged in verbose mode

### Debug Mode
- Use `--debug` flag to enable debug mode
- Prints raw encoded bytes in hex format (complete output, no truncation)
- Creates `flexserver_output.iq` file for signal analysis with tools like GNU Radio
- Skips actual HackRF transmission for safe testing
- Shows EMR status without transmission

### Capcode Validation
- Supports both SHORT (18-bit) and LONG (32-bit) capcodes
- Automatic format detection and validation
- Detailed validation logging in verbose mode

### FLEX Encoding
- Uses TinyFlex library for FLEX protocol encoding
- Comprehensive error handling with detailed error codes
- Complete hex dump display of encoded data for analysis

## Troubleshooting

### Common Issues

1. **"bind failed" error**
   - Port already in use by another process
   - Check with: `netstat -tlnp | grep :16175`
   - Change port in configuration or stop conflicting process

2. **"hackrf_open() failed"**
   - HackRF device not connected or recognized
   - Check USB connection and run: `hackrf_info`
   - May need to run with sudo or add user to plugdev group

3. **Authentication failures**
   - Verify credentials with: `htpasswd -v passwords username`
   - Check passwords file permissions and format
   - Ensure basic auth header is properly formatted
   - Use verbose mode to see authentication attempts

4. **"Missing required field" errors**
   - Both `capcode` and `message` fields are required in JSON requests
   - Only `frequency` field is optional (uses DEFAULT_FREQUENCY if omitted)
   - Check JSON syntax and ensure all required fields are present
   - Use verbose mode to see validation details

5. **"Invalid capcode" errors**
   - Capcode must be a valid numeric value
   - Check capcode format and range limits
   - Use verbose mode to see validation details

6. **JSON parsing errors**
   - Ensure valid JSON format
   - Check for missing quotes around string values
   - Verify Content-Type header is set to `application/json`

### System Service Troubleshooting

```bash
# Check service status
sudo systemctl status hackrf-http-server

# View recent logs
sudo journalctl -u hackrf-http-server --since "1 hour ago"

# Follow logs in real-time
sudo journalctl -u hackrf-http-server -f

# Check configuration
cat /etc/default/hackrf_http_server

# Test configuration manually
sudo -u hackrf /usr/local/bin/hackrf_http_server --verbose

# Check file permissions
ls -la /var/lib/hackrf-server/
sudo -u hackrf ls -la /var/lib/hackrf-server/
```

### Debugging Commands

```bash
# Test JSON parsing without transmission
./hackrf_http_server --debug --verbose

# Check raw IQ samples
./hackrf_http_server --debug
# Creates flexserver_output.iq file for analysis

# Test authentication
curl -v -X POST http://localhost:16180/ \
  -u admin:wrongpassword \
  -H "Content-Type: application/json" \
  -d '{"capcode": 1122334, "message": "test"}'

# Test required field validation
curl -v -X POST http://localhost:16180/ \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{"message": "missing capcode"}'

# Check port usage
netstat -tlnp | grep :16180

# Test HackRF device
hackrf_info
lsusb | grep HackRF

# Validate JSON syntax
echo '{"capcode": 1122334, "message": "test"}' | jq .
```

## Integration Examples

### Python Client Example
```python
import requests
import json

class HackRFClient:
    def __init__(self, base_url, username, password):
        self.base_url = base_url
        self.auth = (username, password)
        self.headers = {'Content-Type': 'application/json'}

    def send_message(self, capcode, message, frequency=None):
        # Both capcode and message are required
        payload = {
            'capcode': capcode,
            'message': message
        }

        # Frequency is optional
        if frequency:
            payload['frequency'] = frequency

        try:
            response = requests.post(
                f'{self.base_url}/',
                auth=self.auth,
                headers=self.headers,
                json=payload,
                timeout=30
            )

            if response.status_code == 200:
                return {'success': True, 'data': response.json()}
            else:
                return {'success': False, 'error': response.text, 'code': response.status_code}

        except requests.exceptions.RequestException as e:
            return {'success': False, 'error': str(e)}

# Usage
client = HackRFClient('http://localhost:16180', 'admin', 'passw0rd')

# Send with all parameters
result = client.send_message(911911, 'Emergency alert', frequency=931937500)

# Send with default frequency
result = client.send_message(1122334, 'Simple message')

print(result)
```

### Shell Script Integration
```bash
#!/bin/bash
# send_alert.sh - Send alert via HTTP API

HACKRF_URL="http://localhost:16180/"
AUTH="admin:passw0rd"

send_alert() {
    local capcode="$1"
    local message="$2"
    local frequency="$3"

    if [ -z "$capcode" ] || [ -z "$message" ]; then
        echo "Usage: send_alert CAPCODE MESSAGE [FREQUENCY]"
        return 1
    fi

    # Build JSON payload
    local payload="{\"capcode\": $capcode, \"message\": \"$message\""
    if [ -n "$frequency" ]; then
        payload="$payload, \"frequency\": $frequency"
    fi
    payload="$payload}"

    response=$(curl -s -w "%{http_code}" -X POST "$HACKRF_URL" \
        -u "$AUTH" \
        -H "Content-Type: application/json" \
        -d "$payload")

    http_code="${response: -3}"

    if [ "$http_code" = "200" ]; then
        echo "Alert sent successfully"
        return 0
    else
        echo "Alert failed with HTTP code: $http_code"
        echo "Response: ${response%???}"
        return 1
    fi
}

# Usage examples
send_alert 1122334 "System startup completed"
send_alert 911911 "Emergency: Fire alarm activated" 931937500
send_alert 99999 "Backup completed successfully"
```

### Frequency Planning

Common paging frequencies and considerations:

Most Common FLEX Frequencies
```
| Frequency (MHz) | Use Case                  | Example Network         |
| --------------- | ------------------------- | ----------------------- |
| **929.6625**    | Nationwide paging         | American Messaging      |
| **929.9375**    | FLEX (popular)            | SkyTel FLEX             |
| **931.4375**    | FLEX / 2-way paging       | Motorola infrastructure |
| **931.8875**    | FLEX (high capacity)      | Commercial pagers       |
| **931.9375**    | FLEX (customized systems) | Private / hospital use  |
| **940.0+**      | Response / return channel | For 2-way FLEX systems  |
```
- **931.9375 MHz**: Default frequency used by this server

**Important**: Always check local regulations and ensure you have proper licensing before transmitting on any frequency.
